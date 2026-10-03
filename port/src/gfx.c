/* gfx.c - BATTLE.EXE's T23DC..T259F: the screen.
 *
 * The program draws in the VGA's 320x200 mode with the four planes
 * unchained: a pixel is a byte in plane x & 3 at 80 * y + x / 4, a page
 * is 3E80h bytes of each plane.  Two pages (segments A000 and A400), one
 * shown and one drawn to; behind them (A7E8) two lists of what the
 * sprites of each page covered, and behind those (AC58) the parts of the
 * ground, copied from there with the card's latches.  All of it goes
 * through doskit's vga.c, by the same ports the original used, so the
 * video memory can be compared with a run's.
 *
 * The original's routines keep their scratch values in their code
 * segments; they are C variables here.
 */
#include <string.h>
#include "bi.h"
#include "vga.h"

/* ---- the card ---- */

static uint16_t vofs(uint16_t seg, uint16_t off)
{
    return (uint16_t)(((seg - 0xA000u) << 4) + off);
}
static void vwb(uint16_t seg, uint16_t off, unsigned v) { vga_write(vofs(seg, off), (uint8_t)v); }
static uint8_t vrb(uint16_t seg, uint16_t off) { return vga_read(vofs(seg, off)); }
static void vww(uint16_t seg, uint16_t off, unsigned v)
{
    vwb(seg, off, v);
    vwb(seg, (uint16_t)(off + 1), v >> 8);
}
static uint16_t vrw(uint16_t seg, uint16_t off)
{
    return (uint16_t)(vrb(seg, off) | vrb(seg, (uint16_t)(off + 1)) << 8);
}
/* the planes written (the sequencer's map mask) */
static void planes(unsigned m) { vga_outw(0x3C4, (uint16_t)(m << 8 | 2)); }
/* write mode 1, a copy through the latches, or mode 0 */
static void latches(int on) { vga_outw(0x3CE, on ? 0x4105 : 0x4005); }

/* T23DC:000A: where a pixel is: the offset in a page, the plane's bit
 * (twice, in both nibbles, so that it can be rotated) */
static unsigned pos(unsigned x, unsigned y, uint16_t *di)
{
    *di = (uint16_t)(rw((uint16_t)(A_row_offsets + 2 * y)) + ((uint16_t)x >> 2));
    return 0x11u << (x & 3) & 0xFF;
}

/* ROL of the plane's bit: 1 when it went round to plane 0 */
static int next_plane(unsigned *m)
{
    int carry = (*m & 0x80) != 0;

    *m = ((*m << 1) | (unsigned)carry) & 0xFF;
    return carry;
}

/* T258F:0002: of `w` pixels in a row, how many are in each plane (from
 * the first pixel's plane on) */
static void plane_widths(unsigned w, uint16_t out[4])
{
    unsigned i;

    for (i = 0; i < 4; i++)
        out[i] = (uint16_t)((w >> 2) + (i < (w & 3)));
}

/* ---- the mode and the pages ---- */

/* T2593:0006: the two pages' segments, the lists of what the sprites
 * covered (one a page; behind the pages), the parts' store */
int t2593_0006(void)
{
    unsigned drawn, lists, parts;

    if ((int16_t)GW(screen_height) >= 0xF0) {
        drawn = 0xA547;
        lists = 0xAA8E;
    } else {
        drawn = 0xA400;
        lists = 0xA7E8;
    }
    parts = 0xAC58;
    if (!GW(two_pages)) {
        SWO(covered_start, 0, 4);
        SWO(covered_start_other, 0, 4);
        SWO(covered_end, 0, 4);
        SWO(covered_end_other, 0, 4);
        SWO(covered_start, 2, lists);
        SWO(covered_start_other, 2, lists);
        SWO(covered_end, 2, lists);
        SWO(covered_end_other, 2, lists);
        SW(page_drawn, 0xA000);
        SW(page_shown, 0xA000);
    } else {
        SWO(covered_start, 2, lists);
        SWO(covered_end, 2, lists);
        SWO(covered_start_other, 2, lists);
        SWO(covered_end_other, 2, lists);
        SWO(covered_start, 0, 4);
        SWO(covered_end, 0, 4);
        if (GW(one_page)) {
            SWO(covered_start_other, 0, 4);
            SWO(covered_end_other, 0, 4);
        } else {
            SWO(covered_start_other, 0, GW(covered_second));
            SWO(covered_end_other, 0, GW(covered_second));
        }
        SW(page_drawn, drawn);
        SW(page_shown, 0xA000);
    }
    SWO(parts_next, 0, 0);
    SWO(parts_next, 2, parts);
    SWO(pointer_store, 0, 0);
    SWO(pointer_store, 2, parts - 0x0A);
    return 0;
}

/* T2485:0001: the mode (the port has the 320 by 200 one: the others the
 * routine knows, 360 by 240 and 360 by 480, the program does not ask
 * for: its only call, in T0708 (battle.c here), passes
 * 140h and C8h), the CRTC's ports, the pages */
int t2485_0001(int width, int height, int unused, int second, int unused2, int pages)
{
    unsigned i;

    (void)unused;
    (void)width;
    (void)height;
    SW(old_video_mode, 0x5003);         /* INT 10h AH=0Fh: mode 3, 80 columns */
    vga_set_mode(0x13);
    vga_outb(0x3C4, 4);                 /* the planes unchained */
    vga_outb(0x3C5, vga_inb(0x3C5) & 0xF7);
    vga_outb(0x3D4, 0x11);
    vga_outb(0x3D5, vga_inb(0x3D5) & 0x7F);
    vga_outw(0x3D4, 0xC317);            /* byte mode */
    vga_outw(0x3D4, 0x0014);
    SW(screen_width, 0x140);
    SW(screen_height, 0xC8);
    SWO(screen_height, 2, 8);
    SWO(screen_height, 4, 7);
    SWO(row_bytes, -2, 0x28);
    SW(row_bytes, 0x50);
    SW(text_scale, 1);
    SW(page_bytes, 0x3E80);
    SWO(page_bytes, 2, 0);
    SWO(page_bytes, 4, 0xFA00);
    SWO(page_bytes, 6, 0);
    for (i = 0; i < 0xF0; i++)
        SWO(row_offsets, 2 * i, i * GW(row_bytes));
    SW(crtc_port, 0x3D4);
    SW(crtc_data_port, 0x3D5);
    SW(status_port, 0x3DA);
    SW(covered_second, second);
    SWO(covered_second, 2, unused2);
    SW(two_pages, pages);
    return t2593_0006();
}

