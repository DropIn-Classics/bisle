/* text.c - BATTLE.EXE's T164D: text in the large letters. */
#include <string.h>
#include "bi.h"

/* the text (ended by 0) in CHAR24.LIB's letters from x, y on, 24 pixels a
 * letter: a byte is the entry's number plus 3, 1 a gap; keep_off,
 * keep_seg as draw_entry's.  Returns the x after it. */
int draw_text24(int x, int y, fptr text, unsigned keep_off, unsigned keep_seg)
{
    fptr entries = pfp(FP(lib_char24), 0x0E);
    unsigned c, i;

    for (i = 0; (c = pb(text, i)) != 0; i++) {
        if (c != 1)
            draw_entry(x, y, pfp(entries, 4 * ((c - 3) & 0xFF)), keep_off, keep_seg, 0);
        x += 0x18;
    }
    return x;
}

/* the line after the one at `at`; after the last (a 2 follows it) the
 * first again */
fptr next_line(fptr at, fptr start)
{
    at = MKFP(FSEG(at), FOFF(at) + strlen(fstr(at)) + 1);
    if (pb(at, 0) == 2)
        at = start;
    return at;
}

/* a text made on the stack in the original: in the megabyte below the
 * program here */
#define NUMBER_SCRATCH MKFP(0x0050, 0x00A0)

/* text `n` of GAME.TXT (each text its lines ended by 0, then 2 and a
 * byte) at x, y in `colour`, 6 rows a line */
void draw_text(int x, int y, int n, int colour)
{
    fptr p = GFP(game_txt);

    SW(draw_colour, colour);
    for (; n > 0; n--) {
        while (pb(p, 0) != 2)
            p = MKFP(FSEG(p), FOFF(p) + 1);
        p = MKFP(FSEG(p), FOFF(p) + 2);
    }
    while (pb(p, 0) != 2) {
        draw_chars(x, y, p);
        while (pb(p, 0))
            p = MKFP(FSEG(p), FOFF(p) + 1);
        p = MKFP(FSEG(p), FOFF(p) + 1);
        y += 6;
    }
}

/* a number in decimal in the small font, in the drawing record's colour */
void draw_number(int n, int x, int y)
{
    char digits[12];
    int count = 0, i;

    do {
        digits[count++] = (char)(n % 10 + 0x30);
        n /= 10;
    } while (n);
    for (i = 0; i < count; i++)
        spb(NUMBER_SCRATCH, (unsigned)i, digits[count - 1 - i]);
    spb(NUMBER_SCRATCH, (unsigned)count, 0);
    draw_chars(x, y, NUMBER_SCRATCH);
}

/* a bar at x, y, `rows` high: half of `full` pixels long in the colour
 * `back` (not for 0), over it half of `n` in the colour `front` (when n
 * is within full) */
void draw_bar(int x, int y, int front, int back, int rows, int n, int full)
{
    full >>= 1;
    n >>= 1;
    if (back) {
        SW(draw_x1, x), SW(draw_y1, y), SW(draw_x2, x + full), SW(draw_y2, y + rows);
        SW(draw_colour, back);
        SWO(clip_on, 2, 1);
        fill_rect();
    }
    if (n <= 0 || n > full)
        return;
    SW(draw_x1, x), SW(draw_y1, y), SW(draw_x2, x + n), SW(draw_y2, y + rows);
    SW(draw_colour, front);
    SWO(clip_on, 2, 1);
    fill_rect();
}

/* BIDISK.VGA loaded (into the first player's buffer; the second's is the
 * work buffer), or the end: the original's check for its disk */
void check_vga_disk(void)
{
    if (!load_file(pfp(FP(players), 9), FP(name_bidisk), pfp(FP(players), 0x17 + 9)))
        fatal_error(2);
}
