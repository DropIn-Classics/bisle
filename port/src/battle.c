/* battle.c - BATTLE.EXE's T0708: the program: the start, the title and
 * the menus, a map's setup, the map's loop, what follows a map.
 *
 * The loop makes a pass every pass_ticks ticks (4: 18.2 a second).  In a
 * pass each player's input is taken (the computer's from computer_step)
 * and his cursor's state carried on: on the map (cursor.c), the overview,
 * the status and the unit's screen, a building's screen with its list of
 * types, the change of phase asked for, the map's end.  Then the timers
 * of the loop, the two cursors, the sounds asked for, and the pages
 * change.
 */
#include <stdio.h>
#include <string.h>
#include "bi.h"

/* the computer's input record for a pass (on the stack in the original):
 * in the megabyte below the program */
#define COMPUTER_INPUT MKFP(0x0050, 0x00C0)

/* T0D36:1592: the keyboard and the timers put back, a message, the end */
void fatal_error(int number)
{
    char text[64];

    t2354_056f();
    t2354_005e();
    t261e_0008();
    snprintf(text, sizeof text, "ERROR: %s", fstr(FP(fatal_messages)) + 0x14 * (number - 1));
    bi_fatal(text);
}

static fptr player_rec(int n) { return MKFP(S_players, A_players + 0x17 * n); }

static void message_flag(int side)
{
    SW(game_flags2, GW(game_flags2) | (side ? 0x10 : 8));
}

static void ask_sound(int number)
{
    SW(game_flags2, GW(game_flags2) | 0x100);
    SW(number_asked, number);
}

/* a library of LIB\ loaded at *p; *p moved behind it unless `keep` is 0 */
static void lib(fptr record, fptr *p, fptr work, int sorted, int keep)
{
    if (load_lib(make_path(1, 0, record, 3), *p, work, record, sorted) == -1)
        fatal_error(2);
    if (keep)
        *p = hadd(*p, (long)pd(record, 0x0A));
}

/* the song and the effects of the game's files, the song's buffer of `size`
 * bytes first */
static void songs(fptr *p, fptr work, long size)
{
    load_song(*p, make_path(1, -1, FP(name_game), 7), work);
    *p = hadd(*p, size);
    if (!load_effects(*p, make_path(1, -1, FP(name_game), 8), work))
        fatal_error(2);
}

/* MOON.EXE's T0DA1:16AE: the ground parts of a map.  The library of parts
 * is loaded at `p`, the map's file and STATMAP.FIN behind it, and only the
 * parts some square of the two files has (the first of each at most 101:
 * a count past 100 ends it) are stored behind the pages; the table of far
 * pointers to them, in the order of the library, goes to the work buffer
 * and from there to `parts_table`, which takes the library's place.  The
 * squares are the bytes at every second place of the two files from their
 * first byte (the four bytes of the header among them).  Returns how many
 * parts it stored, -1 when a file cannot be read. */
static int moon_parts(fptr p, fptr work)
{
    fptr rec = FP(lib_part), name, map, tbl;
    long size, size2, size3;
    int n, sq, di, si, count = 0;

    fade_in();
    name = t26ea_000f((int16_t)GW(map_number), FP(save_name), 2, 4);
    size = load_lib(make_path(1, 0, rec, 3), p, work, rec, 0);
    if (size == -1)
        return -1;
    map = hadd(p, size);
    size2 = t2624_0006(map, make_path(1, 1, name, 0), work, NULL);
    if (size2 == -1)
        return -1;
    size3 = t2624_0006(hadd(map, size2), make_path(1, 1, FP(name_statmap), 0), work, NULL);
    if (size3 == -1)
        return -1;
    SWO(parts_next, 0, 0);
    n = (int16_t)pw(rec, 0x12);
    tbl = pfp(rec, 0x0E);
    sq = (int16_t)((size2 + size3 - 4) >> 1);
    for (di = 0; di < n; di++)
        for (si = 0; si < sq; si++)
            if (pb(map, (unsigned)(2 * si)) == di) {
                spfp(work, 4 * (unsigned)di, store_part(pfp(tbl, 4 * (unsigned)di)));
                if (++count > 0x64)
                    di = n;
                break;
            }
    t0d36_15e9(work, FP(parts_table), (uint32_t)(4L * n));
    spfp(rec, 0x0E, FP(parts_table));
    return count;
}

/* what a map needs: its buffers cut from the program's block, the
 * libraries, the picture and palette, the tables, the map's files, the
 * cursors.  The buffers the loop needs go to *palette (also the saved
 * game's), *orders, *cursor_lib, *pmp and *spare. */
