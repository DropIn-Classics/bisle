/* anim.c - BATTLE.EXE's T2248: the films of ANIM\: a building taken (bl
 * in the left half, br in the right), a headquarters blown up (qa), the
 * map won (hs), the last map's end (es, in five parts), each with its
 * sounds from ANIM.FX by a fixed plan.  A film is a file of frames
 * (T24C5:002E draws one over the picture before and gives the next), a
 * palette of 6-bit values goes with it.
 */
#include <string.h>
#include "bi.h"

#define NAME(off) MKFP(S_anim_names, A_anim_names + (off))

/* the path of a file of the films' folder into anim_path */
static void anim_file(fptr name)
{
    char *to = fstr(FP(anim_path));

    strcpy(to, fstr(GFP(anim_dir)));
    strcat(to, fstr(name));
}

/* T2248:005B: the tick count from 0 until it is above n */
static void wait_ticks(int n)
{
    SW(tick_count, 0);
    while ((int16_t)GW(tick_count) <= n)
        clock_idle();
}

/* T2248:008E: up to `frames` frames at x, y, each shown for `ticks` */
static void show(int frames, int ticks)
{
    int i;

    for (i = 0; GFP(anim_frame) != 0xFFFFFFFFu && i < frames; i++) {
        bi_at("anim_frame");
        fptr next = t24c5_002e(GW(anim_x), GW(anim_y), GFP(anim_frame));

        SFP(anim_frame, next);
        flip_page();
        copy_page();
        wait_ticks(ticks);
    }
}

/* T2248:00E3: a film and its palette loaded (when play_anim was told to
 * load); -1 when a file is missing */
static int load(unsigned film, unsigned palette)
{
    anim_file(NAME(film));
    if (GW(anim_load) && !load_file(GFP(anim_data), FP(anim_path), GFP(anim_work)))
        return -1;
    anim_file(NAME(palette));
    if (GW(anim_load) && !load_file(GFP(anim_palette), FP(anim_path), GFP(anim_work)))
        return -1;
    SFP(anim_frame, GFP(anim_data));
    return 0;
}

/* both pages black */
static void black(void)
{
    SW(draw_colour, 0);
    clear_page();
    flip_page();
    copy_page();
}

/* the film's palette (6-bit values) set */
static void palette(void)
{
    fptr p = GFP(anim_palette);
    unsigned i;

    for (i = 0; i < 0x300; i++)
        SBO(picture_palette, i, pb(p, i) << 2);
    set_palette(0xFF, FP(picture_palette));
}

static void fx(unsigned ch, unsigned number, unsigned volume, unsigned how)
{
    fptr e = GFP(effects_ptr);

    spw(e, 6 * ch, number);
    spw(e, 6 * ch + 2, volume);
    spw(e, 6 * ch + 4, how);
}

static void quiet(void)
{
    fptr e = GFP(effects_ptr);
    unsigned ch;

    for (ch = 0; ch < 4; ch++)
        spw(e, 6 * ch + 4, 0xFFFF);
    effects_start();
}

/* a building taken, in a half of the screen: the game's palette stays */
static int film_taken(int right)
{
    SW(draw_x1, GW(anim_x)), SW(draw_y1, GW(anim_y));
    SW(draw_x2, GW(anim_x) + 0x97), SW(draw_y2, GW(anim_y) + 0xB3);
    SW(draw_colour, 0);
    fill_rect();
    if (load(right ? 0x0E : 0, right ? 0x15 : 7))
        return -1;
    wait_ticks(0x14);
    fx(0, 0x10, 0x78, 1), effects_start();
    show(0x1E, 8);
    fx(0, 0x0F, 0, 1), effects_start();
    wait_ticks(0x64);
    fx(0, 0x0F, 0x78, 1), effects_start();
    show(0x3E8, 3);
    fx(0, 0x11, 0x78, 1), effects_start();
    wait_ticks(0x64);
    quiet();
    return 0;
}

/* a headquarters blown up */
static int film_hq(void)
{
    black();
    if (load(0x1C, 0x23))
        return -1;
    palette();
    fx(0, 0x0D, 0x78, 1), effects_start();
    show(7, 8);
    fx(0, 0x0E, 0x78, 1), effects_start();
    show(7, 8);
    fx(0, 0x0D, 0x78, 1), effects_start();
    show(0x28, 7);
    fx(0, 0x0E, 0x78, 1), fx(1, 0x0F, 0x78, 9), effects_start();
    wait_ticks(0xC8);
    quiet();
    return 0;
}