void wait_retrace(void)
{
    clock_retrace();
}

/* T0D36:000F: n + 1 retraces */
void t0d36_000f(int retraces)
{
    wait_retrace();
    while (retraces-- > 0)
        wait_retrace();
}

/* the page drawn to is shown and the other drawn to, from the next
 * retrace on; the lists of what the sprites covered go with their pages */
void flip_page(void)
{
    unsigned t, start;

    if (!GW(one_page)) {
        t = GWO(covered_start, 2), SWO(covered_start, 2, GWO(covered_start_other, 2)), SWO(covered_start_other, 2, t);
        t = GWO(covered_start, 0), SWO(covered_start, 0, GWO(covered_start_other, 0)), SWO(covered_start_other, 0, t);
        t = GWO(covered_end, 2), SWO(covered_end, 2, GWO(covered_end_other, 2)), SWO(covered_end_other, 2, t);
        t = GWO(covered_end, 0), SWO(covered_end, 0, GWO(covered_end_other, 0)), SWO(covered_end_other, 0, t);
    }
    t = GW(page_shown);
    SW(page_shown, GW(page_drawn));
    SW(page_drawn, t);
    start = (uint16_t)(GW(page_shown) << 4);
    vga_outw(0x3D4, (uint16_t)((start & 0xFF00) | 0x0C));
    vga_outw(0x3D4, (uint16_t)((start & 0xFF) << 8 | 0x0D));
    wait_retrace();
}

void clear_page(void)
{
    uint16_t seg = GW(page_drawn);
    unsigned i, n = GW(page_bytes) & ~1u;

    planes(0x0F);
    for (i = 0; i < n; i++)
        vwb(seg, (uint16_t)i, GB(draw_colour));
}

/* the page shown copied to the page drawn to */
void copy_page(void)
{
    uint16_t to = GW(page_drawn), from = GW(page_shown);
    unsigned i, n = GW(page_bytes);

    planes(0x0F);
    latches(1);
    for (i = 0; i < n; i++) {
        vrb(from, (uint16_t)i);
        vwb(to, (uint16_t)i, 0);
    }
    latches(0);
}

/* T2590:000A: the page shown is drawn to (kept: which page was), and
 * T2592:0002: the pages as they were */
void t2590_000a(void)
{
    SW(page_drawn_kept, GW(page_drawn));
    SW(page_shown_kept, GW(page_shown));
    SW(page_drawn, GW(page_shown));
}

void t2592_0002(void)
{
    SW(page_shown, GW(page_shown_kept));
    SW(page_drawn, GW(page_drawn_kept));
}

/* ---- what the sprites covered ---- */

/* T2515:0008: `w` by `h` pixels of the screen at `at` kept before a
 * sprite is drawn there: in the list of the page drawn to (keep_seg 0),
 * or at keep_seg:keep_off.  An item: the place, the rows, the bytes of a
 * row, then the pixels of all planes (copied with the latches), then the
 * same three words and where the pixels are, for the way back. */
static void keep_covered(unsigned w, unsigned h, uint16_t at_seg, uint16_t at, unsigned keep_off,
                         unsigned keep_seg)
{
    uint16_t seg, di, si = at, data;
    unsigned bytes = (w >> 2) + 2, skip = GW(row_bytes) - bytes, y, x;

    if (!keep_seg) {
        seg = GWO(covered_end, 2);
        di = GWO(covered_end, 0);
    } else {
        seg = (uint16_t)keep_seg;
        di = (uint16_t)keep_off;
    }
    planes(0x0F);
    latches(0);
    vww(seg, di, si);
    vww(seg, (uint16_t)(di + 2), h);
    vww(seg, (uint16_t)(di + 4), bytes);
    di += 6;
    data = di;
    latches(1);
    for (y = 0; y < h; y++) {
        for (x = 0; x < bytes; x++) {
            vrb(at_seg, si++);
            vwb(seg, di++, 0);
        }
        si = (uint16_t)(si + skip);
    }
    latches(0);
    vww(seg, di, at);
    vww(seg, (uint16_t)(di + 2), h);
    vww(seg, (uint16_t)(di + 4), bytes);
    vww(seg, (uint16_t)(di + 6), data);
    di += 8;
    if (!keep_seg)
        SWO(covered_end, 0, di);
}

/* T24D3:0000: one kept item (keep_covered's, at keep_seg:keep_off) put
 * back on the page drawn to */
void t24d3_0000(fptr item)
{
    uint16_t page = GW(page_drawn), seg = FSEG(item), si = FOFF(item);
    uint16_t di = vrw(seg, si);
    unsigned rows = vrw(seg, (uint16_t)(si + 2)), bytes = vrw(seg, (uint16_t)(si + 4)), x;

    si += 6;
    planes(0x0F);
    latches(1);
    do {
        for (x = 0; x < bytes; x++) {
            vrb(seg, si++);
            vwb(page, di++, 0);
        }
        di = (uint16_t)(di + GW(row_bytes) - bytes);
    } while (--rows);
    latches(0);
}

/* what the sprites drawn to this page covered is put back, the last
 * first, and the list is empty again */
