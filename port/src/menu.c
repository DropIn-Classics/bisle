/* menu.c - BATTLE.EXE's T1090: the menus before a map (START, OPTIONS,
 * DISK, EXIT and what is below them), the code typed for a map, the
 * scores of a map and the name typed for them.
 *
 * A menu is a record of 6 bytes in `menus` (five items' numbers and their
 * count), an item 2Eh bytes in `menu_items`: +0 a word of flags (1 an
 * action, the byte +2Ch which; 2 the next of its texts; 8 a field for a
 * code; 10h another menu, the byte +2Dh which; 20h not drawn), +2 four
 * texts of 10 bytes in CHAR24.LIB's letters, +2Ah their number (in a
 * field the letters typed), +2Bh the text chosen.
 */
#include <string.h>
#include "bi.h"

#define ITEM 0x2E
/* a text made on the stack in the original: in the megabyte below the
 * program here */
#define TEXT_SCRATCH MKFP(0x0050, 0x0080)

/* T0D36:0C85: waits until fire and the keys are let go, then for fire or
 * a key; 1 when the key is the key set's first (Esc) */
int t0d36_0c85(void)
{
    while (GBO(input0_events, 4) || GW(key_there) == 1) {
        SBO(input0_events, 4, 0);
        SW(key_there, 0);
        t0d36_000f(3);
    }
    SBO(input0_events, 4, 0);
    SW(key_there, 0);
    while (!GBO(input0_events, 4) && !GW(key_there)) {
        SBO(input0_events, 4, 0);
        SW(key_there, 0);
        t0d36_000f(3);
    }
    return GB(key_scan) == pb(GFP(key_set), 0);
}

/* T26EA:000F: the number in decimal at `dest`, at most `digits` of them
 * (the last ones); with flag 4 filled up to `digits` with zeros, or with
 * blanks with flag 1 as well */
fptr t26ea_000f(long value, fptr dest, int digits, int flags)
{
    char tmp[12];
    int count = 0, i;
    unsigned at = 0;
    char pad = (flags & 1) ? ' ' : '0';

    if (value < 0)
        value = -value;
    if (digits > 10)
        digits = 10;
    do {
        tmp[count++] = (char)(value % 10 + '0');
        value /= 10;
    } while (value);
    if (count > digits)
        flags &= ~4;
    if (flags & 4)
        for (i = 0; digits - count > i; i++)
            spb(dest, at++, pad);
    if (count > digits)
        count = digits;
    while (digits-- != 0)
        if (count > 0)
            spb(dest, at++, tmp[--count]);
    spb(dest, at, 0);
    return dest;
}

static fptr item_of(int menu, int sel)
{
    return MKFP(S_menu_items, A_menu_items + ITEM * (int8_t)GBO(menus, 6 * menu + sel));
}

/* the texts of a menu's items, 34 rows apart */
void draw_menu(int menu)
{
    int i, y = 0x32;

    for (i = 0; (int8_t)GBO(menus, 6 * menu + 5) > i; i++, y += 0x22) {
        fptr it = item_of(menu, i);

        if (!(pw(it, 0) & 0x20))
            draw_text24(0x64, y, MKFP(FSEG(it), FOFF(it) + 2 + 10 * (int8_t)pb(it, 0x2B)), 1, 0);
    }
}

/* a field of letters at x, y: the key that came is taken (a letter is
 * added while there are fewer than `most`, backspace takes one away), the
 * text drawn and the sphere after it.  1 a letter, 2 enter, 3 backspace,
 * 0 nothing. */
int edit_text(int x, int y, int most, fptr it)
{
    fptr sphere = pfp(pfp(FP(lib_char24), 0x0E), 0xA0), text = MKFP(FSEG(it), FOFF(it) + 2);
    unsigned ch = GB(key_char), scan = GB(key_scan);
    int result = 0;

    spb(it, 0x2C, 0xFF);
    SB(key_char, 0);
    SB(key_scan, 0);
    if (ch >= 0x61 && ch <= 0x7A) {
        ch = (ch + 0xB0) & 0xFF;
        result = 1;
        spb(it, 0x2C, ch);
    } else if (ch == 0x0D) {
        result = 2;
        spb(it, 0x2C, 0xFF);
    } else if (pb(GFP(key_set), 2) == scan && (int8_t)pb(it, 0x2A) > 0) {
        spb(it, 0x2C, 0xFF);
        spb(it, 0x2A, pb(it, 0x2A) - 1);
        spb(text, (unsigned)(int8_t)pb(it, 0x2A), 0);
        result = 3;
    }
    if (result == 1 && (int8_t)pb(it, 0x2A) < most) {
        spb(text, (unsigned)(int8_t)pb(it, 0x2A), ch);
        spb(it, 0x2A, pb(it, 0x2A) + 1);
        spb(text, (unsigned)(int8_t)pb(it, 0x2A), 0);
    }
    x = draw_text24(x, y, text, 1, 0);
    draw_entry(x, y, sphere, 1, 0, 0);
    return result;
}