static void map_setup(fptr work0, fptr *palette, fptr *orders, fptr *cursor_lib, fptr *pmp, fptr *spare)
{
    fptr p = work0, work = work0, name, fight = FP(fight_record), table;
    int i, n;

    SW(clip_on, 1);
    SW(clip_x1, 0);
    SW(clip_y1, 0);
    SW(clip_x2, 0x140);
    SW(clip_y2, 0xB3);
    SB(factories_made, 0);
    SB(depots_made, 0);
    SB(cargo_made, 0);
    SB(units_made, 0);
    *palette = p;
    p = hadd(p, 0x320);
    SFP(buffer_248f, p);
    p = hadd(p, 0x82);
    SFP(buffer_2493, p);
    p = hadd(p, 0x82);
    SFP(map0, p);
    p = hadd(p, 0x2008);
    SFP(map1, p);
    p = hadd(p, 0x2008);
    SFP(game_txt, p);
    p = hadd(p, 0x3E8);
    *spare = p;
    if (bi_prog == BI_MOON) {
        SW(parts_stored, (uint16_t)moon_parts(p, work));
        if ((int16_t)GW(parts_stored) < 0)
            fatal_error(2);
    } else {
        /* the ground: each part stored behind the pages, the table of where
         * they are takes the library's place */
        lib(FP(lib_part), &p, work, 1, 0);
        SWO(parts_next, 0, 0);
        table = pfp(FP(lib_part), 0x0E);
        n = (int16_t)pw(FP(lib_part), 0x12);
        for (i = 0; n > i; i++)
            spfp(table, 4 * (unsigned)i, store_part(pfp(table, 4 * (unsigned)i)));
        t0d36_15e9(table, p, (uint32_t)((int32_t)n << 2));
        spfp(FP(lib_part), 0x0E, p);
        p = hadd(p, (int32_t)n << 2);
    }
    lib(FP(lib_unit), &p, work, 1, 1);
    lib(FP(lib_exp), &p, work, 0, 1);
    lib(FP(lib_patt), &p, work, 0, 1);
    spfp(player_rec(1), 0x11, p);
    p = hadd(p, 0xF2);
    spfp(player_rec(0), 0x11, p);
    p = hadd(p, 0xF2);
    *orders = p;
    work = p;
    spfp(player_rec(0), 0x0D, p);
    p = hadd(p, 0x2710);
    spfp(player_rec(1), 0x0D, p);
    p = hadd(p, 0x2710);
    spfp(player_rec(0), 9, p);
    p = hadd(p, 0x3E8);
    spfp(player_rec(1), 9, p);
    p = hadd(p, 0x3E8);
    if (bi_prog == BI_MOON) {
        /* the buffers of the overview's two files (MAPINFO.DAT and MAP02.DAT or MAP04.DAT) */
        SFP(mapinfo_data, p);
        p = hadd(p, 0x2BC);
        SFP(overview_data, p);
        p = hadd(p, 0x898);
    }
    /* where what each cursor covers is kept: behind the pages */
    spfp(player_rec(0), 5, MKFP(0xA7E8, 4));
    spfp(player_rec(1), 5, MKFP(0xA7D0, 0x300));
    load_picture(p, make_path(1, -1, FP(name_game), 0x0A), work, NULL, NULL);
    if (bi_prog == BI_MOON) {
        /* MOON blacks the screen out before the palette's file is read */
        for (i = 0; i < 0x300; i++)
            SBO(picture_palette, i, 0);
        set_palette(0xFF, FP(picture_palette));
    }
    /* the palette chosen in the menu (/m: the third) */
    name = t26ea_000f((int8_t)GBO(menu_items, 11 * 0x2E + 0x2B), FP(save_name), 2, 4);
    if (!load_file(*palette, make_path(1, -1, name, 4), work))
        fatal_error(2);
    for (i = 0; i < 0x300; i++)
        SBO(picture_palette, i, pb(*palette, (unsigned)i));
    if (!load_file(GFP(game_txt), make_path(1, -1, FP(name_game), 6), work))
        fatal_error(2);
    *pmp = p;
    if (bi_prog != BI_MOON)
        p = hadd(p, 0x4650);
    *cursor_lib = p;
    lib(FP(lib_cursor), &p, work, 0, 1);
    lib(FP(lib_bigunit), &p, work, 1, 1);
    lib(FP(lib_shop), &p, work, 0, 1);
    if (bi_prog != BI_MOON)
        songs(&p, work, 0xA028);
    if (!load_file(FP(amok), make_path(1, -1, FP(name_amok), 5), work))
        fatal_error(2);
    if (!load_file(FP(unit_types), make_path(1, -1, FP(name_unit), 5), work))
        fatal_error(2);
    if (!load_file(FP(ground), make_path(1, -1, FP(name_ground), 5), work))
        fatal_error(2);
    name = t26ea_000f((int16_t)GW(map_number), FP(save_name), 2, 4);
    if (load_fin(make_path(1, 1, name, 0), GFP(map0), work))
        fatal_error(2);
    if (load_shp(make_path(1, 1, name, 2), pfp(player_rec(0), 9), work))
        fatal_error(2);
    if (GW(game_flags) & 0x400) {
        /* the computer's table for the map, its records' second and third
         * bytes exchanged */
        if (!load_file(FP(computer_types), make_path(1, 1, name, 9), work))
            fatal_error(2);
        /* MOON.EXE's loader has no such loop (MOON.ASM, behind the load of
         * the .COM file); its records stay as the file has them */
        for (i = 0; bi_prog != BI_MOON && i < 0x1B; i++) {
            unsigned b = GBO(computer_types, 6 * i + 3);

            SBO(computer_types, 6 * i + 3, GBO(computer_types, 6 * i + 2));
            SBO(computer_types, 6 * i + 2, b);
        }
    }
    if (bi_prog == BI_MOON)
        songs(&p, work, 0x4E20);   /* MOON reads them last, of other sizes */
    clear_marks(0xFF);
    t0e9b_000b();
    SD(passes, 0);
    SW(round, 0);
    SW(game_flags2, 0);
    SB(state_2592, 0);
    SB(window_last_column, (int16_t)(GW(map_width) - 10) >> 1);
    SB(window_last_row, (int16_t)(GW(map_height) - 8) >> 1);
    SW(pass_ticks, 4);
    SD(score_now, (uint32_t)score());
    for (i = 0; i < 0x0F; i++) {
        spd(FP(timer_dues), 4 * (unsigned)i, 0);
        SBO(timer_kinds, i, 0);
        SWO(timer_args, 2 * i, 0);
    }
    spb(player_rec(1), 0x15, 0);
    spb(player_rec(0), 0x15, 0);
    spw(player_rec(0), 0, pw(player_rec(0), 0) & 0xFFFA);
    spw(player_rec(1), 0, pw(player_rec(1), 0) & 0xFFFA);
    if (pw(player_rec(0), 0) & 2)
        computer_start(0);
    if (pw(player_rec(1), 0) & 2)
        computer_start(1);
    t0d36_0bb6(CURSOR(0));
    t0d36_0bb6(CURSOR(1));
    /* each cursor on the square below its headquarters */
    t0e9b_15dd(0, GWO(hqs, 0x0E) + (GW(map_width) << 1));
    t0e9b_15dd(1, GWO(hqs, 0x1C + 0x0E) + (GW(map_width) << 1));
    spb(CURSOR(0), 0x16, 1);
    spb(CURSOR(1), 0x16, 2);
    SB(state_2497, 0);
    SB(state_248e, 0);
    for (i = 0; i < 2; i++) {
        fptr in = i ? FP(input1_events) : FP(input0_events);

        spb(in, 4, 0), spb(in, 8, 0), spb(in, 5, 0), spb(in, 3, 0), spb(in, 2, 0), spb(in, 1, 0), spb(in, 0, 0);
    }
    for (i = 0; i < 4; i++)
        SBO(explosions, 7 * i + 2, 0xFF);
    /* the fight scene's record: the units' sprites and the routines it
     * calls through pointers */
    spfp(fight, 0x32, pfp(FP(lib_unit), 0x0E));
    spfp(fight, 0x4E, FP(effects_start));
    spfp(fight, 0x52, FP(restore_sprites));
    spfp(fight, 0x56, FP(draw_entry));
    spfp(fight, 0x5A, FP(draw_packed));
    spfp(fight, 0x5E, FP(srand));
    spfp(fight, 0x62, FP(random));
    spfp(fight, 0x66, FP(abs_value));
    spfp(fight, 0x4A, GFP(effects_ptr));
    spd(fight, 0x26, 0);
    SW(menu_flags, GW(menu_flags) | 0x10);
    SW(menu_flags, GW(menu_flags) | 0x20);
    SW(game_flags, GW(game_flags) | 0x40);
    SW(game_flags, GW(game_flags) | 0x80);
    SW(path_count, 0);
    SB(remove_depth, 0);
    if (GW(game_flags) & 0x200) {
        /* a saved game */
        t262a_000e(pad_box_text(FP(text_insert_save)));
        load_game(*palette);
        SW(game_flags, GW(game_flags) & ~0x200);
    }
    /* the overview's picture; MOON.EXE has no such file (MOON.ASM: no call
     * of make_path with the extension 1 in its map setup) and draws its
     * overview otherwise (not read) */
    if (bi_prog != BI_MOON) {
        name = t26ea_000f((int16_t)GW(map_number), FP(save_name), 2, 4);
        if (!load_file(*pmp, make_path(1, 1, name, 1), work))
            fatal_error(2);
    } else {
        /* what MOON draws its overview from: MAPINFO.DAT and, by the map's
         * size, MAP02.DAT or MAP04.DAT (what they hold: not read) */
        if (!load_file(GFP(mapinfo_data), make_path(1, -1, FP(name_mapinfo), -1), work))
            fatal_error(2);
        if ((int16_t)GW(map_width) > 0x20 || (int16_t)GW(map_height) > 0x28) {
            SW(overview_scale, 1);
            name = FP(name_map02);
        } else {
            SW(overview_scale, 2);
            name = FP(name_map04);
        }
        if (!load_file(GFP(overview_data), make_path(1, -1, name, -1), work))
            fatal_error(2);
    }
    draw_window(pw(CURSOR(0), 2), 0);
    draw_marks(pw(CURSOR(0), 2), 0);
    draw_window(pw(CURSOR(1), 2), 1);
    draw_marks(pw(CURSOR(1), 2), 1);
    show_message(-1, 0);
    show_message(-1, 1);
    SW(game_flags, GW(game_flags) & ~0x0C);
    SW(key_there, 0);
    SB(key_scan, 0);
    SB(key_char, 0);
    flip_page();
    copy_page();
    fade_in();
    if (GW(game_flags) & 0x40)
        play_song(0, 0);
    SB(loop_player, 0);
    SW(game_flags, GW(game_flags) | 2);
}

/* a message for a second, and no other key's message meanwhile */
static void key_message(int number, int side, int passes_shown)
{
    show_message(number, side);
    timer_set(side ? 2 : 1, passes_shown, 0);
    SW(game_flags2, GW(game_flags2) | 0x200);
    timer_set(6, 0x0A, 0);
}

/* the keys that are not a player's: the version, the music and the
 * effects on and off, the joysticks, the mouse, Esc */
/* QUIT THE GAME ? (Y) and CANCEL : PRESS BUTTON (messages 0Ch, 0Dh) with
 * the controller's buttons for Y and fire named, when a controller is in
 * use and has both (the port's; the original names the keys only): 1 when
 * shown */
static int quit_for_pad(fptr ks)
{
    const char *yes = bi_pad_name(pb(ks, 0x18)), *no = bi_pad_name(0x1C);
    char text[32];

    if (!yes && bi_quit_yz)
        yes = bi_pad_name(0x15) ? bi_pad_name(0x15) : bi_pad_name(0x2C);
    if (!no)
        no = bi_pad_name(pb(ks, 1));
    if (!yes || !no)
        return 0;
    snprintf(text, sizeof text, "QUIT THE GAME ? (%s)", yes);
    show_text(text, 0);
    snprintf(text, sizeof text, "CANCEL : PRESS %s", no);
    show_text(text, 1);
    return 1;
}