/* the map won */
static int film_won(void)
{
    black();
    if (load(0x2A, 0x31))
        return -1;
    palette();
    show(1, 0x1E);
    fx(0, 0x0C, 0x78, 1), effects_start();
    spw(GFP(effects_ptr), 2, 0x8000);
    for (SW(anim_count, 1); (int16_t)GW(anim_count) <= 0x2C; SW(anim_count, GW(anim_count) + 1)) {
        fx(1, 0x0B, 0x78, 1), effects_start();
        show(1, 5);
    }
    quiet();
    return 0;
}

/* the end of the last map, five films */
static int film_end(void)
{
    static const uint8_t louder[5] = {0x50, 0x5A, 0x64, 0x6E, 0x78};
    fptr e = GFP(effects_ptr);
    unsigned i;

    black();
    if (load(0x38, 0x3F))
        return -1;
    palette();
    fx(0, 0, 0x4B, 0x80), effects_start();
    spw(e, 8, 0x8000);
    spw(e, 2, 0x8000);
    show(1, 0x0D);
    wait_ticks(0x96);
    for (i = 0; i < 5; i++) {
        fx(2, 2, louder[i], 1), effects_start();
        show(3, 0x0D);
    }
    fx(2, 2, 0, 1), effects_start();
    black();
    if (load(0x47, 0x4E))
        return -1;
    palette();
    show(3, 0x0D);
    fx(2, 2, 0x6E, 1), effects_start();
    show(3, 0x0D);
    fx(2, 2, 0x78, 1), effects_start();
    show(3, 0x0D);
    fx(3, 3, 0x78, 1), fx(2, 2, 0x78, 1), effects_start();
    show(0x10, 0x0A);
    wait_ticks(0x2D);
    fx(3, 4, 0x7F, 1), effects_start();
    show(2, 7);
    fx(3, 5, 0x7F, 1), effects_start();
    show(2, 7);
    fx(3, 4, 0x7F, 1), effects_start();
    show(0x3E8, 8);
    wait_ticks(0xC8);
    black();
    if (load(0x56, 0x5D))
        return -1;
    palette();
    show(1, 1);
    wait_ticks(0xC8);
    fx(3, 7, 0x7F, 1), effects_start();
    show(0x1B, 3);
    fx(2, 6, 0x7F, 1), effects_start();
    wait_ticks(0xC8);
    black();
    if (load(0x65, 0x6C))
        return -1;
    palette();
    fx(0, 0, 0, 1), fx(1, 0, 0, 1), effects_start();
    show(1, 1);
    wait_ticks(0x64);
    for (SW(anim_count, 1); (int16_t)GW(anim_count) <= 0x1A; SW(anim_count, GW(anim_count) + 1)) {
        fx(0, 8, 0x46, 1), effects_start();
        show(1, 6);
    }
    wait_ticks(0x64);
    show(7, 5);
    wait_ticks(0x32);
    for (SW(anim_count, 1); (int16_t)GW(anim_count) <= 9; SW(anim_count, GW(anim_count) + 1)) {
        fx(0, 9, 0x50, 1), effects_start();
        show(3, 0x0D);
    }
    wait_ticks(0x64);
    black();
    if (load(0x74, 0x7B))
        return -1;
    palette();
    show(1, 0xC8);
    for (SW(anim_count, 1); (int16_t)GW(anim_count) < 0x0D; SW(anim_count, GW(anim_count) + 1)) {
        unsigned ch = GW(anim_count) & 3;

        fx(ch, 0x0A, 0x7F, 1), effects_start();
        spw(e, 6 * ch + 2, 0x8000);
        show(1, 2);
    }
    wait_ticks(0x64);
    quiet();
    return 0;
}

/* film `number` (0 and 1 a building taken, left and right; 2 a
 * headquarters blown up; 3 the map won; 4 the end) at x, y; `buffer`
 * takes the sounds (1000h in), the palette (1800h) and the film
 * (1B20h); with `load` 0 nothing is loaded again; `dir` is the folder */
void play_anim(int number, fptr buffer, int x, int y, int load_files, fptr dir)
{
    quiet();
    SW(anim_load, load_files);
    SW(anim_x, x);
    SW(anim_y, y);
    SFP(anim_dir, dir);
    SFP(anim_work, buffer);
    SFP(anim_palette, MKFP(FSEG(buffer), FOFF(buffer) + 0x1800));
    SFP(anim_data, MKFP(FSEG(buffer), FOFF(buffer) + 0x1B20));
    anim_file(NAME(0x83));
    if (GW(anim_load) && !load_effects(MKFP(FSEG(buffer), FOFF(buffer) + 0x1000), FP(anim_path), GFP(anim_work)))
        return;
    switch (number) {
    case 0:
    case 1:
        film_taken(number);
        break;
    case 2:
        film_hq();
        break;
    case 3:
        film_won();
        break;
    case 4:
        film_end();
        break;
    default:
        SW(anim_x, number);
        break;
    }
}
