/* save.c - BATTLE.EXE's T13CA: a game saved and loaded.  The file is a
 * header of 12h bytes (0, 7, the history's two bytes, of each player the
 * word +0 and the bytes +2, +3, +4, +15h, +16h) and then 23 parts of the
 * program's memory, as they are: the two maps, the players' lists of
 * moves, the history, the palette's buffer, the game's state, the
 * cursors, the marks, the buildings, the cargo, the units, the types,
 * the computer player's state.
 */
#include "bi.h"

#define PLAYER(n) MKFP(S_players, A_players + 0x17 * (n))
#define PARTS 0x17

/* T13CA:06DF: the parts' table (a far pointer and a long each) */
static void parts(fptr t, fptr palette)
{
    static const uint16_t sizes[PARTS] = {
        0x2008, 0x2008, 0xF2, 0xF2, 0x82, 0x82, 0x320, 0x8F, 0x31, 0x31, 6, 0x1104,
        0x1C * 0x0A, 0x1C * 0x0A, 0x1C * 2, 0x1C * 0x46, 0x1A * 0xF1, 0x44 * 0x1B,
        9 * 0xF1, 9 * 0xF1, 0x0C * 2, 0x49 * 2, 0x20 * 2,
    };
    fptr at[PARTS];
    unsigned i;

    at[0] = GFP(map0);
    at[1] = GFP(map1);
    at[2] = pfp(PLAYER(0), 0x11);
    at[3] = pfp(PLAYER(1), 0x11);
    at[4] = GFP(buffer_248f);
    at[5] = GFP(buffer_2493);
    at[6] = palette;
    at[7] = FP(game_flags);
    at[8] = CURSOR(0);
    at[9] = CURSOR(1);
    at[10] = FP(map_bytes);
    at[11] = FP(marks);
    at[12] = FP(factories);
    at[13] = FP(depots);
    at[14] = FP(hqs);
    at[15] = FP(cargo);
    at[16] = FP(units);
    at[17] = FP(unit_types);
    at[18] = FP(computer_attack_units);
    at[19] = FP(computer_move_units);
    at[20] = FP(computer_state);
    at[21] = FP(computer_queues);
    at[22] = FP(computer_scripts);
    for (i = 0; i < PARTS; i++) {
        spfp(t, 8 * i, at[i]);
        spd(t, 8 * i + 4, sizes[i]);
    }
}

/* the header's fields of a player's record */
static const uint8_t kept[5] = {2, 3, 4, 0x15, 0x16};

/* the save screen in player 0's window: the notes, a key, the question;
 * a digit names the file (SAVE\0n), the key set's first key gives up.
 * The message's number: 29h saved, 2Ah no file, 28h given up. */
int save_game(fptr palette)
{
    fptr list, hdr;
    unsigned fill = GBO(amok, 0x10), colour = GBO(amok, 0x0D), k, i, j;
    int result = 0x29, h, waiting = 1;

    draw_shop_window(0);
    draw_box(0x14, 0x23, 0x6E, 0x30, 0, (int)fill);
    draw_text(0x1A, 0x2C, 0x1D, (int)colour);
    draw_box(0x23, 0x6E, 0x50, 0x19, 0, (int)fill);
    draw_text(0x2A, 0x78, 0x1E, (int)colour);
    flip_page();
    copy_page();
    t0d36_0c85();
    draw_shop_window(0);
    draw_box(0x25, 0x14, 0x50, 0x14, 0, (int)fill);
    draw_text(0x34, 0x1A, 0x1B, (int)colour);
    draw_box(0x16, 0x3B, 0x6E, 0x30, 0, (int)fill);
    draw_text(0x20, 0x42, 0x1A, (int)colour);
    draw_box(0x25, 0x80, 0x50, 0x1E, 0, (int)fill);
    draw_text(0x29, 0x88, 0x19, (int)colour);
    flip_page();
    copy_page();
    k = 0;
    while (waiting) {
        bi_at("save_key");
        clock_idle();
        k = GB(key_scan);
        if (k >= 2 && k <= 0x0A) {
            k--;
            waiting = 0;
        } else if (k == 0x0B) {
            k = 0;
            waiting = 0;
        }
        if (GB(key_scan) == pb(GFP(key_set), 0))
            return 0x28;
    }
    SB(key_scan, 0);
    if (GW(menu_flags) & 0x10)
        stop_song();
    list = pfp(PLAYER(0), 0x0D);
    t164d_0482(2);
    h = file_open(1, make_path(2, -1, t26ea_000f((long)k, FP(save_name), 2, 4), 5));
    if (!h)
        result = 0x2A;
    else {
        hdr = list;
        spb(hdr, 0, 0);
        spb(hdr, 1, 7);
        spb(hdr, 2, GB(state_248e));
        spb(hdr, 3, GB(state_2497));
        for (i = 0; i < 2; i++) {
            spw(hdr, 4 + 7 * i, pw(PLAYER(i), 0));
            for (j = 0; j < 5; j++)
                spb(hdr, 6 + 7 * i + j, pb(PLAYER(i), kept[j]));
        }
        t26de_0008(h, list, 0x12);
        parts(list, palette);
        for (i = 0; i < PARTS; i++)
            t26de_0008(h, pfp(list, 8 * i), pd(list, 8 * i + 4));
        file_close(h);
    }
    t164d_0482(1);
    draw_window(pw(CURSOR(0), 2), 0);
    draw_marks(pw(CURSOR(0), 2), 0);
    flip_page();
    copy_page();
    if ((GW(menu_flags) & 0x10) && (GW(game_flags) & 0x40))
        play_song(0, 0);
    return result;
}

/* the saved game number_asked loaded over the map that was set up; a
 * file that is not there or not one of these leaves all as it is.  The
 * sound's two switches stay as the menu has them. */
void load_game(fptr palette)
{
    fptr list = pfp(PLAYER(0), 0x0D), hdr = list;
    unsigned i, j, menu;
    int h;

    t164d_0482(2);
    h = file_open(0, make_path(2, -1, t26ea_000f((int16_t)GW(number_asked), FP(save_name), 2, 4), 5));
    if (!h)
        return;
    t264b_0000(h, list, 0x12);
    if (pb(hdr, 0) != 0 || pb(hdr, 1) != 7) {
        file_close(h);
        t164d_0482(1);
        return;
    }
    SB(state_248e, pb(hdr, 2));
    SB(state_2497, pb(hdr, 3));
    for (i = 0; i < 2; i++) {
        spw(PLAYER(i), 0, pw(hdr, 4 + 7 * i));
        for (j = 0; j < 5; j++)
            spb(PLAYER(i), kept[j], pb(hdr, 6 + 7 * i + j));
    }
    menu = GW(menu_flags);
    parts(list, palette);
    for (i = 0; i < PARTS; i++)
        t264b_0000(h, pfp(list, 8 * i), pd(list, 8 * i + 4));
    file_close(h);
    t164d_0482(1);
    if (!(menu & 0x10))
        SW(game_flags, GW(game_flags) & 0xFFBF);
    if (!(menu & 0x20))
        SW(game_flags, GW(game_flags) & 0xFF7F);
}
