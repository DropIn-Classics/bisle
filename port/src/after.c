/* after.c - BATTLE.EXE's T15AC: the two players' numbers of units after
 * each change of phase (history_add: 64 of them a player, in a ring) and
 * the screen after a map: the two curves of those numbers, the score,
 * the rounds, the next map's code.
 */
#include "bi.h"

/* a text made on the stack in the original: in the megabyte below the
 * program here */
#define CODE_SCRATCH MKFP(0x0050, 0x00C0)

/* the history's record: +0 1 when the ring is full, +1 and +5 the two
 * players' buffers, +9 where the next goes */
#define HISTORY FP(state_248e)

void history_add(int units0, int units1)
{
    fptr h = HISTORY;

    spb(pfp(h, 1), pb(h, 9), units0);
    spb(pfp(h, 5), pb(h, 9), units1);
    spb(h, 9, pb(h, 9) + 1);
    if (pb(h, 9) >= 0x3F)
        spb(h, 0, 1);
    spb(h, 9, pb(h, 9) & 0x3F);
}

/* a player's numbers put in their order (a full ring turned so that the
 * oldest is first, through `tmp`): how many there are */
static int history_rows(int which, fptr tmp)
{
    fptr h = HISTORY, src = pfp(h, which ? 5 : 1);
    unsigned c = pb(h, 9), i = 0;

    if (pb(h, 0) != 1)
        return pb(h, 9);
    do {
        spb(tmp, i++, pb(src, c));
        c = (c + 1) & 0x3F;
    } while (pb(h, 9) != c);
    for (i = 0; i < 0x40; i++)
        spb(src, i, pb(tmp, i));
    return 0x40;
}

/* T15AC:0650: the largest of n bytes */
static unsigned t15ac_0650(fptr data, unsigned n)
{
    unsigned top = pb(data, 0), i;

    for (i = 0; i < n; i++)
        if (pb(data, i) > top)
            top = pb(data, i);
    return top;
}

/* T15AC:095B: the library's entry drawn along the line from x1, y1 to
 * x2, y2, at every x */
static void t15ac_095b(int x1, int y1, int x2, int y2, fptr entry)
{
    int32_t slope;
    int x;

    if (x2 == x1)
        x2++;
    slope = (int32_t)((uint32_t)(uint16_t)(y2 - y1) << 16) / (int32_t)(int16_t)(x2 - x1);
    for (x = x1; x <= x2; x++) {
        uint32_t part = (uint32_t)slope * (uint32_t)(int32_t)(int16_t)(x - x1);

        draw_entry(x, (int16_t)((part >> 16) + (unsigned)y1), entry, 0, 0, 0);
    }
}

/* a curve of the history's numbers from x, y to the right, upwards 64
 * rows for `top`: a line in `colour` and the entry along it; 4, 8 or 16
 * pixels a number by how many there are */
static void draw_curve(int x, int y, fptr data, fptr entry, int colour, uint32_t top)
{
    unsigned n = GB(state_2497), step = n >= 0x20 ? 4 : n >= 0x10 ? 8 : 0x10, k;
    uint32_t scale;
    int y1, y2, x2;

    if (!top)
        top = 0x40;
    scale = 0x400000u / top;
    y1 = (int16_t)(y - (int16_t)((scale * pb(data, 0)) >> 16));
    for (k = 1; n > k; k++) {
        x2 = x + (int)step;
        y2 = (int16_t)(y - (int16_t)((scale * pb(data, k)) >> 16));
        SW(draw_x1, x), SW(draw_y1, y1), SW(draw_x2, x2), SW(draw_y2, y2);
        SW(draw_colour, colour);
        draw_line();
        t15ac_095b(x, y1, x2, y2, entry);
        y1 = y2;
        x = x2;
    }
}

/* the screen after a map (not after a map left before the first change
 * of phase): STATS' picture, the curves, the winner's or the loser's
 * song; a map won gives the next map's number (the codes' file: +7 how
 * far on, +8 bit 1 the last map) and its code, 500 more points with
 * game_flags' bit 2000h; a map lost gives no score */