/* T1090:110F: the code of map `n` into the code's item, and by the
 * map's players (CODES.DAT's +6) the two PLAYER items and the game's flag
 * 400h (a map against the computer) */
static void show_code(int n, fptr codes)
{
    fptr field = MKFP(S_menu_items, A_menu_items + 2 * ITEM);
    int i;

    for (i = 0; i < 5; i++)
        spb(field, 2 + i, pb(codes, 10 * n + i));
    spb(field, 2 + 5, 0);
    spb(field, 0x2A, 5);
    SBO(menu_items, 3 * ITEM + 0x2B, 0);
    if (pb(codes, 10 * n + 6) == 2) {
        SBO(menu_items, 4 * ITEM + 0x2B, 0);
        SW(game_flags, GW(game_flags) & ~0x400);
    } else {
        SBO(menu_items, 4 * ITEM + 0x2B, 1);
        SW(game_flags, GW(game_flags) | 0x400);
    }
}

/* SELECT POSITION 0 TO 9: the digit into load_position, 0; or 1 for the
 * key set's first key (Esc) */
int ask_position(void)
{
    restore_sprites();
    draw_text24(0x40, 0x32, FP(text_select), 1, 0);
    draw_text24(0x40, 0x54, FP(text_position), 1, 0);
    draw_text24(0x40, 0x76, FP(text_0_to_9), 1, 0);
    flip_page();
    SW(key_there, 0);
    for (;;) {
        unsigned c;

        while (!GW(key_there))
            clock_idle();
        c = GB(key_char);
        SW(key_there, 0);
        if (c >= 0x30 && c <= 0x39) {
            SW(load_position, c - 0x30);
            return 0;
        }
        if (GB(key_scan) == pb(GFP(key_set), 0))
            return 1;
    }
}

/* T1090:1573: a number in five digits of the large letters */
static void draw_number24(int x, int y, long v)
{
    int i;

    for (i = 4; i >= 0; i--) {
        spb(TEXT_SCRATCH, (unsigned)i, v % 10 + 5);
        v /= 10;
    }
    spb(TEXT_SCRATCH, 5, 0);
    draw_text24(x, y, TEXT_SCRATCH, 0, 0);
}

/* the scores' screen: the map's code and the four scores (held within 0
 * and 7EF4h) with their names, the highest first, until a key */