static void loop_keys(unsigned key)
{
    fptr ks = GFP(key_set);

    if (pb(ks, 0x0F) == key) {
        show_message(0x27, 0);
        draw_number(GBO(version, 1), 0x63, 0xBD);
        draw_number(GBO(version, 0), 0x79, 0xBD);
        timer_set(1, 5, 0);
    } else if (pb(ks, 0x0D) == key) {
        if ((GW(menu_flags) & 0x10) && !(GW(game_flags2) & 0x200)) {
            if (GW(game_flags) & 0x40) {
                stop_song();
                SW(game_flags, GW(game_flags) & ~0x40);
                key_message(0x24, 0, 5);
            } else {
                play_song(0, 0);
                SW(game_flags, GW(game_flags) | 0x40);
                key_message(0x23, 0, 5);
            }
        }
    } else if (pb(ks, 0x0E) == key) {
        if ((GW(menu_flags) & 0x20) && !(GW(game_flags2) & 0x200)) {
            if (GW(game_flags) & 0x80) {
                SW(game_flags, GW(game_flags) & ~0x80);
                key_message(0x26, 0, 5);
            } else {
                SW(game_flags, GW(game_flags) | 0x80);
                key_message(0x25, 0, 5);
            }
        }
    } else if (pb(ks, 0x10) == key || pb(ks, 0x11) == key) {
        int side = pb(ks, 0x10) == key ? 0 : 1;

        /* a player's joystick on or off (T265E:000E looks for them
         * again: the port has none) */
        if (!(GW(game_flags2) & 0x200)) {
            int on;

            if (side) {
                on = !GW(joy1_used);
                SW(joy1_used, on ? 0xFFFF : 0);
            } else {
                on = !GW(joy0_used);
                SW(joy0_used, on ? 0xFFFF : 0);
            }
            t265e_000e();
            key_message(0x33 + 2 * side + on, side, 7);
        }
    } else if (pb(ks, 0x12) == key) {
        /* the mouse on or off: the port has none, so "on" finds none */
        if (!(GW(game_flags2) & 0x200))
            key_message(GW(mouse_on) ? 0x38 : 0x37, 0, 7);
    } else if (pb(ks, 0x13) == key) {
        /* the mouse's speed, only with a mouse */
    } else if (pb(ks, 0) == key) {
        if (GW(game_flags2) & 0x200)
            return;
        /* QUIT THE GAME ? (Y): Y leaves the map, fire or space goes on */
        if (!quit_for_pad(ks)) {
            show_message(0x0C, 0);
            show_message(0x0D, 1);
        }
        flip_page();
        copy_page();
        for (;;) {
            /* bi_quit_yz: the letters Y and Z, the keys 15h and 2Ch, whichever of
             * them the key set and the message name, as the layout has them */
            if (pb(ks, 0x18) == GB(key_scan) || (bi_quit_yz && (GB(key_scan) == 0x15 || GB(key_scan) == 0x2C))) {
                SW(game_flags, GW(game_flags) & ~2);
                SW(game_flags, GW(game_flags) | 1);
                SD(score_now, 0);
                break;
            }
            if (GBO(input0_events, 4) || GBO(input1_events, 4) || pb(ks, 1) == GB(key_scan)) {
                timer_set(1, 5, 0);
                timer_set(2, 5, 0);
                SBO(input1_events, 4, 0);
                SBO(input0_events, 4, 0);
                break;
            }
            clock_idle();
        }
    }
}

/* the line of the unit under a cursor at rest, and in the attack phase
 * the aim of its order marked for a moment */
static void loop_unit_line(fptr cur, fptr map, int side)
{
    unsigned b = pb(map, pw(cur, 0) + 1), f, aim;
    fptr u;

    if (b > 0xF0)
        return;
    u = UNIT(b);
    if (pw(u, 6) & 4)
        return;
    if ((pw(u, 6) & 2) && ((side & 1) ? !(pw(u, 4) & 1) : (pw(u, 4) & 1)))
        return;
    if (GW(game_flags) & (side ? 8 : 4))
        return;
    f = pw(u, 4);
    if ((f & 0x40) && !(f & 0x80))
        b = (b - 1) & 0xFF;
    u = UNIT(b);
    aim = pw(u, 0x11);
    if ((pb(cur, 0x16) & 2) && aim && same_side(side, pw(u, 4))) {
        int w = (int16_t)GW(map_width), sq = (int16_t)aim >> 1;
        unsigned index = (uint16_t)(sq % w + ((sq / w) << 6)), old;

        timer_set(5, 1, (int)aim);
        old = GBO(marks, index);
        SBO(marks, index, old | (side ? 8 : 4));
        redraw_square(side, pw(u, 0x11));
        SBO(marks, index, old);
    }
    unit_line((int)b, side);
    timer_set(side ? 2 : 1, 5, 0);
}

/* the cursor's step on the map by the directions: a column left or
 * right (with a row for the odd columns' half step), a row up or down;
 * at the window's edge the window moves by two */
static void loop_cursor_move(fptr cur, int bits, int side)
{
    int w4 = GW(map_width) << 2, count = 1;
    unsigned moved = 0;

    if (bits & (4 | 8)) {
        if (pw(cur, 0x0C) & 1) {
            if (bits & 2) {
                moved |= 2;
                spw(cur, 0x0E, pw(cur, 0x0E) + 1);
            }
        } else if (bits & 1) {
            moved |= 1;
            spw(cur, 0x0E, pw(cur, 0x0E) - 1);
        }
        if (bits & 4) {
            moved |= 4;
            spw(cur, 0x0C, pw(cur, 0x0C) - 1);
        } else {
            moved |= 8;
            spw(cur, 0x0C, pw(cur, 0x0C) + 1);
        }
    } else if (bits & 1) {
        moved |= 1;
        spw(cur, 0x0E, pw(cur, 0x0E) - 1);
    } else if (bits & 2) {
        moved |= 2;
        spw(cur, 0x0E, pw(cur, 0x0E) + 1);
    }
    if ((int16_t)pw(cur, 0x0E) < 1) {
        if ((int8_t)pb(cur, 0x15) > 0) {
            SW(game_flags2, GW(game_flags2) | 2);
            spw(cur, 2, pw(cur, 2) - (unsigned)w4);
            spb(cur, 0x15, pb(cur, 0x15) - 1);
            spw(cur, 0x0E, pw(cur, 0x0E) + 1);
            count = 2;
        }
        if ((int16_t)pw(cur, 0x0E) < 0) {
            moved &= ~1u;
            spw(cur, 0x0E, 0);
        }
    } else if ((int16_t)pw(cur, 0x0E) >= 7) {
        if (pb(cur, 0x15) < GB(window_last_row)) {
            SW(game_flags2, GW(game_flags2) | 2);
            spw(cur, 2, pw(cur, 2) + (unsigned)w4);
            spb(cur, 0x15, pb(cur, 0x15) + 1);
            spw(cur, 0x0E, pw(cur, 0x0E) - 1);
            count = 2;
        }
        if ((int16_t)pw(cur, 0x0E) > 6) {
            moved &= ~2u;
            spw(cur, 0x0E, 6);
        }
    }
    if ((int16_t)pw(cur, 0x0C) < 1) {
        if ((int8_t)pb(cur, 0x14) > 0) {
            SW(game_flags2, GW(game_flags2) | 2);
            spw(cur, 2, pw(cur, 2) - 4);
            spb(cur, 0x14, pb(cur, 0x14) - 1);
            spw(cur, 0x0C, pw(cur, 0x0C) + 1);
            count = 2;
        }
        if ((int16_t)pw(cur, 0x0C) < 0) {
            moved &= ~4u;
            spw(cur, 0x0C, 0);
        }
    } else if ((int16_t)pw(cur, 0x0C) >= 9) {
        if (pb(cur, 0x14) < GB(window_last_column)) {
            SW(game_flags2, GW(game_flags2) | 2);
            spw(cur, 2, pw(cur, 2) + 4);
            spb(cur, 0x14, pb(cur, 0x14) + 1);
            spw(cur, 0x0C, pw(cur, 0x0C) - 1);
            count = 2;
        }
        if ((int16_t)pw(cur, 0x0C) > 8) {
            moved &= ~8u;
            spw(cur, 0x0C, 8);
        }
    }
    if (moved && (GW(game_flags) & 0x80)) {
        int ch = side ? 1 : 3;

        SWO(effects_asked, 6 * ch, side ? 1 : 0);
        SWO(effects_asked, 6 * ch + 2, 0x4E);
        SWO(effects_asked, 6 * ch + 4, count);
    }
    redraw_cursor_square(cur, side);
    t0d36_0044(cur, side);
    if (GW(game_flags2) & 2) {
        draw_window(pw(cur, 2), side);
        draw_marks(pw(cur, 2), side);
        SW(game_flags2, GW(game_flags2) & ~2);
    }
}

/* the overview over a player's window (the cursor's state 3): it opens
 * with a frame for the cursor over the part the window shows, the
 * directions move the frame by two squares, fire puts the window there */