void after_map(fptr buffer)
{
    fptr palette = buffer, work, song, rows, codes, lib, table, name;
    unsigned m0, m1, top, n, i;

    buffer = hadd(buffer, 0x320);
    if (GW(round) < 1)
        return;
    work = buffer;
    buffer = hadd(buffer, 0x2710);
    song = buffer;
    buffer = hadd(buffer, 0x9858);
    rows = buffer;
    buffer = hadd(buffer, 0x400);
    codes = buffer;
    buffer = hadd(buffer, 0x2710);
    lib = buffer;
    load_picture(buffer, make_path(1, -1, FP(name_stats), 0x0A), work, NULL, NULL);
    name = t26ea_000f((int8_t)GBO(menu_items, 11 * 0x2E + 0x2B), FP(save_name), 2, 4);
    if (!load_file(palette, make_path(1, -1, name, 4), work))
        fatal_error(2);
    for (i = 0; i < 0x300; i++)
        SBO(picture_palette, i, pb(palette, i));
    name = make_path(1, -1, (GW(game_flags) & 0x10) ? FP(name_winner) : FP(name_looser), 7);
    if (load_song(song, name, work) == -1)
        return;
    if (load_lib(make_path(1, 0, FP(lib_stats), 3), lib, work, FP(lib_stats), 0) == -1)
        return;
    if (!load_file(codes, make_path(1, -1, FP(name_codes), 5), work))
        return;
    history_rows(0, rows);
    SB(state_2497, history_rows(1, rows));
    m0 = t15ac_0650(GFP(buffer_248f), GB(state_2497));
    m1 = t15ac_0650(GFP(buffer_2493), GB(state_2497));
    top = m0 > m1 ? m0 : m1;
    table = pfp(FP(lib_stats), 0x0E);
    draw_curve(0x1D, 0x48, GFP(buffer_248f), pfp(table, 0), 3, top);
    draw_curve(0x1D, 0xA8, GFP(buffer_2493), pfp(table, 4), 0x12, top);
    SW(pass_ticks, GW(map_number));
    if (GW(game_flags) & 0x2000)
        SD(score_now, GD(score_now) + 0x1F4);
    if (GW(game_flags) & 0x10) {
        SW(map_number, GW(map_number) + pb(codes, 10 * (int16_t)GW(map_number) + 7));
        if (pb(codes, 10 * (int16_t)GW(pass_ticks) + 8) & 1) {
            draw_text(0x28, 0xB6, 0x18, GBO(amok, 0x0D));
            SW(game_flags, GW(game_flags) | 0x20);
        } else {
            draw_text(0x28, 0xB6, 0x12, GBO(amok, 0x0D));
            for (i = 0; i < 5; i++)
                spb(CODE_SCRATCH, i, pb(codes, 10 * (int16_t)GW(map_number) + i) + 0x30);
            spb(CODE_SCRATCH, 5, 0);
            SW(draw_colour, GBO(amok, 0x0B));
            draw_chars(0x28 + 0xDE, 0xB6, CODE_SCRATCH);
        }
    } else {
        draw_text(0x28, 0xB6, 0x11, GBO(amok, 0x0D));
        SD(score_now, 0);
    }
    draw_text(0x28, 0x56, 0x1C, GBO(amok, 0x0D));
    SW(draw_colour, GBO(amok, 0x0B));
    draw_number((int16_t)GW(score_now), 0x5E, 0x56);
    draw_text(0x82, 0x56, 0x13, GBO(amok, 0x0D));
    SW(draw_colour, GBO(amok, 0x0B));
    draw_number((int16_t)GW(round), 0xB8, 0x56);
    n = GB(state_2497);
    draw_text(0xD2, 0x56, 0x14, GBO(amok, 0x0D));
    draw_text(0x102, 0x56, n >= 0x20 ? 0x17 : n >= 0x10 ? 0x16 : 0x15, GBO(amok, 0x0B));
    /* MOON starts the song after the fade (its second pointer, the same
     * as the first) */
    if (bi_prog != BI_MOON)
        play_song(0, 0);
    flip_page();
    fade_in();
    if (bi_prog == BI_MOON)
        play_song(0, 1);
    t0d36_000f(0x32);
    t0d36_0c85();
    fade_out();
    stop_song();
}
