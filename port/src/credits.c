/* credits.c - BATTLE.EXE's T25A6: the text typed on the screen after the
 * last map's end: green letters with a blinking cursor, a sound a
 * letter, a page at a time (0Dh ends a line, 0Ch a page, 0 the text),
 * each page fading out; Esc ends it; then a white picture that closes
 * to a line and a dot.
 */
#include <setjmp.h>
#include <string.h>
#include "bi.h"

static jmp_buf escape;

/* T25A6:000D: n ticks; Esc (from the BIOS's keyboard) ends the text */
static void ticks(int n)
{
    SW(tick_count, 0);
    while ((int16_t)GW(tick_count) < n) {
        if (bios_key_waits() && (bios_key() & 0xFF) == 0x1B)
            longjmp(escape, 1);
        clock_idle();
    }
}

static void cursor_rect(void)
{
    SW(draw_x1, GW(credits_cursor_x) + 1), SW(draw_y1, GW(credits_cursor_y));
    SW(draw_x2, GW(credits_cursor_x) + 5), SW(draw_y2, GW(credits_cursor_y) + 4);
    fill_rect();
}

/* T25A6:0064: n + 1 ticks with the cursor blinking, every 7th */
static void blink(int n)
{
    int i;

    for (i = 0; i <= n; i++) {
        unsigned count;

        ticks(1);
        count = GW(credits_count);
        SW(credits_count, count - 1);
        if (count)
            continue;
        SW(credits_count, 6);
        SW(draw_colour, GW(credits_blink) ? 0 : 1);
        if (bios_key_waits() && (bios_key() & 0xFF) == 0x1B)
            longjmp(escape, 1);
        SW(credits_blink, 1 - GW(credits_blink));
        cursor_rect();
    }
}

/* colour 0 black, colour 1 green */
static void colours(void)
{
    set_dac_entry(0, 0, 0, 0);
    set_dac_entry(1, 0, 0x3C, 0);
}

/* T25A6:0339: a page fades out through violet, with a sound */
static void page_out(void)
{
    fptr e = GFP(effects_ptr);
    int v;

    spw(e, 0, 1);
    spw(e, 2, 0x64);
    spw(e, 4, 1);
    effects_start();
    clear_page();
    for (v = 0; v <= 0x3F; v++) {
        set_dac_entry(1, v, 0x3F - v, v);
        ticks(1);
    }
    flip_page();
    colours();
}

/* T25A6:03B8: a letter typed, the cursor behind it */
static void letter(unsigned c)
{
    fptr e = GFP(effects_ptr), s = MKFP(0x0050, 0x00D0);

    bi_at("credits_letter");
    SW(draw_colour, 0);
    cursor_rect();
    SW(draw_colour, 1);
    spb(s, 0, c);
    spb(s, 1, 0);
    spw(e, 0, 0);
    spw(e, 2, 0x46);
    spw(e, 4, 1);
    effects_start();
    draw_chars((int16_t)(GW(credits_x) - 6), (int16_t)GW(credits_y), s);
    SW(credits_cursor_x, GW(credits_x));
    SW(credits_cursor_y, GW(credits_y));
    cursor_rect();
    SW(credits_x, GW(credits_x) + 6);
}

/* T25A6:04ED: the text, each page in the middle of the screen and each
 * line in the middle of its row */
static void type_text(fptr text)
{
    unsigned i = 0, j, n;

    while (pb(text, i)) {
        for (j = i, n = 0; pb(text, j) != 0x0C; j++)
            if (pb(text, j) == 0x0D)
                n++;
        SW(credits_y, (0xC8 - (int)(6 * n)) / 2);
        while (pb(text, i) != 0x0C) {
            for (j = i, n = 0; pb(text, j) != 0x0C && pb(text, j) != 0x0D; j++)
                n++;
            SW(credits_x, (0x140 - (int)(6 * n)) / 2);
            for (; pb(text, i) != 0x0C && pb(text, i) != 0x0D; i++) {
                letter(pb(text, i));
                ticks(4);
            }
            if (pb(text, i) == 0x0D) {
                for (; pb(text, i) == 0x0D; i++)
                    SW(credits_y, GW(credits_y) + 6);
                blink(0x3C);
            }
        }
        blink(0x190);
        SW(page_drawn, GW(credits_page));
        SW(draw_colour, 0);
        page_out();
        ticks(0x64);
        SW(credits_page, GW(page_drawn));
        SW(page_drawn, GW(page_shown));
        i++;
    }
}

/* T25A6:0142: white, closing to a line, to a dot, gone */
static void close_picture(void)
{
    int x1 = 0, y1 = 0, x2 = 0x140, y2 = 0xC8;

    set_dac_entry(1, 0x3F, 0x3F, 0x3F);
    for (; y2 > y1; y1 += 10, y2 -= 10, x1 += 10, x2 -= 10) {
        SW(draw_x1, x1), SW(draw_y1, y1), SW(draw_x2, x2), SW(draw_y2, y2);
        SW(draw_colour, 1);
        fill_rect();
        ticks(1);
        SW(draw_colour, 0);
        clear_page();
        flip_page();
        copy_page();
        ticks(1);
    }
    for (; x2 > x1; x1 += 2, x2 -= 2) {
        SW(draw_x1, x1), SW(draw_y1, 0x64), SW(draw_x2, x2), SW(draw_y2, 0x64);
        SW(draw_colour, 1);
        fill_rect();
        flip_page();
        SW(draw_colour, 0);
        clear_page();
        flip_page();
        ticks(1);
    }
    SW(draw_x1, 0xA0), SW(draw_y1, 0x64), SW(draw_x2, 0xA0), SW(draw_y2, 0x64);
    SW(draw_colour, 1);
    fill_rect();
    flip_page();
    ticks(0x12C);
    SW(draw_colour, 0);
    clear_page();
    flip_page();
}

/* the text of the end, typed onto the page shown, with the sounds of the
 * file in `dir` (loaded 1000h into `work`) */
void end_credits(fptr work, fptr dir)
{
    fptr path = MKFP(0x0050, 0x00E0);

    strcpy(fstr(path), fstr(dir));
    strcat(fstr(path), fstr(FP(credits_name)));
    clear_page();
    colours();
    flip_page();
    clear_page();
    if (!load_effects(MKFP(FSEG(work), FOFF(work) + 0x1000), path, work))
        return;
    SW(credits_page, GW(page_drawn));
    SW(page_drawn, GW(page_shown));
    if (!setjmp(escape))
        type_text(FP(credits_text));
    close_picture();
}