static void loop_overview(fptr cur, fptr in, int bits, int side, fptr pmp)
{
    fptr pl = player_rec(side);
    /* MOON's overview has 4 pixels a square on the small maps (scale 2),
     * BATTLE's 2 */
    int sc = bi_prog == BI_MOON ? (int16_t)GW(overview_scale) : 1;
    int sq = sc == 2 ? 2 : 1;

    if (pb(cur, 0x18) == 4) {
        spw(cur, 0x10, ((side & 1) ? 0xA0 : 0) + 0x50 - (sq * GW(map_width) + 2) - 3);
        spw(cur, 4, pw(cur, 0x10));
        spw(cur, 0x12, 0x64 - (sq * GW(map_height) + 2) - 4);
        spw(cur, 6, pw(cur, 0x12));
        spw(cur, 8, pw(cur, 0x0C));
        spw(cur, 0x0A, pw(cur, 0x0E));
        spw(cur, 0x0C, (int8_t)pb(cur, 0x14) << 1);
        spw(cur, 0x0E, (int8_t)pb(cur, 0x15) << 1);
        spw(cur, 0x10, pw(cur, 0x10) + (pw(cur, 0x0C) << sc));
        spw(cur, 0x12, pw(cur, 0x12) + (pw(cur, 0x0E) << sc));
        draw_shop_window(side);
        draw_overview(side, (int16_t)pw(cur, 4), (int16_t)pw(cur, 6), pmp);
        spw(pl, 0, pw(pl, 0) | 4);
        ask_sound(2);
        spb(cur, 0x2A, 1), spb(cur, 0x29, 1), spb(cur, 0x28, 1);
        spb(cur, 0x18, 5);
        return;
    }
    if (bits & 0x10) {
        spb(cur, 0x17, 0);
        spb(cur, 0x18, 0x19);
        spb(cur, 0x1B, 5);
        spb(cur, 0x14, (int16_t)pw(cur, 0x0C) >> 1);
        spb(cur, 0x15, (int16_t)pw(cur, 0x0E) >> 1);
        spw(cur, 2, (pw(cur, 0x0C) << 1) + (pw(cur, 0x0E) * GW(map_width) << 1));
        t24d3_0000(pfp(pl, 5));
        spw(pl, 0, pw(pl, 0) & ~4);
        draw_window(pw(cur, 2), side);
        draw_marks(pw(cur, 2), side);
        spw(cur, 4, pw(cur, 8));
        spw(cur, 0x0C, pw(cur, 8));
        spw(cur, 6, pw(cur, 0x0A));
        spw(cur, 0x0E, pw(cur, 0x0A));
        t0d36_0044(cur, side);
        spb(cur, 0x28, 1), spb(cur, 0x29, 2), spb(cur, 0x2A, 1);
        spb(in, 8, 0);
        return;
    }
    if (bits & 1) {
        if ((int16_t)pw(cur, 0x0E) > 0) {
            spw(cur, 0x0E, pw(cur, 0x0E) - 2);
            spw(cur, 0x12, pw(cur, 0x12) - (sc << 2));
        }
    } else if ((bits & 2) && (int16_t)pw(cur, 0x0E) < (int16_t)(GW(map_height) - 8)) {
        spw(cur, 0x0E, pw(cur, 0x0E) + 2);
        spw(cur, 0x12, pw(cur, 0x12) + (sc << 2));
    }
    if (bits & 4) {
        if ((int16_t)pw(cur, 0x0C) > 0) {
            spw(cur, 0x0C, pw(cur, 0x0C) - 2);
            spw(cur, 0x10, pw(cur, 0x10) - (sc << 2));
        }
    } else if ((bits & 8) && (int16_t)pw(cur, 0x0C) < (int16_t)(GW(map_width) - 10)) {
        spw(cur, 0x0C, pw(cur, 0x0C) + 2);
        spw(cur, 0x10, pw(cur, 0x10) + (sc << 2));
    }
    t24d3_0000(pfp(pl, 5));
}

/* the unit's screen or the status screen over a player's window (the
 * cursor's state 4), until fire */
static void loop_info(fptr cur, fptr in, int bits, int side, fptr map)
{
    if (pb(cur, 0x18) == 0) {
        unsigned b = pb(map, pw(cur, 0) + 1);

        if (b <= 0xF0) {
            if ((pw(UNIT(b), 6) & 2) && !same_side(pw(UNIT(b), 4), side))
                b = 0xFF;                   /* hidden: the status screen */
            else {
                unsigned f = pw(UNIT(b), 4);

                spb(cur, 0x18, 6);
                spb(cur, 0x1B, 1);
                spw(cur, 4, pw(cur, 0x0C));
                spw(cur, 6, pw(cur, 0x0E));
                spw(cur, 0x10, 0x75 + ((side & 1) ? 0xA0 : 0));
                spw(cur, 0x12, 0x87);
                if ((f & 0x40) && !(f & 0x80))
                    b = (b - 1) & 0xFF;
                spw(cur, 0x1E, b);
                draw_unit_info(side, pw(cur, 0), (int)b, map);
                b = 0;
            }
        }
        if (b) {
            spb(cur, 0x18, 7);
            spb(cur, 0x1B, 1);
            spw(cur, 4, pw(cur, 0x0C));
            spw(cur, 6, pw(cur, 0x0E));
            spw(cur, 0x10, 0x75 + ((side & 1) ? 0xA0 : 0));
            spw(cur, 0x12, 0x8B);
            draw_status(side);
        }
        ask_sound(2);
    }
    if (bits & 0x10) {
        spb(cur, 0x17, 0);
        spb(cur, 0x18, 0x19);
        spb(cur, 0x1B, 5);
        spw(cur, 0x0C, pw(cur, 4));
        spw(cur, 0x0E, pw(cur, 6));
        t0d36_0044(cur, side);
        spb(in, 8, 0);
        draw_window(pw(cur, 2), side);
        draw_marks(pw(cur, 2), side);
    }
}

/* fire with left on a building or a unit that holds others (the cursor's
 * state 6): which record its screen shows */
static void loop_open_building(fptr cur, int side, fptr map)
{
    unsigned flags = pw(GROUND(pb(map, pw(cur, 0))), 0), b = pb(map, pw(cur, 0) + 1);

    spb(cur, 0x17, 2);
    spb(cur, 0x18, 9);
    if (flags & 0x200) {
        spw(cur, 0x1E, find_building(pw(cur, 0), FP(factories)));
        spfp(cur, 0x24, MKFP(S_factories, A_factories + 0x1C * pw(cur, 0x1E)));
        spw(cur, 0x22, 1);
    } else if (flags & 0x80) {
        spw(cur, 0x1E, find_building(pw(cur, 0), FP(depots)));
        spfp(cur, 0x24, MKFP(S_depots, A_depots + 0x1C * pw(cur, 0x1E)));
        spw(cur, 0x22, 1);
    } else if (flags & 0x40) {
        spw(cur, 0x22, 1);
        spw(cur, 0x1E, same_side(side, (int)flags) ? side : !side);
        spfp(cur, 0x24, MKFP(S_hqs, A_hqs + 0x1C * pw(cur, 0x1E)));
    } else {
        if (pw(UNIT(b), 4) & 0x2000)
            b--;
        spw(cur, 0x1E, (int8_t)pb(UNIT(b), 0x0A));
        spfp(cur, 0x24, MKFP(S_cargo, A_cargo + 0x1C * pw(cur, 0x1E)));
        spw(cur, 0x22, 2);
    }
}

/* the box of the slot the cursor is on in a building's screen: the unit's
 * numbers and big picture, or FREE PART, and the building's energy (or a
 * holder's room) by the title */
static void building_box(fptr cur, int side)
{
    fptr rec = pfp(cur, 0x24);
    int x = 0x38, y = 0x7D, x0 = (side & 1) ? 0xA0 : 0, n, i;
    unsigned f;

    draw_slots(0x10, 0x0C, side, rec);
    SW(draw_x1, x0 + 0x2C), SW(draw_y1, 0x16), SW(draw_x2, x0 + 0x8B), SW(draw_y2, 0x75);
    SW(draw_colour, GBO(amok, 0x10));
    fill_rect();
    spw(cur, 0x1E, pb(rec, (unsigned)(7 * side) + pw(cur, 0x0E)));
    if (pw(cur, 0x1E) != 0xFF) {
        f = pw(UNIT(pw(cur, 0x1E)), 4);
        SW(draw_colour, GBO(amok, 0x0D));
        if (f & (0x400 | 0x800)) {
            x += x0;
            t1479_028e(x, y);
            draw_text(x, y, (f & 0x400) ? 8 : 9, GBO(amok, 0x0D));
            show_message(-1, side);
        } else if (!(GW(game_flags) & (side ? 8 : 4))) {
            draw_unit_numbers(x, y, pb(cur, 0x1E), side);
            SW(game_flags, GW(game_flags) & ~(side ? 8 : 4));
        }
        f = pw(UNIT(pw(cur, 0x1E)), 4);
        t1479_0c93(0x2C, 0x16, pb(UNIT(pw(cur, 0x1E)), 8), side, (f & 1) ? 1 : (f & 2) ? side : 0);
    } else {
        x += x0;
        t1479_028e(x, y);
        draw_text(x, y, 2, GBO(amok, 0x0D));
        show_message(-1, side);
    }
    if (pw(cur, 0x22) == 1) {
        x = (side & 1) ? 0xCC : 0x2C;
        draw_bar(x + 0x4E, 0x0D, GBO(amok, 0x13), GBO(amok, 0x14), 5, pb(rec, 0x16 + side) / 7 + 1, 0x23);
        draw_bar(x + 0x32, 0x0D, GBO(amok, 0x0E), GBO(amok, 0x0E), 6, 0x1E, 0x23);
        SW(draw_colour, GBO(amok, 0x13));
        draw_number(pb(rec, 0x16 + side), x + 0x32, 0x0D);
    } else {
        /* the room left in a unit that holds others */
        n = 0;
        for (i = 0; i < 7; i++) {
            unsigned s = pb(rec, (unsigned)(7 * side + i));

            if (s <= 0xF0)
                n += pb(TYPE(pb(UNIT(s), 8)), 0x40);
        }
        n = (pb(TYPE(pb(UNIT(pb(rec, 0x1B)), 8)), 0x3F) - n) & 0xFF;
        x = (side & 1) ? 0x110 : 0x70;
        draw_bar(x, 0x0E, GBO(amok, 0x0E), GBO(amok, 0x0E), 6, 0x1E, 0x1E);
        SW(draw_colour, GBO(amok, 0x13));
        draw_number(n, x, 0x0D);
    }
}