void restore_sprites(void)
{
    uint16_t page = GW(page_drawn), seg = GWO(covered_end, 2);
    uint16_t bp = GWO(covered_end, 0), start = GWO(covered_start, 0);
    unsigned rowb = GW(row_bytes);

    planes(0x0F);
    latches(1);
    while ((int16_t)start < (int16_t)bp) {
        uint16_t si = vrw(seg, (uint16_t)(bp - 2)), di = vrw(seg, (uint16_t)(bp - 8));
        unsigned bytes = vrw(seg, (uint16_t)(bp - 4)), rows = vrw(seg, (uint16_t)(bp - 6)), x;

        if (!si) {
            /* T2467:007B: an item of 14 bytes, a block copied from the
             * page at A400h to the same place (T2475:0006: rows of 28h
             * bytes; it leaves the latches off for the items after it, as
             * here).  Nothing in BATTLE.EXE was found to make one (read in
             * BATTLE.ASM, not run). */
            latches(1);
            do {
                for (x = 0; x < bytes; x++) {
                    vrb(0xA400, di);
                    vwb(page, di++, 0);
                }
                di = (uint16_t)(di + 0x28 - bytes);
            } while (--rows);
            latches(0);
            bp = (uint16_t)(bp - 0x0E);
            continue;
        }
        do {
            for (x = 0; x < bytes; x++) {
                vrb(seg, si++);
                vwb(page, di++, 0);
            }
            di = (uint16_t)(di + rowb - bytes);
        } while (--rows);
        bp = (uint16_t)(vrw(seg, (uint16_t)(bp - 2)) - 6);
    }
    SWO(covered_end, 0, start);
    latches(0);
}

/* ---- the palette ---- */

/* the 256 colours of `palette` (8 bits each) at `level` of 256.  The
 * original waits for the display's blank after each colour with the
 * interrupts off: 256 scan lines. */
void set_palette(int level, fptr palette)
{
    unsigned i, l = (unsigned)level & 0xFF;

    vga_outb(0x3C8, 0);
    for (i = 0; i < 768; i++)
        vga_outb(0x3C9, (uint8_t)((pb(palette, i) * l) >> 10));
    clock_lines(256);
}

void set_dac_entry(int index, int r, int g, int b)
{
    vga_outb(0x3C8, (uint8_t)index);
    vga_outb(0x3C9, (uint8_t)r);
    vga_outb(0x3C9, (uint8_t)g);
    vga_outb(0x3C9, (uint8_t)b);
}

void fade_in(void)
{
    int level;

    for (level = 0; level < 0x100; level += 4)
        set_palette(level, FP(picture_palette));
}

void fade_out(void)
{
    int level;

    for (level = 0xFF; level >= 0; level -= 4)
        set_palette(level, FP(picture_palette));
}

/* ---- rows, columns, rectangles, lines ---- */

void draw_row(void)
{
    uint16_t seg = GW(page_drawn), di;
    unsigned x1 = GW(draw_x1), m, colour;
    int left;

    if ((int16_t)x1 > (int16_t)GW(draw_x2)) {
        SW(draw_x1, GW(draw_x2));
        SW(draw_x2, x1);
        x1 = GW(draw_x1);
    }
    pos(x1, GW(draw_y1), &di);
    colour = GB(draw_colour);
    left = (int16_t)(GW(draw_x2) - GW(draw_x1) + 1);
    m = 0x0F << (x1 & 3) & 0x0F;
    left -= 4 - (int)(x1 & 3);
    if (left <= 0) {                    /* all in one byte */
        planes(m & ((2u << (GW(draw_x2) & 3)) - 1));
        vwb(seg, di, colour);
        return;
    }
    planes(m);
    vwb(seg, di++, colour);
    planes(0x0F);
    for (; left >= 4; left -= 4)
        vwb(seg, di++, colour);
    if (left) {
        planes((2u << (GW(draw_x2) & 3)) - 1);
        vwb(seg, di, colour);
    }
}

void draw_column(void)
{
    uint16_t seg = GW(page_drawn), di;
    unsigned y1 = GW(draw_y1), m, n;

    if ((int16_t)y1 > (int16_t)GW(draw_y2)) {
        SW(draw_y1, GW(draw_y2));
        SW(draw_y2, y1);
        y1 = GW(draw_y1);
    }
    m = pos(GW(draw_x1), y1, &di);
    planes(m);
    n = (uint16_t)(GW(draw_y2) - GW(draw_y1) + 1);
    do {
        vwb(seg, di, GB(draw_colour));
        di = (uint16_t)(di + GW(row_bytes));
    } while (--n & 0xFFFF);
}

/* the rectangle of the drawing record filled; its first row is then the
 * one below it */
void fill_rect(void)
{
    unsigned n;

    if (GW(draw_y2) < GW(draw_y1)) {
        unsigned t = GW(draw_y1);
        SW(draw_y1, GW(draw_y2));
        SW(draw_y2, t);
    }
    n = (uint16_t)(GW(draw_y2) - GW(draw_y1) + 1);
    do {
        draw_row();
        SW(draw_y1, GW(draw_y1) + 1);
    } while (--n & 0xFFFF);
}

void put_pixel(void)
{
    uint16_t di;
    unsigned m = pos(GW(draw_x1), GW(draw_y1), &di);

    vga_outb(0x3C4, 2);
    vga_outb(0x3C5, (uint8_t)m);
    vwb(GW(page_drawn), di, GB(draw_colour));
}

/* a line from x1, y1 to x2, y2 of the drawing record.  As the original:
 * a line steeper than 45 degrees ends a pixel before its end. */
