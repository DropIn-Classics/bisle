/* title.c - BATTLE.EXE's T1727: the Blue Byte logo and the title with
 * its running lines, until space. */
#include <string.h>
#include "bi.h"

int bi_skip_intro, bi_quit_yz;

/* the key set of the keyboard and twelve of its keys copied, as the title
 * does when space ends it (the key set of the other, what for is not known,
 * is chosen when it is not the keyboard's space) */
static void title_key_set(fptr set)
{
    static const uint8_t from[12] = { 8, 8, 9, 0x0B, 0x0A, 6, 0x1A, 0x1E, 0x1C, 0x1B, 0x1D, 1 };
    int i;

    SFP(key_set, set);
    for (i = 0; i < 12; i++)
        SBO(key_set_copy, i, pb(set, from[i]));
}

/* the title left out (setup screen): what it leaves behind that the rest
 * of the program uses is the key set and a cleared page, the picture's
 * palette being set by the menu's own picture */
int title_skip(void)
{
    title_key_set(FP(key_set_keyboard));
    fade_out();
    restore_sprites();
    SW(draw_colour, 0);
    clear_page();
    flip_page();
    restore_sprites();
    return 0;
}

int title(fptr work)
{
    fptr p = hadd(work, 0x2710), path, text, line[3], at, picture;
    int y[3], x[3], i;
    long size;

    SW(clip_on, 1);
    SW(clip_x1, 0);
    SW(clip_y1, 0);
    SW(clip_x2, 0x140);
    SW(clip_y2, 0xC8);
    /* BB.IFF's name is kept while BB.DAT's is made: on the stack in the
     * original, here in the megabyte below the program, which nothing
     * else uses */
    picture = MKFP(0x0050, 0);
    strcpy(fstr(picture), fstr(make_path(1, -1, FP(name_bb), 0x0A)));
    path = make_path(1, -1, FP(name_bb), 5);
    t2433_0006(p, picture, path, work);
    t0d36_000f(0x64);
    fade_out();
    path = make_path(1, -1, FP(name_titel), 0x0A);
    load_picture(p, path, work, NULL, NULL);
    path = make_path(1, 0, FP(lib_char24), 3);
    size = load_lib(path, p, work, FP(lib_char24), 0);
    if (size == -1)
        return -1;
    p = hadd(p, (long)pd(FP(lib_char24), 0x0A));
    path = make_path(1, -1, FP(name_titel), 6);
    size = t2624_0006(p, path, work, NULL);
    if (size == 0)
        return -1;
    text = p;
    p = hadd(p, size);
    path = make_path(1, -1, FP(name_titel), 7);
    if (!load_song(p, path, work))
        return -1;
    play_song(0, 0);
    flip_page();
    copy_page();
    fade_in();
    SW(clip_on, 1);
    SW(clip_x1, 0);
    SW(clip_y1, 0x84);
    SW(clip_x2, 0x140);
    SW(clip_y2, 0xC7);
    /* three lines run up the screen, 34 rows apart, a row every three
     * ticks */
    at = text;
    line[0] = at;
    at = next_line(at, text);
    line[1] = at;
    at = next_line(at, text);
    line[2] = at;
    y[0] = 0xC8;
    y[1] = 0xEA;
    y[2] = 0x10C;
    for (;;) {
        bi_at("title_pass");
        SW(tick_count, 0);
        SW(key_there, 0);
        for (i = 0; i < 3; i++)
            x[i] = 0xA0 - ((int)strlen(fstr(line[i])) * 0x18 >> 1);
        restore_sprites();
        for (i = 0; i < 3; i++)
            draw_text24(x[i], y[i]--, line[i], 1, 0);
        flip_page();
        while ((int16_t)GW(tick_count) < 3)
            clock_idle();
        for (i = 0; i < 3; i++)
            if (y[i] <= 0x62) {
                y[i] = 0xC8;
                at = next_line(at, text);
                line[i] = at;
                break;
            }
        if (GW(key_there) && GB(key_char) == 0x20) {
            title_key_set(GB(key_scan) == 0x39 ? FP(key_set_keyboard) : FP(key_set_other));
            SW(key_there, 0);
            break;
        }
    }
    fade_out();
    stop_song();
    restore_sprites();
    SW(draw_colour, 0);
    clear_page();
    flip_page();
    restore_sprites();
    return 0;
}