/* a building's screen (the cursor's state 2; BATTLE.hints at
 * draw_building has it step by step): 9 it opens, 0Ah a slot is chosen,
 * 0Bh fire held shows what a direction would do and fire let go does it
 * (up: the unit out onto the map, down: a repair, left: the list of
 * types to make, right: leave), 0Ch the list, 0Dh back from it, 8 it
 * closes */
static void loop_building(fptr cur, fptr in, int bits, int side, fptr map)
{
    fptr rec = pfp(cur, 0x24), pl = player_rec(side), u;
    int sound, i;
    unsigned f, n;

    if (pb(cur, 0x18) == 9) {
        spb(cur, 0x1B, 5);
        spw(cur, 4, pw(cur, 0x0C));
        spw(cur, 6, pw(cur, 0x0E));
        spw(cur, 0x0C, 0);
        spw(cur, 0x0E, 3);
        for (i = 0; i < 7; i++)
            if (pb(rec, (unsigned)(7 * side + i)) != 0xFF) {
                spw(cur, 0x0E, i);
                break;
            }
        spw(cur, 0x12, pw(cur, 0x0E) * 0x18 + 0x0C);
        spw(cur, 0x10, 0x10 + (side ? 0xA0 : 0));
        draw_building(side, rec);
        if (pw(cur, 0x22) == 1 && (pb(cur, 0x16) & 2)) {
            n = pb(rec, 0x16 + side);
            if (n > 0xFA)
                spb(rec, 0x16 + side, 0xFA);
            list_makeable((int)n);
        }
        ask_sound(2);
        spb(cur, 0x18, 0x0A);
        spb(cur, 0x1A, 1);
    }
    if (pb(cur, 0x18) == 0x0A) {
        sound = 0xFF;
        if (bits & 0x10) {
            spb(cur, 0x18, 0x0B);
            spb(cur, 0x19, 1);
        } else {
            if (bits & 1) {
                if ((int16_t)pw(cur, 0x0E) > 0) {
                    spw(cur, 0x0E, pw(cur, 0x0E) - 1);
                    spw(cur, 0x12, pw(cur, 0x12) - 0x18);
                    spb(cur, 0x1A, 1);
                    sound = side ? 1 : 0;
                }
            } else if ((bits & 2) && (int16_t)pw(cur, 0x0E) < 6) {
                spw(cur, 0x0E, pw(cur, 0x0E) + 1);
                spw(cur, 0x12, pw(cur, 0x12) + 0x18);
                spb(cur, 0x1A, 1);
                sound = side ? 1 : 0;
            }
            if (pb(cur, 0x1A) == 1) {
                building_box(cur, side);
                spb(cur, 0x1A, 0);
            }
            if (sound != 0xFF)
                ask_sound(sound);
        }
    }
    if (pb(cur, 0x18) == 0x0B) {
        if (pb(cur, 0x19) == 1) {
            /* what fire offers on this slot */
            spw(cur, 0x1C, 2);
            spw(cur, 0x1E, pb(rec, (unsigned)(7 * side) + pw(cur, 0x0E)));
            if ((int16_t)pw(cur, 0x1E) <= 0xF0) {
                f = pw(UNIT(pw(cur, 0x1E)), 4);
                if (!(f & 0x100) && !(f & 0x200) && same_side((int)f, side) && !(f & 2)) {
                    if (pb(cur, 0x16) & 2) {
                        if (!(pw(UNIT(pw(cur, 0x1E)), 6) & 4))
                            spw(cur, 0x1C, pw(cur, 0x1C) | 0x100);
                    } else if (!(GW(game_flags2) & 0x80))
                        spw(cur, 0x1C, pw(cur, 0x1C) | 0x10);
                }
            } else {
                f = pw(rec, 0x19);
                if ((f & 8) && !(f & 2) && same_side(side, (int)f) && GB(types_listed) && (pb(cur, 0x16) & 2))
                    spw(cur, 0x1C, pw(cur, 0x1C) | 0x80);
            }
            if (pw(cur, 0x22) == 2)
                spw(cur, 0x1C, pw(cur, 0x1C) & 0xFE7F);
            spb(cur, 0x19, 2);
        }
        if (pb(cur, 0x19) == 2) {
            sound = 0xFF;
            if (bits & 0x10) {
                spb(cur, 0x1B, 5);
                if ((bits & 8) && (pw(cur, 0x1C) & 2))
                    spb(cur, 0x1B, 1);
                else if ((bits & 2) && (pw(cur, 0x1C) & 0x100))
                    spb(cur, 0x1B, 8);
                else if ((bits & 4) && (pw(cur, 0x1C) & 0x80))
                    spb(cur, 0x1B, 7);
                else if (pb(pl, 0x15) >= pb(pl, 0x16) && !(pw(pl, 0) & 2)) {
                    show_message(0x21, side);
                    message_flag(side);
                } else if ((bits & 1) && (pw(cur, 0x1C) & 0x10))
                    spb(cur, 0x1B, 4);
                draw_slots(0x10, 0x0C, side, rec);
            } else if (pb(cur, 0x1B) == 4) {
                /* the unit out onto the map, chosen to be moved */
                int sq = (int16_t)pw(rec, 0x0E), w = (int16_t)GW(map_width), s;

                n = pb(rec, (unsigned)(7 * side) + pw(cur, 0x0E));
                u = UNIT(n);
                if (pw(rec, 0x19) & 0x20)
                    sq--;
                spb(rec, (unsigned)(7 * side) + pw(cur, 0x0E), 0xFF);
                spw(u, 0x0B + 2 * side, sq + 1);
                t0d36_032c(cur, 0);
                t0e9b_15dd(side, sq);
                f = (pw(u, 4) & 1) ? 6 : 3;
                if (pw(u, 6) & 4)
                    f |= 0x18;
                reach(sq, pfp(pl, 0x0D), (int)n, (pw(u, 6) & 0x10) ? 4 : 5, side, (int)f, map);
                spb(map, (uint16_t)sq + 1, n);
                s = (int16_t)pw(u, 0x0B + 2 * side) >> 1;
                spw(u, 4, pw(u, 4) | 0x100);
                s = s % w + ((s / w) << 6);
                SBO(marks, (uint16_t)s, GBO(marks, (uint16_t)s) | (side ? 0x80 : 0x40));
                draw_window(pw(cur, 2), side);
                draw_marks(pw(cur, 2), side);
                redraw_square(side, pw(cur, 0));
                spb(cur, 0x17, 0);
                spb(cur, 0x18, 0x16);
                spb(cur, 0x19, 3);
                spw(cur, 0x1E, n);
                spb(cur, 0x1B, 4);
                sound = 2;
            } else if (pb(cur, 0x1B) == 8) {
                /* a repair for 3 of the building's energy (25 for the
                 * pioneers) */
                unsigned cost = 3;
                int message;

                u = UNIT(pb(rec, (unsigned)(7 * side) + pw(cur, 0x0E)));
                sound = 3;
                if (pw(u, 6) & 0x4000)
                    cost = 0x19;
                if (pb(rec, 0x16 + side) < cost)
                    message = 0x18;
                else if (pb(TYPE(pb(u, 8)), 2) == pb(u, 2) && (!(pw(u, 6) & 0x4000) || pb(u, 0x0A) == 2))
                    message = 0x1E;
                else {
                    unsigned type = pb(u, 8);

                    spb(rec, 0x16 + side, pb(rec, 0x16 + side) - cost);
                    list_makeable(pb(rec, 0x16 + side));
                    spw(u, 4, pw(u, 4) | 0x0A00);
                    spb(u, 2, pb(TYPE(type), 2));
                    if (pw(u, 6) & 0x4000)
                        spb(u, 0x0A, 2);
                    message = 0x1F;
                    sound = 2;
                }
                show_message(message, side);
                message_flag(side);
                spb(cur, 0x18, 0x0A);
                spb(cur, 0x19, 0);
                spb(cur, 0x1B, 5);
                spb(cur, 0x1A, 1);
            } else if (pb(cur, 0x1B) == 7) {
                spb(cur, 0x18, 0x0C);
                spb(cur, 0x19, 1);
                sound = 2;
            } else if (pb(cur, 0x1B) == 1)
                spb(cur, 0x18, 8);
            else {
                spb(cur, 0x18, 0x0A);
                spb(cur, 0x19, 0);
            }
            if (sound != 0xFF)
                ask_sound(sound);
        }
    }
    if (pb(cur, 0x18) == 0x0C) {
        /* the list of the types the energy pays for */
        if (pb(cur, 0x19) == 1) {
            spw(cur, 0x20, 0);
            spw(cur, 0x0A, pw(cur, 0x0E));
            spw(cur, 8, pw(cur, 0x0C));
            spw(cur, 0x0E, 0);
            spb(cur, 0x1B, 5);
            spw(cur, 0x10, 0x10 + ((side & 1) ? 0xA0 : 0));
            spw(cur, 0x12, 0x0C);
            draw_type_list(0x10, 0x0C, side, pw(cur, 0x20));
            spb(cur, 0x1A, 2);
            spb(cur, 0x19, 4);
        }
        if (pb(cur, 0x19) == 4) {
            sound = 0xFF;
            if (bits & 0x10)
                spb(cur, 0x19, 2);
            else {
                if (bits & 1) {
                    if ((int16_t)pw(cur, 0x0E) > 0) {
                        spw(cur, 0x0E, pw(cur, 0x0E) - 1);
                        spw(cur, 0x12, pw(cur, 0x12) - 0x18);
                        spb(cur, 0x1A, 2);
                        sound = side ? 1 : 0;
                    } else if ((int16_t)pw(cur, 0x20) > 0) {
                        spw(cur, 0x20, pw(cur, 0x20) - 1);
                        spb(cur, 0x1A, 2);
                        sound = side ? 1 : 0;
                    }
                } else if (bits & 2) {
                    if ((int16_t)pw(cur, 0x0E) < 6) {
                        if ((int16_t)pw(cur, 0x0E) < (int)GB(types_listed) - 1) {
                            spw(cur, 0x0E, pw(cur, 0x0E) + 1);
                            spw(cur, 0x12, pw(cur, 0x12) + 0x18);
                            spb(cur, 0x1A, 2);
                            sound = side ? 1 : 0;
                        }
                    } else if ((int16_t)pw(cur, 0x20) < (int)GB(types_listed) - 7) {
                        spw(cur, 0x20, pw(cur, 0x20) + 1);
                        spb(cur, 0x1A, 2);
                        sound = side ? 1 : 0;
                    }
                }
                if (pb(cur, 0x1A) == 2) {
                    int x = 0x38, y = 0x7D, x0 = (side & 1) ? 0xA0 : 0;

                    draw_type_list(0x10, 0x0C, side, pw(cur, 0x20));
                    SW(draw_x1, x0 + 0x2C), SW(draw_y1, 0x16), SW(draw_x2, x0 + 0x8B), SW(draw_y2, 0x75);
                    SW(draw_colour, GBO(amok, 0x10));
                    SWO(clip_on, 2, 1);
                    fill_rect();
                    spw(cur, 0x1E, GBO(types_list, (uint16_t)(pw(cur, 0x0E) + pw(cur, 0x20))));
                    if (pw(cur, 0x1E) != 0xFF) {
                        if (side & 1)
                            x += 0xA0;
                        t1479_028e(x, y);
                        draw_text(x, y, 7, GBO(amok, 0x0D));
                        draw_number(pb(TYPE(pw(cur, 0x1E)), 0x3E), x + 0x1E, y + 0x18);
                        t1479_0c93(0x2C, 0x16, pw(cur, 0x1E), side, (side & 1) ? 1 : 0);
                    }
                    spb(cur, 0x1A, 0);
                }
            }
            if (sound != 0xFF)
                ask_sound(sound);
        }
        if (pb(cur, 0x19) == 2) {
            if (bits & 0x10) {
                spb(cur, 0x1B, (bits & 4) ? 7 : (bits & 8) ? 1 : 5);
                draw_type_list(0x10, 0x0C, side, pw(cur, 0x20));
            } else if (pb(cur, 0x1B) == 7) {
                /* the type made into the slot for its cost */
                int owner = (pw(rec, 0x19) & 1) ? 1 : 0;
                unsigned type = GBO(types_list, (uint16_t)(pw(cur, 0x0E) + pw(cur, 0x20)));

                n = (unsigned)t169e_01a9(0x8000) & 0xFF;
                make_unit((int)n, (int)type, pw(rec, 0x0E), owner);
                spw(UNIT(n), 4, pw(UNIT(n), 4) | 0x4600);
                computer_unit_new((int)n);
                spb(rec, (unsigned)(7 * side) + pw(cur, 0x0A), n);
                SB(units_made, GB(units_made) + 1);
                spb(player_rec(owner), 2, pb(player_rec(owner), 2) + 1);
                spb(rec, 0x16 + side, pb(rec, 0x16 + side) - pb(TYPE(type), 0x3E));
                list_makeable(pb(rec, 0x16 + side));
                ask_sound(2);
                spb(cur, 0x18, 0x0D);
            } else if (pb(cur, 0x1B) == 1)
                spb(cur, 0x18, 0x0D);
            else
                spb(cur, 0x19, 4);
        }
    }
    if (pb(cur, 0x18) == 0x0D) {
        spw(cur, 0x0C, pw(cur, 8));
        spw(cur, 0x0E, pw(cur, 0x0A));
        spw(cur, 0x10, 0x10 + ((side & 1) ? 0xA0 : 0));
        spw(cur, 0x12, pw(cur, 0x0E) * 0x18 + 0x0C);
        spb(cur, 0x1B, 5);
        spb(cur, 0x18, 0x0A);
        spb(cur, 0x19, 0);
        spb(cur, 0x1A, 1);
    }
    if (pb(cur, 0x18) == 8) {
        spb(cur, 0x17, 0);
        spb(cur, 0x18, 0x19);
        spb(cur, 0x19, 0);
        spfp(cur, 0x24, 0);
        spb(cur, 0x1B, 5);
        spw(cur, 0x0C, pw(cur, 4));
        spw(cur, 0x0E, pw(cur, 6));
        t0d36_0044(cur, side);
        show_message(-1, side);
        SW(game_flags2, GW(game_flags2) & (side ? ~0x10 : ~8));
        spb(in, 8, 0);
        draw_window(pw(cur, 2), side);
        draw_marks(pw(cur, 2), side);
    }
}