void draw_line(void)
{
    uint16_t seg = GW(page_drawn), di;
    int dx = (int16_t)(GW(draw_x2) - GW(draw_x1)), dy, step;
    unsigned m, err = 0, frac, count, colour = GB(draw_colour);
    int steep;

    if (dx == 0) {
        draw_column();
        return;
    }
    if (dx < 0) {
        unsigned t = GW(draw_x1);
        SW(draw_x1, GW(draw_x2));
        SW(draw_x2, t);
        t = GW(draw_y1);
        SW(draw_y1, GW(draw_y2));
        SW(draw_y2, t);
        dx = (int16_t)-dx;
    }
    dy = (int16_t)(GW(draw_y2) - GW(draw_y1));
    if (dy == 0) {
        draw_row();
        return;
    }
    if (dy < 0) {
        dy = (int16_t)-dy;
        step = -(int)GW(row_bytes);
        steep = (int16_t)(dx - dy) < 0;
        if (!steep && dx == dy)
            dx++;
    } else {
        step = (int)GW(row_bytes);
        steep = (int16_t)(dx - dy) < 0;
        if (!steep && dx == dy) {
            dy++;
            steep = 1;
        }
    }
    m = pos(GW(draw_x1), GW(draw_y1), &di);
    vga_outb(0x3C4, 2);
    if (steep) {
        frac = (uint16_t)(((uint32_t)(uint16_t)dx << 16) / (uint16_t)dy);
        count = (uint16_t)dy;
        vga_outb(0x3C5, (uint8_t)m);
        do {
            vwb(seg, di, colour);
            di = (uint16_t)(di + step);
            err += frac;
            if (err > 0xFFFF) {
                err &= 0xFFFF;
                di = (uint16_t)(di + next_plane(&m));
                vga_outb(0x3C5, (uint8_t)m);
            }
        } while (--count & 0xFFFF);
    } else {
        frac = (uint16_t)(((uint32_t)(uint16_t)dy << 16) / (uint16_t)dx);
        count = (uint16_t)dx;
        do {
            vga_outb(0x3C5, (uint8_t)m);
            vwb(seg, di, colour);
            di = (uint16_t)(di + next_plane(&m));
            err += frac;
            if (err > 0xFFFF) {
                err &= 0xFFFF;
                di = (uint16_t)(di + step);
            }
        } while (--count & 0xFFFF);
    }
}

/* T24A8:0008: a box: filled, a column left and a row above in `light`, a
 * column right and a row below in `dark` */
void t24a8_0008(int x1, int y1, int x2, int y2, int light, int dark, int fill)
{
    int t;

    if ((uint16_t)x1 >= (uint16_t)x2)
        t = x1, x1 = x2, x2 = t;
    if ((uint16_t)y1 >= (uint16_t)y2)
        t = y1, y1 = y2, y2 = t;
    SW(draw_x1, x1), SW(draw_y1, y1), SW(draw_x2, x2), SW(draw_y2, y2);
    SW(draw_colour, fill);
    fill_rect();
    SW(draw_x1, x1), SW(draw_y1, y1), SW(draw_y2, y2 - 1);
    SW(draw_colour, light);
    draw_column();
    SW(draw_x1, x2), SW(draw_y1, y1 + 1), SW(draw_y2, y2);
    SW(draw_colour, dark);
    draw_column();
    SW(draw_x1, x1), SW(draw_y1, y1), SW(draw_x2, x2 - 1);
    SW(draw_colour, light);
    draw_row();
    SW(draw_x1, x1 + 1), SW(draw_y1, y2), SW(draw_x2, x2);
    SW(draw_colour, dark);
    draw_row();
}

/* ---- the libraries' entries ---- */

/* An entry: +8 the value that is not drawn, +9 the kind, +0Ah, +0Ch what
 * is added to x and y, +0Eh the width, +10h the height, +12h the pixels,
 * plane by plane.  draw_entry draws one at x, y with `base` added to its
 * values; with keep_off or keep_seg not 0 what it covers is kept first
 * (keep_covered: 1, 0 for the page's list).  With clip_on it is cut to
 * the rectangle clip_x1.. (the lower and the right edge left out). */

/* T23DE:000A: kind 'P', two pixels a byte, the high nibble first; the
 * width counts the bytes of two rows of a plane */
static void draw_entry_p(int x, int y, fptr e, unsigned keep_off, unsigned keep_seg, int base)
{
    uint16_t seg = GW(page_drawn), di, d;
    unsigned w4 = pw(e, 0x0E) >> 2, w = 2 * w4, h = pw(e, 0x10), m, plane, row, i;
    unsigned skip_first = 0, skip_plane = 0, si = 0x12, transparent, add;
    int ax = (int16_t)(x + pw(e, 0x0A)), bx = (int16_t)(y + pw(e, 0x0C));

    if (GW(clip_on)) {
        /* T23DE:0135; as the original: cut above and below only, moved to
         * the left edge when it begins left of it */
        int cx = (int)w, dx = (int)h, t;
        int x1 = (int16_t)GW(clip_x1), y1 = (int16_t)GW(clip_y1);
        int x2 = (int16_t)GW(clip_x2), y2 = (int16_t)GW(clip_y2);

        if (ax < x1) {
            cx -= x1 - ax;
            if (cx <= 0)
                return;
            ax = x1;
        } else if (ax > x2)
            return;
        if (bx < y1) {
            t = y1 - bx;
            dx -= t;
            if (dx <= 0)
                return;
            bx = y1;
        } else if (bx > y2)
            return;
        if (dx + bx > y2) {
            dx -= dx + bx - y2;
            if (dx == 0)
                return;
        }
        if (cx + ax > x2) {
            cx -= cx + ax - x2;
            if (cx == 0)
                return;
        }
        skip_plane = (uint16_t)((h - (unsigned)dx) * w4);
        skip_first = (uint16_t)((unsigned)(bx - (int16_t)(y + pw(e, 0x0C))) * w4);
        h = (unsigned)dx;
    }
    m = pos((unsigned)ax, (unsigned)bx, &di);
    if (keep_off | keep_seg)
        keep_covered(2 * pw(e, 0x0E), h, seg, di, keep_off, keep_seg);
    add = (unsigned)base & 0xFF;
    transparent = (pb(e, 8) + add) & 0xFF;
    si += skip_first;
    for (plane = 0; plane < 4; plane++) {
        planes(m);
        d = di;
        for (row = 0; row < h; row++) {
            for (i = 0; i < w4; i++) {
                unsigned b = pb(e, si++), hi, lo, sum;

                /* ADD AX,BP: both nibbles and the base in one word */
                sum = (((b & 0x0F) << 8) | (b >> 4)) + (add << 8 | add);
                hi = sum & 0xFF;
                lo = (sum >> 8) & 0xFF;
                if (hi != transparent)
                    vwb(seg, d, hi);
                if (lo != transparent)
                    vwb(seg, (uint16_t)(d + 1), lo);
                d += 2;
            }
            d = (uint16_t)(d + GW(row_bytes) - w);
        }
        di = (uint16_t)(di + next_plane(&m));
        si += skip_plane;
    }
    planes(0x0F);
}