void show_scores(fptr scores, int map, fptr codes)
{
    /* the original's list of the places taken is on the stack and not
     * set before it is read: taken here as holding no place */
    uint8_t order[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    int32_t before = 0x7EF4, best, v;
    int n, i, c, at = 0, unused;

    for (i = 0; i < 4; i++) {
        v = (int32_t)pd(scores, 4 * i);
        if (v < 0)
            v = 0;
        if (v > 0x7EF4)
            v = 0x7EF4;
        spd(scores, 4 * i, (uint32_t)v);
    }
    for (n = 0; n < 4; n++) {
        best = -1;
        for (i = 0; i < 4; i++) {
            v = (int32_t)pd(scores, 4 * i);
            if (v > best && v < before) {
                best = v;
                at = i;
            } else if (v > best && v == before) {
                unused = 1;
                for (c = 0; c < 4; c++)
                    if (order[c] == i)
                        unused = 0;
                if (unused) {
                    best = v;
                    at = i;
                }
            }
        }
        order[n] = (uint8_t)at;
        before = (int32_t)pd(scores, 4 * at);
    }
    restore_sprites();
    flip_page();
    restore_sprites();
    for (i = 0; i < 5; i++)
        spb(TEXT_SCRATCH, 8 + i, pb(codes, 10 * (int8_t)map + i));
    spb(TEXT_SCRATCH, 8 + 5, 0);
    draw_text24(0x6E, 0x16, MKFP(FSEG(TEXT_SCRATCH), FOFF(TEXT_SCRATCH) + 8), 0, 0);
    for (n = 0; n < 4; n++) {
        draw_text24(0xAE, 0x22 * n + 0x42, MKFP(FSEG(scores), FOFF(scores) + 0x10 + 6 * order[n]), 0, 0);
        draw_number24(0x12, 0x22 * n + 0x42, (int32_t)pd(scores, 4 * order[n]));
    }
    flip_page();
    t0d36_0c85();
    flip_page();
    copy_page();
}

/* the scores of map `n`: four of 0 with the name EMPTY, then the map's
 * .HI file over them when there is one (28h bytes: four longs, four
 * names of 6 bytes) */
void load_scores(fptr scores, fptr work, int n)
{
    int i, j;

    for (i = 0; i < 4; i++) {
        spd(scores, 4 * i, 0);
        for (j = 0; j < 6; j++)
            spb(scores, 0x10 + 6 * i + j, GBO(score_empty, j));
    }
    SWO(disk_record, 0, 0);
    load_file(scores, make_path(1, 1, t26ea_000f(n, FP(save_name), 2, 4), 0x0B), work);
    SWO(disk_record, 0, 1);
}

static void ask_effect(int number)
{
    SWO(effects_asked, 0x12, number);
    SWO(effects_asked, 0x14, 0x6E);
    SWO(effects_asked, 0x16, 1);
    effects_start();
}

/* The menus, from the title menu on, until START, EXIT or a game to
 * load is chosen; first, after a map, the name for its scores when the
 * score is among the map's four.  Leaves for the map: who is the
 * computer (the players' flag 2, menu_flags), the limit of turns, HIDE
 * SHOP (menu_flags 8), the map (map_number), the highest score. */
int menu(fptr work)
{
    fptr p = work, scores, name, codes, sphere, path, it;
    int sel = 0, y, menu_now = 3, state, mode, repeat, sound, ncodes, i, j, lowest = 0;
    uint8_t in[5];
    long size;
    int32_t low, v;

    p = hadd(p, 0x2710);
    scores = p;
    p = hadd(p, 0x32);
    name = p;
    p = hadd(p, 0x400);
    codes = p;
    p = hadd(p, 0x2710);
    SW(clip_on, 0);
    SW(clip_x1, 0);
    SW(clip_y1, 0);
    SW(clip_x2, 0x140);
    SW(clip_y2, 0xC8);
    load_picture(p, make_path(1, -1, FP(name_menu), 0x0A), work, NULL, NULL);
    if (load_lib(make_path(1, 0, FP(lib_char24), 3), p, work, FP(lib_char24), 0) == -1)
        return -1;
    p = hadd(p, (long)pd(FP(lib_char24), 0x0A));
    sphere = pfp(pfp(FP(lib_char24), 0x0E), 0xA0);
    size = t2624_0006(codes, make_path(1, -1, FP(name_codes), 5), work, NULL);
    if (size == -1)
        return -1;
    ncodes = (int16_t)size / 10;
    path = make_path(1, -1, FP(name_menu), 8);
    if (!load_effects(p, path, work))
        return -1;
    show_code(GW(map_number), codes);
    flip_page();
    copy_page();
    fade_in();
    if ((int32_t)GD(score_now) > 0) {
        /* a score: among the map's four? */
        low = 0x7EF4;
        load_scores(scores, work, GW(pass_ticks));
        for (i = 0; i < 4; i++) {
            v = (int32_t)pd(scores, 4 * i);
            if (v < low) {
                lowest = i;
                low = v;
            }
        }
        if ((int32_t)GD(score_now) > low) {
            spb(name, 0x2C, 0);
            spb(name, 0x2A, 0);
            spb(name, 2, 0);
            for (state = 1; state;) {
                bi_at("name_pass");
                restore_sprites();
                draw_text24(0x3A, 0x14, FP(text_type_name), 1, 0);
                draw_text24(0x80, 0x38, FP(text_for_top), 1, 0);
                draw_text24(0x46, 0x5C, FP(text_four), 1, 0);
                if (edit_text(0x66, 0xA4, 5, name) == 2)
                    state = 0;
                flip_page();
            }
            for (i = 0; i < 5; i++)
                spb(scores, 0x10 + 6 * lowest + i, pb(name, 2 + i));
            spd(scores, 4 * lowest, GD(score_now));
            /* the path is the one load_scores made */
            save_file(FP(path_made), scores, 0x28);
            show_scores(scores, GB(pass_ticks), codes);
        }
    }
    load_scores(scores, work, GW(map_number));
    ask_effect(3);
    y = 0x22 * sel + 0x32;
    restore_sprites();
    draw_menu(menu_now);
    draw_entry(0x42, y, sphere, 1, 0, 0);
    flip_page();
    do {
        SBO(input0_events, 4, 0);
        t0d36_000f(5);
    } while (GBO(input0_events, 4));
    SW(key_there, 0);
    SB(key_scan, 0);
    SB(key_char, 0);
    mode = 1;
    repeat = 0;
    state = 0;
    while (state != 1) {
        bi_at("menu_pass");
        sound = 0xFF;
        y = 0x22 * sel + 0x32;
        /* up, down and fire, from the first player's input or the keys */
        for (i = 2; i < 5; i++) {
            in[i] = GBO(input0_events, i);
            SBO(input0_events, i, 0);
        }
        if (GW(key_there)) {
            if (pb(GFP(key_set), 8) == GB(key_scan))
                in[2] = 1;
            if (pb(GFP(key_set), 9) == GB(key_scan))
                in[3] = 1;
            if (pb(GFP(key_set), 6) == GB(key_scan))
                in[4] = 1;
            SW(key_there, 0);
        }
        restore_sprites();
        draw_menu(menu_now);
        draw_entry(0x42, y, sphere, 1, 0, 0);
        if (mode == 1) {
            /* a key held is taken again after two passes */
            if ((in[2] || in[3] || in[4]) && state == 2 && repeat < 2)
                repeat++;
            else {
                state = 0;
                repeat = 0;
            }
            if (in[4] && state != 2) {
                unsigned flags;

                it = item_of(menu_now, sel);
                flags = pw(it, 0);
                state = 2;
                if (flags & 1) {
                    switch ((int8_t)pb(it, 0x2C)) {
                    case 2:                     /* LOAD */
                        if (ask_position()) {
                            restore_sprites();
                            draw_menu(menu_now);
                            draw_entry(0x42, y, sphere, 1, 0, 0);
                            state = 2;
                            sound = 2;
                            break;
                        }
                        restore_sprites();
                        draw_text24(0x57, 0x37, FP(text_please), 1, 0);
                        draw_text24(0x57, 0x5B, FP(text_insert), 1, 0);
                        draw_text24(0x57, 0x7F, FP(text_disk), 1, 0);
                        flip_page();
                        t0d36_0c85();
                        path = make_path(2, -1, t26ea_000f((int16_t)GW(load_position), FP(save_name), 2, 4), 5);
                        SWO(disk_record, 0, 0);
                        i = file_open(0, path);
                        if (i) {
                            state = 1;
                            SW(game_flags, GW(game_flags) | 0x200);
                            file_close(i);
                        } else {
                            restore_sprites();
                            draw_menu(menu_now);
                            draw_entry(0x42, y, sphere, 1, 0, 0);
                            state = 2;
                            sound = 2;
                        }
                        SWO(disk_record, 0, 1);
                        break;
                    case 4:                     /* EXIT */
                        SW(game_flags, GW(game_flags) | 0x100);
                        SW(game_flags, GW(game_flags) & ~3);
                        state = 1;
                        sound = 1;
                        break;
                    case 0x10:                  /* RATING */
                        show_scores(scores, GB(map_number), codes);
                        restore_sprites();
                        draw_menu(menu_now);
                        draw_entry(0x42, y, sphere, 1, 0, 0);
                        state = 2;
                        sound = 1;
                        break;
                    default:                    /* 8: START */
                        state = 1;
                        sound = 1;
                        break;
                    }
                } else if (flags & 0x10) {
                    menu_now = (int8_t)pb(it, 0x2D);
                    sel = 0;
                    sound = 0;
                } else if (flags & 2) {
                    int number = (int8_t)GBO(menus, 6 * menu_now + sel);

                    spb(it, 0x2B, pb(it, 0x2B) + 1);
                    if ((int8_t)pb(it, 0x2B) >= (int8_t)pb(it, 0x2A))
                        spb(it, 0x2B, 0);
                    sound = 1;
                    /* who plays is the map's unless it is one against
                     * the computer */
                    if ((number == 3 || number == 4) && !(GW(game_flags) & 0x400)) {
                        spb(it, 0x2B, 0);
                        sound = 2;
                    }
                } else if (flags & 8) {
                    spw(it, 0, pw(it, 0) | 0x20);
                    spb(it, 0x2A, 0);
                    spb(it, 2, 0);
                    mode = 0;
                    SB(key_char, 0);
                    SB(key_scan, 0);
                    sound = 1;
                }
            } else {
                if (in[2] && state != 2) {
                    sel--;
                    state = 2;
                    if (sel < 0)
                        sel = 0;
                    sound = 0;
                }
                if (in[3] && state != 2) {
                    sel++;
                    state = 2;
                    if ((int8_t)GBO(menus, 6 * menu_now + 5) <= sel)
                        sel = (int8_t)GBO(menus, 6 * menu_now + 5) - 1;
                    sound = 0;
                }
            }
        } else {
            /* a code is typed */
            int r;

            it = item_of(menu_now, sel);
            r = edit_text(0x64, y, 5, it);
            if (r == 2) {
                spw(it, 0, pw(it, 0) & ~0x20);
                mode = 0;
                for (i = 0; i < ncodes; i++) {
                    for (j = 0; j < 5; j++)
                        if (pb(codes, 10 * i + j) != pb(it, 2 + j))
                            break;
                    if (j == 5) {
                        mode = 1;
                        break;
                    }
                }
                if (mode) {
                    show_code(i, codes);
                    SW(map_number, i);
                    load_scores(scores, work, GW(map_number));
                    sound = 1;
                } else {
                    show_code(GW(map_number), codes);
                    sound = 2;
                }
                mode = 1;
            } else if (r == 1 || r == 3) {
                mode = 3;
                sound = 0;
            }
        }
        if (sound != 0xFF)
            ask_effect(sound);
        flip_page();
        /* the mouse's speed by the item SLOW/MEDIUM (T268A:0006): the
         * port has no mouse */
        if (mode == 3) {
            mode = 0;
            t0d36_000f(3);
        }
    }
    /* what the menus leave for the map */
    SW(menu_flags, GW(menu_flags) & ~7);
    if ((int8_t)GBO(menu_items, 3 * ITEM + 0x2B) > 0) {
        SWO(players, 0, GWO(players, 0) | 2);
        SW(menu_flags, GW(menu_flags) | 1);
    } else
        SWO(players, 0, GWO(players, 0) & ~2);
    if ((int8_t)GBO(menu_items, 4 * ITEM + 0x2B) > 0) {
        SWO(players, 0x17, GWO(players, 0x17) | 2);
        SW(menu_flags, GW(menu_flags) | 1);
    } else
        SWO(players, 0x17, GWO(players, 0x17) & ~2);
    if ((GWO(players, 0) & 2) && (GWO(players, 0x17) & 2)) {
        SW(menu_flags, GW(menu_flags) | 4);
        SW(menu_flags, GW(menu_flags) & ~1);
    }
    if (!(GWO(players, 0) & 2) && !(GWO(players, 0x17) & 2)) {
        SW(menu_flags, GW(menu_flags) | 2);
        SW(menu_flags, GW(menu_flags) & ~1);
    }
    switch ((int8_t)GBO(menu_items, 12 * ITEM + 0x2B)) {
    case 1: i = 4; break;
    case 2: i = 8; break;
    case 3: i = 0x10; break;
    default: i = 0xFF; break;
    }
    SBO(players, 0x17 + 0x16, i);
    SBO(players, 0x16, i);
    if ((int8_t)GBO(menu_items, 13 * ITEM + 0x2B) > 0)
        SW(menu_flags, GW(menu_flags) | 8);
    else
        SW(menu_flags, GW(menu_flags) & ~8);
    v = -1;
    for (i = 0; i < 4; i++)
        if ((int32_t)pd(scores, 4 * i) > v)
            v = (int32_t)pd(scores, 4 * i);
    SD(score_best, (uint32_t)v);
    fade_out();
    restore_sprites();
    SW(draw_colour, 0);
    clear_page();
    flip_page();
    restore_sprites();
    SW(draw_colour, 0);
    clear_page();
    return 0;
}