/* the change of phase asked for (the cursor's state 5): once both
 * players asked, F1 carries it out (two computers do without); D saves
 * the game; fire takes the request back */
static void loop_change(fptr cur, int bits, int side, unsigned *key, fptr orders, fptr cursor_lib, fptr pmp,
                        fptr palette)
{
    fptr other = (side & 1) ? CURSOR(0) : CURSOR(1), ks = GFP(key_set);

    if (bits & 0x10) {
        spb(cur, 0x17, 0), spb(cur, 0x18, 0), spb(cur, 0x19, 0);
        spb(cur, 0x1B, 5);
        message_flag(side);
        show_message(2, side);
        SW(pass_ticks, 4);
        return;
    }
    if (pb(other, 0x17) != 5) {
        int o = side ? 0 : 1;

        /* the other has not asked yet */
        show_message(pb(cur, 0x16) == 2 ? 1 : 0, side);
        if (!(pw(player_rec(side), 0) & 2) && (pw(player_rec(o), 0) & 2)
            && (GWO(computer_state, 0x0C * o) & 0x60))
            SW(menu_flags, GW(menu_flags) | 0x80);
        if (!(pw(player_rec(side), 0) & 2) && !(GW(menu_flags) & 2)) {
            SW(pass_ticks, 0);
            SW(game_flags, GW(game_flags) & ~0x800);
        }
        return;
    }
    SW(pass_ticks, 4);
    if (((pw(player_rec(0), 0) & 2) && (pw(player_rec(1), 0) & 2)) || pb(ks, 0x0C) == *key) {
        int sq0 = pw(CURSOR(0), 0), sq1 = pw(CURSOR(1), 0), attacker, result;

        *key = 0;
        attacker = (pb(CURSOR(0), 0x16) & 2) ? 0 : 1;
        if (!pb(player_rec(attacker), 0x15)) {
            t2590_000a();
            t0e9b_1cb2();
            t2592_0002();
            SW(number_asked, 0);
        } else
            SW(number_asked, 1);
        result = (int8_t)change_phase(orders, cursor_lib, pmp, palette);
        t0e9b_15dd(0, sq0);
        t0e9b_15dd(1, sq1);
        t2590_000a();
        if (GW(number_asked))
            t0e9b_1cb2();
        t0e9b_1b9d(pw(CURSOR(0), 2), pw(CURSOR(1), 2));
        t2592_0002();
        copy_page();
        if (result >= 0) {
            /* a headquarters is taken: the map's end */
            spb(CURSOR(1), 0x17, 7);
            spb(CURSOR(0), 0x17, 7);
            spw(CURSOR(result), 0x22, 0x0F);
            spw(CURSOR((result & 1) ? 0 : 1), 0x22, 0x10);
        }
        history_add(pb(player_rec(0), 2), pb(player_rec(1), 2));
        SWO(computer_state, 0, GWO(computer_state, 0) | 4);
        SWO(computer_state, 0x0C, GWO(computer_state, 0x0C) | 4);
    } else if (pb(ks, 0x1A) == *key) {
        show_message(save_game(palette), 0);
        timer_set(1, 5, 0);
        SW(game_flags2, GW(game_flags2) | 0x200);
        timer_set(6, 4, 0);
        draw_window(pw(CURSOR(0), 2), 0);
        draw_marks(pw(CURSOR(0), 2), 0);
        flip_page();
        copy_page();
    } else {
        /* F1 : CHANGE MODE and CANCEL : PRESS BUTTON, or with the
         * controller's buttons named */
        const char *f1 = bi_pad_name(pb(ks, 0x0C)), *no = bi_pad_name(0x1C);
        char text[32];

        if (f1 && no) {
            snprintf(text, sizeof text, "%s : CHANGE MODE", f1);
            show_text(text, 0);
            snprintf(text, sizeof text, "CANCEL : PRESS %s", no);
            show_text(text, 1);
        } else {
            show_message(0x20, 0);
            show_message(0x0D, 1);
        }
    }
}