/* T23FD:000A: kind 'U', a byte a pixel */
static void draw_entry_u(int x, int y, fptr e, unsigned keep_off, unsigned keep_seg, int base)
{
    uint16_t seg = GW(page_drawn), di, d, full[4], seen[4], start[4];
    unsigned w = pw(e, 0x0E), h = pw(e, 0x10), m, plane, row, i, si, transparent = pb(e, 8);
    int ax = (int16_t)(x + pw(e, 0x0A)), bx = (int16_t)(y + pw(e, 0x0C)), clipped = 0;

    plane_widths(w, full);
    memcpy(seen, full, sizeof seen);
    /* where each plane's pixels begin */
    start[0] = 0x12;
    start[1] = (uint16_t)(start[0] + h * full[0]);
    start[2] = (uint16_t)(start[1] + h * full[1]);
    start[3] = (uint16_t)(start[2] + h * full[2]);
    if (GW(clip_on)) {
        /* T23FD:0111 */
        int cx = (int)w, dx = (int)h, t;
        int x1 = (int16_t)GW(clip_x1), y1 = (int16_t)GW(clip_y1);
        int x2 = (int16_t)GW(clip_x2), y2 = (int16_t)GW(clip_y2);
        uint16_t cut[4];

        if (ax < x1) {
            clipped = 1;
            t = x1 - ax;
            cx -= t;
            if (cx <= 0)
                return;
            plane_widths((unsigned)t & 0xFC, cut);
            for (i = 0; i < 4; i++)
                start[i] = (uint16_t)(start[i] + cut[i]);
            ax = x1;
        } else if (ax > x2)
            return;
        if (bx < y1) {
            clipped = 1;
            t = y1 - bx;
            dx -= t;
            if (dx <= 0)
                return;
            for (i = 0; i < 4; i++)
                start[i] = (uint16_t)(start[i] + (unsigned)t * full[i]);
            bx = y1;
        } else if (bx > y2)
            return;
        if (dx + bx > y2) {
            clipped = 1;
            dx -= dx + bx - y2;
            if (dx == 0)
                return;
        }
        h = (unsigned)dx;
        if (cx + ax > x2) {
            clipped = 1;
            cx -= cx + ax - x2;
            if (cx == 0)
                return;
        }
        if (clipped) {
            w = (unsigned)cx;
            plane_widths(w, seen);
        }
    }
    m = pos((unsigned)ax, (unsigned)bx, &di);
    if (keep_off | keep_seg)
        keep_covered(w, h, seg, di, keep_off, keep_seg);
    si = start[0];
    for (plane = 0; plane < 4; plane++) {
        if (clipped)
            si = start[plane];
        planes(m);
        d = di;
        if (seen[plane])
            for (row = 0; row < h; row++) {
                for (i = 0; i < seen[plane]; i++) {
                    unsigned b = pb(e, si++);

                    if (b != transparent)
                        vwb(seg, (uint16_t)(d + i), b + (unsigned)base);
                }
                d = (uint16_t)(d + GW(row_bytes));
                if (clipped)
                    si += full[plane] - seen[plane];
            }
        di = (uint16_t)(di + next_plane(&m));
    }
    planes(0x0F);
}

void draw_entry(int x, int y, fptr entry, unsigned keep_off, unsigned keep_seg, int base)
{
    if (pb(entry, 9) == 'P')
        draw_entry_p(x, y, entry, keep_off, keep_seg, base);
    else
        draw_entry_u(x, y, entry, keep_off, keep_seg, base);
}

/* a part of the ground (an entry of 24 by 24, kind 'U') into the store
 * behind the pages, 6 bytes a row and plane; where it is */
fptr store_part(fptr entry)
{
    fptr at = GFP(parts_next);
    uint16_t seg = FSEG(at), di;
    unsigned plane, i, si = 0x12;

    SWO(parts_next, 0, FOFF(at) + 0x90);
    for (plane = 0; plane < 4; plane++) {
        planes(1u << plane);
        di = FOFF(at);
        for (i = 0; i < 0x90; i++)
            vwb(seg, di++, pb(entry, si++));
    }
    planes(0x0F);
    return at;
}

/* the hexagon of a stored part at x, y (x a multiple of 4): whole bytes
 * of four pixels copied with the latches, rows 0 to 23, and at the edges
 * single planes of a byte */