/* the map's end (the cursor's state 7): the two players' messages, a
 * key, and the loop ends */
static void loop_end(void)
{
    int a = (int16_t)pw(CURSOR(0), 0x22), b = (int16_t)pw(CURSOR(1), 0x22);

    if (a == 0x0F) {
        a = 0x0A, b = 0x0B;
        SW(game_flags, GW(game_flags) | 0x4000);
    } else if (b == 0x0F) {
        b = 0x0A, a = 0x0B;
        SW(game_flags, GW(game_flags) | 0x4000);
    } else if (a == 0x11) {
        a = 0x0F, b = 0x10;
        SW(game_flags, GW(game_flags) | 0x2000);
    } else if (b == 0x11) {
        b = 0x0F, a = 0x10;
        SW(game_flags, GW(game_flags) | 0x2000);
    } else
        a = b = 0x10;
    /* 10h: the winner's animation follows */
    if (GW(menu_flags) & 2)
        SW(game_flags, GW(game_flags) | 0x10);
    else if (GW(menu_flags) & 1) {
        fptr c = (pw(player_rec(0), 0) & 2) ? CURSOR(1) : CURSOR(0);

        if ((pw(c, 0x22) == 0x0F || pw(c, 0x22) == 0x11) && !(pw(player_rec(0), 0) & 2))
            SW(game_flags, GW(game_flags) | 0x10);
    }
    if (!(GW(game_flags) & 0x6000))
        SW(game_flags, GW(game_flags) & ~0x10);
    show_message(a, 0);
    show_message(b, 1);
    flip_page();
    copy_page();
    t0d36_0c85();
    SW(game_flags, GW(game_flags) & ~2);
}

/* a pass of the map's loop for one player */
static void loop_player_pass(int side, unsigned *key, fptr orders, fptr cursor_lib, fptr pmp, fptr palette)
{
    fptr in = (side & 1) ? FP(input1_events) : FP(input0_events), cur = CURSOR(side), map;
    int bits, i;

    if (GW(key_there)) {
        SW(key_there, 0);
        *key = GB(key_scan);
    }
    if (pw(player_rec(side), 0) & 2) {
        /* the computer: its own input record, its keys from its script */
        in = COMPUTER_INPUT;
        for (i = 0; i < 8; i++)
            spb(in, (unsigned)i, 0);
        spb(in, 8, 1);
        if (GW(menu_flags) & 0x80) {
            /* the human asked for the change: the computer thinks on
             * until it is done, or a key */
            SBO(input0_events, 4, 0);
            SBO(input1_events, 4, 0);
            SW(key_there, 0);
            t2590_000a();
            show_message(0x32, side);
            do {
                computer_step(side);
                clock_idle();
            } while ((GWO(computer_state, 0x0C * side) & 0x60) && !GBO(input0_events, 4)
                     && !GBO(input1_events, 4) && !GW(key_there));
            show_message(-1, 0);
            show_message(-1, 1);
            SW(game_flags, GW(game_flags) & ~0x0C);
            t2592_0002();
            SW(menu_flags, GW(menu_flags) & ~0x80);
        } else
            computer_step(side);
        bits = (int16_t)GWO(computer_keys, 4 * side);
        show_message((int16_t)GWO(computer_keys, 4 * side + 2), side);
        message_flag(side);
    } else
        bits = t0d36_04d8(in, cur);
    map = side ? GFP(map1) : GFP(map0);
    SFP(loop_map, map);
    if (!(bits & 0x0F) && pb(cur, 0x17) == 0)
        loop_unit_line(cur, map, side);
    loop_keys(*key);
    if (pb(cur, 0x17) == 0 && t0d36_0642(cur, in, bits, map, side))
        loop_cursor_move(cur, bits, side);
    if (pb(cur, 0x17) == 1)
        t0d36_0d82(cur, side, bits);
    if (pb(cur, 0x17) == 3)
        loop_overview(cur, in, bits, side, pmp);
    else if (pb(cur, 0x17) == 4)
        loop_info(cur, in, bits, side, map);
    else if (pb(cur, 0x17) == 6)
        loop_open_building(cur, side, map);
    if (pb(cur, 0x17) == 2)
        loop_building(cur, in, bits, side, map);
    if (pb(cur, 0x17) == 5)
        loop_change(cur, bits, side, key, orders, cursor_lib, pmp, palette);
    if (pb(cur, 0x17) == 7)
        loop_end();
    if (GW(game_flags2) & 0x100) {
        /* the sound asked for in this pass, on the player's channel */
        if (GW(game_flags) & 0x80) {
            int ch = side ? 1 : 3;

            SWO(effects_asked, 6 * ch, GW(number_asked));
            SWO(effects_asked, 6 * ch + 2, 0x4E);
            SWO(effects_asked, 6 * ch + 4, 1);
        }
        SW(game_flags2, GW(game_flags2) & ~0x100);
    }
}

/* the map's loop, until game_flags' bit 2 is cleared */
static void map_loop(fptr orders, fptr cursor_lib, fptr pmp, fptr palette)
{
    unsigned key;
    int i;

    while (GW(game_flags) & 2) {
        bi_at("map_pass");
        key = 0;
        SW(tick_count, 0);
        for (SB(loop_player, 0); GB(loop_player) < 2; SB(loop_player, GB(loop_player) + 1))
            loop_player_pass(GB(loop_player), &key, orders, cursor_lib, pmp, palette);
        if (GW(game_flags2) & 8) {
            timer_set(1, 0x19, 0);
            SW(game_flags2, GW(game_flags2) & ~8);
        }
        if (GW(game_flags2) & 0x10) {
            timer_set(2, 0x19, 0);
            SW(game_flags2, GW(game_flags2) & ~0x10);
        }
        if (GW(game_flags2) & 0x40) {
            move_step();
            SW(game_flags2, GW(game_flags2) & ~0x40);
        }
        /* the two cursors, each over what it covers when a screen is up */
        for (i = 0; i < 2; i++) {
            fptr keep = (pw(player_rec(i), 0) & 4) ? pfp(player_rec(i), 5) : 0;

            draw_entry((int16_t)pw(CURSOR(i), 0x10), (int16_t)pw(CURSOR(i), 0x12),
                       pfp(pfp(FP(lib_cursor), 0x0E), 4 * pb(CURSOR(i), 0x1B)), FOFF(keep), FSEG(keep),
                       bi_prog == BI_MOON ? 0x40 : 0); /* MOON.EXE adds the colour base 40h */
        }
        effects_start();
        flip_page();
        copy_page();
        SD(passes, GD(passes) + 1);
        for (i = 0; i < 0x0F; i++)
            if (pd(FP(timer_dues), 4 * (unsigned)i) <= GD(passes)) {
                timer_due(GBO(timer_kinds, i), i);
                SBO(timer_kinds, i, 0);
            }
        while ((int16_t)GW(tick_count) < (int16_t)GW(pass_ticks))
            clock_idle();
    }
}

/* MOON.EXE's films (T070B:4703 and 49ED): three files of ANIM loaded one
 * behind the other at `buffer` (T26F8:0000 is load_file that gives the
 * size), the frames ("VDIF") one after the other, a frame each `ticks`
 * ticks; the first is drawn behind a black palette and faded in; -1
 * when a file is missing */
static int moon_film(fptr buffer, fptr work, fptr ani, fptr pal, fptr snd, int frames, int ticks, int first_wait,
                     fptr *start)
{
    fptr anim, palette;
    long n;
    int i;

    for (i = 0; i < 0x300; i++)
        SBO(picture_palette, i, 0);
    set_palette(0xFF, FP(picture_palette));
    SW(draw_colour, 0);
    clear_page();
    check_vga_disk();
    n = t2624_0006(buffer, make_path(1, 3, ani, -1), work, NULL);
    if (n == -1)
        return -1;
    anim = *start = buffer;
    buffer = hadd(buffer, n);
    n = t2624_0006(buffer, make_path(1, 3, pal, -1), work, NULL);
    if (n == -1)
        return -1;
    palette = buffer;
    buffer = hadd(buffer, n);
    if (load_song(buffer, make_path(1, 3, snd, -1), work) == -1)
        return -1;
    anim = t24c5_002e(0, 0, anim);
    flip_page();
    copy_page();
    for (i = 0; i < 0x300; i++)
        SBO(picture_palette, i, pb(palette, i));
    fade_in();
    play_song(0, 0);
    if (first_wait)
        t0d36_000f(first_wait);
    for (i = 0; i < frames; i++) {
        SW(tick_count, 0);
        anim = t24c5_002e(0, 0, anim);
        flip_page();
        copy_page();
        while ((int16_t)GW(tick_count) < ticks)
            wait_retrace();
    }
    return 0;
}

/* T070B:4703: a map won, where BATTLE.EXE plays play_anim 2 or 3: HQ.ANI
 * (0: the headquarters taken; 87h frames of 2 ticks after a wait of 300)
 * or TOT.ANI (1: the other's units gone; 80h frames of 6 ticks) */
static void moon_win_film(fptr buffer, fptr work, int which)
{
    fptr start;

    make_path(1, 3, 0, -1);
    if (moon_film(buffer, work, MKFP(S_name_win_ani, A_name_win_ani + 0x0E * which),
                  MKFP(S_name_win_pal, A_name_win_pal + 0x0E * which),
                  MKFP(S_name_win_snd, A_name_win_snd + 0x0E * which),
                  which ? 0x80 : 0x87, which ? 6 : 2, which ? 0 : 0x12C, &start))
        return;
    t0d36_000f(0xDC);
    stop_song();
    fade_out();
}

/* T070B:49ED: the last map won, where BATTLE.EXE plays play_anim 4 and
 * end_credits: END.ANI (4Ch frames of 6 ticks), then its first frame
 * again with five lines of text (MOON's codes asked for) for 2000 ticks */
static void moon_end_film(fptr buffer, fptr work)
{
    static const uint16_t at[5][3] = {
        { 0x36, 0x78, 0x00 }, { 0x48, 0x84, 0x2A }, { 0x48, 0x8C, 0x4E }, { 0x5E, 0x9B, 0x71 }, { 0x5E, 0xA5, 0x8B },
    };
    fptr start;
    int i;

    if (moon_film(buffer, work, FP(name_end_ani), FP(name_end_pal), FP(name_end_snd), 0x4C, 6, 0x12C, &start))
        return;
    t0d36_000f(0x3E8);
    stop_song();
    fade_out();
    t0d36_000f(0x258);
    t24c5_002e(0, 0, start);
    SW(draw_colour, 1);
    for (i = 0; i < 5; i++)
        draw_chars(at[i][0], at[i][1], MKFP(S_end_text, A_end_text + at[i][2]));
    flip_page();
    copy_page();
    fade_in();
    t0d36_000f(0x7D0);
    fade_out();
}

/* main: the switch /m (the third palette in the map, and another text
 * than "Color." at the start).  The original's /s, the PC speaker's
 * sound, is not taken: the port plays the AdLib's. */
void battle_main(int argc, char **argv)
{
    fptr work, p, palette, orders, cursor_lib, pmp, spare;
    int colour = 1, i;

    t2354_0011();
    SW(game_flags, 0);
    SW(menu_flags, 0);
    SWO(disk_record, 0, 1);
    SWO(disk_record, 2, 0);
    t164d_0482(1);
    for (i = 1; i < argc; i++)
        if (argv[i][0] == '/' && (argv[i][1] == 'm' || argv[i][1] == 'M'))
            colour = 0;
    SBO(menu_items, 11 * 0x2E + 0x2B, colour ? 0 : 2);
    sound_init(0);
    t0d36_000f(0x32);
    work = t2619_0004(0x493E0);
    if (work == 0)
        fatal_error(1);
    p = work;
    if (t2485_0001(0x140, 0xC8, 4, 0x2518, 0, 1))
        fatal_error(3);
    SW(draw_colour, 0);
    clear_page();
    flip_page();
    clear_page();
    SFP(font_ptr, p);
    p = hadd(p, 0xBF4);
    work = hadd(work, 0xBF4);
    if (!load_file(GFP(font_ptr), make_path(1, -1, FP(name_char6), 5), work))
        fatal_error(2);
    if (bi_skip_intro ? title_skip() : title(work))
        fatal_error(2);
    SWO(players, 0x17, 0);
    SWO(players, 0, 0);
    SD(score_now, 0);
    SW(map_number, 0);
    stop_song();
    t2354_0530();
    do {
        SW(game_flags, GW(game_flags) | 1);
        SW(game_flags, GW(game_flags) & ~0x10);
        /* until fire is let go */
        do {
            SBO(input0_events, 4, 0);
            t0d36_000f(5);
        } while (GBO(input0_events, 4));
        t2354_056f();
        SW(one_page, 0);
        t2593_0006();
        flip_page();
        /* the mouse (T267C:000E, T2683:000A, T268A:0006): the port has
         * none */
        bi_at("menu");
        if (menu(work))
            fatal_error(2);
        t2354_0530();
        SW(one_page, 0xFFFF);
        t2593_0006();
        flip_page();
        if (!(GW(game_flags) & 0x100)) {
            bi_at("map_setup");
            map_setup(work, &palette, &orders, &cursor_lib, &pmp, &spare);
            map_loop(orders, cursor_lib, pmp, palette);
            if (GW(game_flags) & 0x40)
                stop_song();
            fade_out();
            restore_sprites();
            if (GW(game_flags) & 0x10) {
                /* the winner's animation: a headquarters blown up, or
                 * the other's units gone */
                fptr path = make_path(1, 3, 0, -1);

                SW(draw_colour, 0);
                clear_page();
                check_vga_disk();
                if (bi_prog == BI_MOON) {
                    bi_at("moon_win_film");
                    moon_win_film(hadd(spare, 0x2710), spare, (GW(game_flags) & 0x2000) ? 1 : 0);
                } else {
                    play_anim((GW(game_flags) & 0x2000) ? 3 : 2, spare, 0, 0, 1, path);
                    t0d36_000f(0x64);
                    fade_out();
                }
            }
            bi_at("after_map");
            after_map(spare);
            if ((GW(game_flags) & 0x20) && bi_prog == BI_MOON) {
                bi_at("moon_end_film");
                moon_end_film(hadd(work, 0x2710), work);
                SW(draw_colour, 0);
                clear_page();
                flip_page();
                clear_page();
            } else if (GW(game_flags) & 0x20) {
                /* the last map: the ending and the credits */
                fptr path = make_path(1, 3, 0, -1);

                SW(draw_colour, 0);
                clear_page();
                check_vga_disk();
                play_anim(4, spare, 0, 0, 1, path);
                t0d36_000f(0x64);
                fade_out();
                t0d36_000f(0x12C);
                check_vga_disk();
                end_credits(work, path);
                t0d36_000f(0xC8);
                SW(draw_colour, 0);
                clear_page();
                flip_page();
                clear_page();
            }
            SW(game_flags, GW(game_flags) & 0x9FCF);
        }
        SW(draw_colour, 0);
        clear_page();
        flip_page();
        copy_page();
        t0d36_000f(0x0A);
    } while (GW(game_flags) & 1);
    t2354_056f();
    t261e_0008();
    t2354_005e();
}