void draw_hexagon(int x, int y, fptr part)
{
    static const struct { uint8_t first, last, from, count; } rows[] = {
        { 0, 4, 2, 2 }, { 5, 10, 1, 4 }, { 11, 12, 0, 6 }, { 13, 18, 1, 4 }, { 19, 23, 2, 2 },
    };
    static const struct { uint8_t planes, n; uint16_t at[12]; } edges[] = {
        { 0x01, 6, { 0x0A, 0x54, 0x10, 0xA4, 0x82, 0x694, 0x88, 0x6E4, 0x2F, 0x235, 0x65, 0x505 } },
        { 0x03, 4, { 0x16, 0xF4, 0x7C, 0x644, 0x35, 0x285, 0x5F, 0x4B5 } },
        { 0x07, 6, { 0x1C, 0x144, 0x76, 0x5F4, 0x3B, 0x2D5, 0x59, 0x465, 0x41, 0x325, 0x53, 0x415 } },
        { 0x08, 6, { 0x07, 0x51, 0x0D, 0xA1, 0x7F, 0x691, 0x85, 0x6E1, 0x2A, 0x230, 0x60, 0x500 } },
        { 0x0C, 4, { 0x13, 0xF1, 0x79, 0x641, 0x30, 0x280, 0x5A, 0x4B0 } },
        { 0x0E, 6, { 0x19, 0x141, 0x73, 0x5F1, 0x36, 0x2D0, 0x54, 0x460, 0x3C, 0x320, 0x4E, 0x410 } },
    };
    uint16_t seg = GW(page_drawn), from = FSEG(part), si = FOFF(part);
    uint16_t di = (uint16_t)(((uint16_t)x >> 2) + rw((uint16_t)(A_row_offsets + 2 * y)));
    unsigned i, row, b;

    planes(0x0F);
    latches(1);
    for (i = 0; i < sizeof rows / sizeof rows[0]; i++)
        for (row = rows[i].first; row <= rows[i].last; row++)
            for (b = rows[i].from; b < (unsigned)rows[i].from + rows[i].count; b++) {
                vrb(from, (uint16_t)(si + 6 * row + b));
                vwb(seg, (uint16_t)(di + 0x50 * row + b), 0);
            }
    for (i = 0; i < sizeof edges / sizeof edges[0]; i++) {
        planes(edges[i].planes);
        for (b = 0; b < edges[i].n; b++) {
            vrb(from, (uint16_t)(si + edges[i].at[2 * b]));
            vwb(seg, (uint16_t)(di + edges[i].at[2 * b + 1]), 0);
        }
    }
    latches(0);
    planes(0x0F);
}

/* a unit's entry (24 by 24, two pixels a byte, a row of a plane three
 * bytes) at x, y (x a multiple of 4) with `base` added */
void draw_unit24(int x, int y, int base, fptr entry)
{
    uint16_t seg = GW(page_drawn), d;
    uint16_t di = (uint16_t)(((uint16_t)x >> 2) + rw((uint16_t)(A_row_offsets + 2 * y)));
    unsigned t = pb(entry, 8), both = (t << 4 | t) & 0xFF, add = (unsigned)base & 0xFF;
    unsigned transparent = (t + add) & 0xFF, si = 0x12, plane, row, i;

    for (plane = 0; plane < 4; plane++) {
        planes(1u << plane);
        d = di;
        for (row = 0; row < 0x18; row++) {
            for (i = 0; i < 3; i++) {
                unsigned b = pb(entry, si++), sum, hi, lo;

                if (b == both)
                    continue;
                sum = (((b & 0x0F) << 8) | (b >> 4)) + (add << 8 | add);
                hi = sum & 0xFF;
                lo = (sum >> 8) & 0xFF;
                if (hi != transparent)
                    vwb(seg, (uint16_t)(d + 2 * i), hi);
                if (lo != transparent)
                    vwb(seg, (uint16_t)(d + 2 * i + 1), lo);
            }
            d += 0x50;
        }
    }
    planes(0x0F);
}

/* ---- text in the small font ---- */

/* the four bits of a nibble the other way round: the leftmost pixel is
 * plane 0 */
static unsigned rev4(unsigned n)
{
    return (n & 1) << 3 | (n & 2) << 1 | (n & 4) >> 1 | (n & 8) >> 3;
}

/* the text (ended by 0; 7Ch or 0Dh a new line, 6 rows down) in the
 * drawing record's colour: a character is 6 words of the font, a row
 * each, 6 pixels from bit 7 of the first byte */
void draw_chars(int x, int y, fptr text)
{
    uint16_t seg = GW(page_drawn), di;
    fptr font = GFP(font_ptr);
    unsigned colour = GB(draw_colour), c, row, i, x0 = (unsigned)x;

    for (i = 0; (c = pb(text, i)) != 0; i++) {
        if (c == 0x7C || c == 0x0D) {
            y += 6;
            x = (int)x0;
            continue;
        }
        pos((unsigned)x, (unsigned)y, &di);
        vga_outw(0x3CE, 0xFF08);
        vga_outb(0x3C4, 2);
        for (row = 0; row < 6; row++) {
            unsigned bits = pw(font, c * 12 + 2 * row), n = 4 - ((unsigned)x & 3);

            bits = (bits << n | bits >> (16 - n)) & 0xFFFF;
            vga_outb(0x3C5, (uint8_t)rev4(bits >> 8 & 0x0F));
            vwb(seg, di, colour);
            vga_outb(0x3C5, (uint8_t)rev4(bits >> 4 & 0x0F));
            vwb(seg, (uint16_t)(di + 1), colour);
            vga_outb(0x3C5, (uint8_t)rev4(bits & 0x0F));
            vwb(seg, (uint16_t)(di + 2), colour);
            di = (uint16_t)(di + GW(row_bytes));
        }
        vga_outb(0x3C4, 2);
        x += 6;
    }
}

/* ---- pictures ---- */

/* The picture file `name` (IFF "PBM " or "ILBM", its body packed by
 * rows) loaded to `dest` (0: a block of its own, given back) and drawn on
 * the page drawn to from its upper left corner; its palette goes to
 * picture_palette.  Always returns -1, as the original; the size through
 * width and height. */
static fptr find_chunk(uint16_t seg, uint16_t *di, unsigned *n, unsigned first, unsigned second)
{
    /* word by word, 5000 words on (REPNE SCASW; a find in the last word
     * does not count) */
    while (*n) {
        unsigned wd = frw(seg, *di);

        *di += 2;
        --*n;
        if (wd != first)
            continue;
        if (!*n)
            break;
        if (frw(seg, *di) == second) {
            *di -= 2;
            return MKFP(seg, *di);
        }
    }
    return 0;
}

/* T2550:02E5: a picture wider than 360 and higher than 240 pixels (none of
 * the game's files is; read in BATTLE.ASM, not run).  Only compression 1:
 * the runs (as below) go byte after byte to page_drawn:0 with the map mask
 * as it is, h rows of (w + 7) / 8 bytes, a run over a row's end counted
 * into the next; both pointers step on by 1000h paragraphs where their
 * offset wraps.  What would land above A000:FFFF is not on the card in this
 * mode and is left out here. */
static void draw_large(fptr body, unsigned w, unsigned h, unsigned packing)
{
    uint16_t ss = FSEG(body), so = (uint16_t)(FOFF(body) + 8);
    uint16_t ds = GW(page_drawn), d = 0;
    int16_t x = 0, y = 0, row = (int16_t)((w + 7) >> 3);

    if (packing != 1)
        return;
    do {
        int8_t c = (int8_t)frb(ss, so);
        int16_t run = c >= 0 ? c + 1 : 1 - c;
        uint8_t v = 0;

        if (!++so)
            ss += 0x1000;
        if (c < 0) {
            v = frb(ss, so);
            if (!++so)
                ss += 0x1000;
        }
        for (int16_t k = run; k; k--) {
            if (c >= 0) {
                v = frb(ss, so);
                if (!++so)
                    ss += 0x1000;
            }
            if (ds < 0xB000)
                vwb(ds, d, v);
            if (!++d)
                ds += 0x1000;
        }
        x = (int16_t)(x + run);
        if (x >= row) {
            x = 0;
            y++;
        }
    } while (y < (int16_t)h);
}

int load_picture(fptr dest, fptr name, fptr work, int *width, int *height)
{
    fptr at, bmhd, cmap, body;
    uint16_t seg, di, page = GW(page_drawn), d;
    unsigned w = 0, h = 0, packing, depth, rowb = GW(row_bytes), pbm = 1, si, y, left, n, m, i;

    work = hnorm(work);
    name = hnorm(name);
    at = load_file(dest, name, work);
    if (FSEG(at)) {
        seg = FSEG(at);
        di = FOFF(at);
        n = 0x1388;
        if (!find_chunk(seg, &di, &n, 0x4250, 0x204D)) {    /* "PBM " */
            di = FOFF(at);
            n = 0x1388;
            if (!find_chunk(seg, &di, &n, 0x4C49, 0x4D42))  /* "ILBM" */
                goto out;
            pbm = 0;
        } else
            di = FOFF(at);
        n = 0x1388;
        bmhd = find_chunk(seg, &di, &n, 0x4D42, 0x4448);
        if (!bmhd)
            goto out;
        n = 0x1388;
        cmap = find_chunk(seg, &di, &n, 0x4D43, 0x5041);
        if (!cmap)
            goto out;
        di = 0;
        n = 0x1388;
        body = find_chunk(seg, &di, &n, 0x4F42, 0x5944);
        if (!body)
            goto out;
        w = pb(bmhd, 8) << 8 | pb(bmhd, 9);
        h = pb(bmhd, 0x0A) << 8 | pb(bmhd, 0x0B);
        packing = pb(bmhd, 0x12);
        depth = pb(bmhd, 0x10);
        n = pb(cmap, 6) << 8 | pb(cmap, 7);
        for (i = 0; i < n; i++)
            SBO(picture_palette, i, pb(cmap, 8 + i));
        if ((int16_t)w > 0x168 && (int16_t)h > 0xF0) {
            draw_large(body, w, h, packing);
            goto out;
        }
        si = 8;
        if (packing != 1)
            goto out;
        vga_outb(0x3C4, 2);
        d = (uint16_t)(0 - rowb);
        if (pbm) {
            /* a byte a pixel, rows of runs: n + 1 bytes as they are, or
             * one byte 1 - n times */
            for (y = 0; y < h; y++) {
                uint16_t p;

                d = (uint16_t)(d + rowb);
                p = d;
                m = 0x11;
                left = w;
                while (left) {
                    unsigned c = pb(body, si++), v = 0, run;

                    if (c & 0x80) {
                        run = (0x101 - c) & 0xFF;
                        v = pb(body, si++);
                    } else
                        run = c + 1;
                    for (; run && left; run--, left--) {
                        if (!(c & 0x80))
                            v = pb(body, si++);
                        vga_outb(0x3C5, (uint8_t)m);
                        vwb(page, p, v);
                        p = (uint16_t)(p + next_plane(&m));
                    }
                }
            }
        } else {
            /* bit planes: a row of each of the planes in turn, the same
             * runs; a bit set ORs the plane's bit into the pixel */
            for (y = 0; y < h; y++) {
                unsigned plane, bit = 1;

                d = (uint16_t)(d + rowb);
                for (plane = 0; plane < depth; plane++, bit = (bit << 1 | bit >> 7) & 0xFF) {
                    uint16_t p = d;

                    m = 0x11;
                    left = w;
                    while (left) {
                        unsigned c = pb(body, si++), v = 0, run, k;

                        if (c & 0x80) {
                            run = (0x101 - c) & 0xFF;
                            v = pb(body, si++);
                        } else
                            run = c + 1;
                        for (; run && left; run--) {
                            if (!(c & 0x80))
                                v = pb(body, si++);
                            for (k = 0; k < 8 && left; k++, left--) {
                                vga_outb(0x3C5, (uint8_t)m);
                                if (v << k & 0x80)
                                    vwb(page, p, vrb(page, p) | bit);
                                p = (uint16_t)(p + next_plane(&m));
                            }
                        }
                    }
                }
            }
        }
    }
out:
    if (dest == 0)
        t2621_0008();
    if (width)
        *width = (int)w;
    if (height)
        *height = (int)h;
    return -1;
}

/* ---- the Blue Byte logo ---- */

/* T2433:0006: the picture `picture` (BB.IFF) shown at once, then the
 * sprites of `sprites` (BB.DAT) drawn over it one after the other, a
 * picture each, by the table logo_steps: the bars grow and the star
 * turns.  With buffer 0 a block is taken for the sprites and given back. */
void t2433_0006(fptr buffer, fptr picture, fptr sprites, fptr work)
{
    fptr data, entry;
    unsigned i;
    int own = buffer == 0;

    SW(clip_on, 0xFFFF);
    if (own) {
        work = t2619_0004(0x1000);
        data = load_file(0, sprites, work);
        load_picture(0, picture, work, NULL, NULL);
    } else {
        long size;

        data = buffer;
        size = t2624_0006(buffer, sprites, work, NULL);
        buffer = MKFP(FSEG(buffer) + (uint16_t)((uint32_t)(FOFF(buffer) + (size & 0xFFFF)) >> 16)
                          + (uint16_t)(size >> 16),
                      FOFF(buffer) + (uint16_t)size);
        load_picture(buffer, picture, work, NULL, NULL);
    }
    flip_page();
    set_palette(0xFF, FP(picture_palette));
    copy_page();
    for (i = 0; i < 0x43; i++) {
        entry = hnorm(MKFP(FSEG(data), FOFF(data) + GWO(logo_steps, 6 * i + 4)));
        draw_entry((int16_t)(GWO(logo_steps, 6 * i) + 3), (int16_t)(GWO(logo_steps, 6 * i + 2) - 1),
                   entry, 1, 0, 0);
        flip_page();
        restore_sprites();
    }
    flip_page();
    restore_sprites();
    SW(clip_on, 0xFFFF);
    if (own) {
        t2621_0008();
        t2621_0008();
    }
}

/* T24C5:002E: a film's frame ("VDIF", then for each of the four planes
 * runs of bytes: FFh and a word is where the next goes, 3E80h or more
 * ends the plane; a byte with bit 80h is a count less 2 and the byte to
 * fill with, another the count of bytes that follow) drawn at x, y of
 * the page drawn to; the frame after it, or FFFF:FFFF for no frame */
fptr t24c5_002e(int x, int y, fptr frame)
{
    uint16_t seg = GW(page_drawn), fs = FSEG(frame), si = FOFF(frame), base, di;
    unsigned n, b, v, at;
    int plane;

    base = (uint16_t)(y * 0x50 + ((uint16_t)(x & 0xFFFC) >> 2));
    di = base;
    if (frw(fs, si) != 0x4456 || frw(fs, (uint16_t)(si + 2)) != 0x4649)
        return 0xFFFFFFFFu;
    si += 4;
    for (plane = 0; plane < 4; plane++) {
        planes(1u << plane);
        for (;;) {
            b = frb(fs, si++);
            if (b == 0xFF) {
                at = frw(fs, si);
                si += 2;
                if (at >= 0x3E80)
                    break;
                di = (uint16_t)(at + base);
                b = frb(fs, si++);
            }
            if (b & 0x80) {
                n = (b & 0x7F) + 2;
                v = frb(fs, si++);
                while (n--)
                    vwb(seg, di++, v);
            } else
                for (n = b; n; n--)
                    vwb(seg, di++, frb(fs, si++));
        }
    }
    return MKFP(fs + (si >> 4), si & 0x0F);
}

/* T2728:000E: a box with a text (lines parted by '|') in the middle of
 * the screen, what it covers kept in the page's list: the two colours of
 * the box's edges, the text's, the fill's */
void t2728_000e(fptr text, int keep_off, int keep_seg, int light, int dark, int colour, int fill)
{
    const char *s = fstr(text);
    unsigned longest = 0, lines = 0, n, w, h;
    int x1, y1;
    uint16_t di;

    /* T2723:000A: the longest line and its end, and the lines */
    do {
        for (n = 1; *s && *s != '|'; s++)
            n++;
        if (n > longest)
            longest = n;
        lines++;
    } while (*s++);
    w = 6 * longest;
    h = 6 * (lines + 1);
    y1 = (int)((GW(screen_height) - h) >> 1);
    x1 = (int)((GW(screen_width) - w) >> 1);
    pos((unsigned)x1, (unsigned)y1, &di);
    keep_covered(w / GW(text_scale) + 1, h + 1, GW(page_drawn), di, (unsigned)keep_off, (unsigned)keep_seg);
    t24a8_0008(x1, y1, x1 + (int)w, y1 + (int)h, light, dark, fill);
    SW(draw_colour, colour);
    draw_chars(x1 + 3, y1 + 3, text);
}

/* T262A:000E: a message in a box until a key is pressed */
void t262a_000e(fptr text)
{
    if (GW(text_scale) == 1) {
        set_dac_entry(0xFA, 0x36, 0x36, 0x36);
        set_dac_entry(0xFB, 0x0C, 0x0C, 0x0C);
        set_dac_entry(0xFC, 0x0C, 0x0C, 0x0C);
        set_dac_entry(0xFD, 0x20, 0x20, 0x20);
        t2728_000e(text, 1, 0, 0xFA, 0xFB, 0xFC, 0xFD);
    } else
        t2728_000e(text, 1, 0, 0x0F, 0, 4, 7);
    flip_page();
    while (!GW(key_there)) {
        bi_at("box_key");
        clock_idle();
    }
    SW(key_there, 0);
    flip_page();
    restore_sprites();
}
