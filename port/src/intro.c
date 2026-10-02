/* intro.c - INTEGA/INTRO.EXE, the intro of 256 colours that GOG's start
 * runs before the game (BI.EXE, with its EGA branch: the folder is named
 * the other way round, see docs/HANDOFF.md): T0529 (the screen) and T03FA
 * (the show) as C over the program's memory image, like the game's.
 *
 * The show is a list of pictures (INTRO0..12.VGA, "VDIF" frames of the
 * four planes as the films of BATTLE.EXE, with IPA0..12.VGA the palettes)
 * and texts in two fonts (CSMA, CBIG, found by the tables STAB, BTAB),
 * faded in and out, with a song and a few effects.  The screen is the
 * VGA's 320x200 with the planes unchained, two pages (A400 drawn, A000
 * shown at the start).  The timer, the sound and the files are the game's
 * modules (timer.c, sound.c, files.c).  What the library of the program
 * does around it (the Ctrl-Break handler, the texts it prints before the
 * mode is set) is left out.  Esc ends the show at once.
 */
#include <setjmp.h>
#include <string.h>
#include "bi.h"
#include "vga.h"

#define F0728 ((uint16_t)(INTEGA_F0728 + LOAD_SEG))
/* a far pointer into T03FA's far data */
#define FPD(off) MKFP(F0728, (off))

static jmp_buf escape;

/* ---- T0529: the screen ---- */

static uint16_t vofs(uint16_t seg, uint16_t off)
{
    return (uint16_t)(((seg - 0xA000u) << 4) + off);
}
static void vwb(uint16_t seg, uint16_t off, unsigned v) { vga_write(vofs(seg, off), (uint8_t)v); }
static uint8_t vrb(uint16_t seg, uint16_t off) { return vga_read(vofs(seg, off)); }
static void planes(unsigned m) { vga_outw(0x3C4, (uint16_t)(m << 8 | 2)); }

/* T0529:000E: mode 13h with the planes unchained, byte mode, and the
 * 64 KB from the page drawn to cleared */
static void set_mode(void)
{
    uint32_t i, o;

    SB(intro_old_mode, 3);
    vga_set_mode(0x13);
    vga_outb(0x3C4, 4);
    vga_outb(0x3C5, vga_inb(0x3C5) & 0xF7);
    for (i = 0; i < 0x10000; i++) {
        o = ((uint32_t)(GW(intro_page_drawn) - 0xA000u) << 4) + i;
        if (o < 0x10000)
            vga_write((uint16_t)o, 0);
    }
    vga_outb(0x3D4, 0x11);
    vga_outb(0x3D5, vga_inb(0x3D5) & 0x7F);
    vga_outw(0x3D4, 0xC317);
    vga_outw(0x3D4, 0x0014);
}

/* T03FA:0026, T0529:00A0: the mode as it was */
static void restore_mode(void)
{
    vga_set_mode(GB(intro_old_mode));
}

/* T0529:0152: all four planes of the page drawn to cleared */
static void clear_pages(void)
{
    unsigned m, i;

    for (m = 1; m < 0x10; m <<= 1) {
        planes(m);
        for (i = 0; i < 0x3E80; i++)
            vwb(GW(intro_page_drawn), (uint16_t)i, 0);
    }
}

/* T0529:017F: the page drawn to is shown (the start address from the next
 * retrace) and the other drawn to */
static void flip(void)
{
    uint16_t shown = GW(intro_page_shown), drawn = GW(intro_page_drawn), start;

    SW(intro_page_drawn, shown);
    SW(intro_page_shown, drawn);
    start = (uint16_t)(drawn << 4);
    vga_outw(0x3D4, (uint16_t)((start & 0xFF00) | 0x0C));
    vga_outw(0x3D4, (uint16_t)((start & 0xFF) << 8 | 0x0D));
    clock_retrace();
}

/* T0529:01B0: the page shown copied to the page drawn to through the
 * latches */
static void copy_shown(void)
{
    uint16_t from = GW(intro_page_shown), to = GW(intro_page_drawn);
    unsigned old, i;

    vga_outb(0x3CE, 5);
    old = vga_inb(0x3CF);
    vga_outw(0x3CE, (uint16_t)((((old & 0xF0) | 1) << 8) | 5));
    planes(0x0F);
    for (i = 0; i < 0x3E80; i++) {
        vrb(from, (uint16_t)i);
        vwb(to, (uint16_t)i, 0);
    }
    vga_outw(0x3CE, (uint16_t)(old << 8 | 5));
}

/* T0529:01F9: the colours first to last of `pal` (three bytes each) at
 * `level` of 64, scaled in the program's buffer and written to the DAC at
 * the next retrace */
static void palette(fptr pal, unsigned first, unsigned last, unsigned level)
{
    unsigned c, k, di = 0, n = last + 1 - first;

    for (c = first; c <= last; c++)
        for (k = 0; k < 3; k++)
            SBO(intro_dac, di++, (uint16_t)(pb(pal, 3 * c + k) * level) >> 6);
    vga_outb(0x3C8, (uint8_t)first);
    clock_retrace();
    for (di = 0; di < 3 * n; di++)
        vga_outb(0x3C9, GBO(intro_dac, di));
}

/* T0529:029D: a glyph (a record: +0Ah, +0Ch the offset of its corner, +0Eh
 * the width in each plane, +18h the height, +1Ah the bytes plane by
 * plane) at x, y of the page drawn to */
static void draw_glyph(int x, int y, fptr g)
{
    uint16_t seg = GW(intro_page_drawn);
    unsigned h, si = 0x1A, m, p, r, k, w;
    uint16_t di, d;

    x = (int16_t)(x + pw(g, 0x0A));
    y = (int16_t)(y + pw(g, 0x0C));
    di = (uint16_t)(y * 0x50 + ((uint16_t)x >> 2));
    m = 1u << (x & 3);
    h = pw(g, 0x18);
    for (p = 0; p < 4; p++) {
        planes(m);
        w = pw(g, 0x0E + 2 * p);
        d = di;
        for (r = 0; r < h; r++) {
            for (k = 0; k < w; k++)
                vwb(seg, (uint16_t)(d + k), pb(g, si++));
            d = (uint16_t)(d + 0x50);
        }
        m <<= 1;
        if (m & 0x10) {
            m = 1;
            di++;
        }
    }
}

/* T0529:00AF: a frame ("VDIF", then for each plane runs: FFh and a word
 * say where the next bytes go (3E80h or more ends the plane), a byte with
 * bit 80h a count less 2 and the byte to fill with, another the count of
 * bytes that follow) drawn at x, y of the page drawn to; the frame after
 * it, FFFF:FFFF when there is none */
static fptr decode(int x, int y, fptr frame)
{
    uint16_t seg = GW(intro_page_drawn), fs = FSEG(frame), si = FOFF(frame), base, di;
    unsigned n, b, v, at;
    int plane;

    x &= 0xFFFC;
    base = (uint16_t)(y * 0x50 + ((uint16_t)x >> 2));
    planes(1);
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

/* ---- T03FA: the show ---- */

/* T03FA:00BB: Esc ends the show (the original's kbhit and getch; the
 * timer's callback reads the BIOS's key too, it may have been first) */
static void check_key(void)
{
    if (bios_key_waits() && (bios_key() & 0xFF) == 0x1B)
        longjmp(escape, 1);
    if (GW(key_there) && GB(key_char) == 0x1B)
        longjmp(escape, 1);
}

/* T03FA:0288: ticks of the timer, the keys looked at meanwhile */
static void wait_ticks(int n)
{
    bi_at("intro_wait");
    SW(tick_count, 0);
    while ((int16_t)GW(tick_count) < n) {
        check_key();
        clock_idle();
    }
}

/* T03FA:00EB: the glyph of a character (above FFh: the big font's) */
static fptr glyph(int c)
{
    uint16_t off = frw(F0728, (uint16_t)(A_intro_small_table + 2 * c));

    if (c > 0xFF)
        return MKFP(frw(F0728, A_intro_big_font + 2), (uint16_t)(frw(F0728, A_intro_big_font) + off));
    return MKFP(frw(F0728, A_intro_small_font + 2), (uint16_t)(frw(F0728, A_intro_small_font) + off));
}

/* T03FA:0124: the width of a character */
static int char_width(int c)
{
    return c == 0x20 ? 5 : pb(glyph(c), 0x16);
}

/* T03FA:015B: the width of a line (up to its end or a 0Ah) */
static int str_width(fptr s)
{
    int w = 0;

    for (; pb(s, 0) != 0 && pb(s, 0) != 0x0A; s++)
        w += char_width(pb(s, 0)) + 1;
    return w;
}

/* T03FA:019E: picture n and its palette loaded and put on the stack of the
 * loaded ones; a file that is missing ends the show */
static void load_pic(int n)
{
    fptr pic = load_file(0, FPD((uint16_t)(A_intro_pic_names + n * 0x14)), GFP(intro_work));
    fptr pal = load_file(0, FPD((uint16_t)(A_intro_pal_names + n * 0x14)), GFP(intro_work));
    unsigned i;

    if (!pic || !pal)
        longjmp(escape, 2);
    i = GW(intro_pics);
    SWO(intro_pal_list, 4 * i, FOFF(pal));
    SWO(intro_pal_list, 4 * i + 2, FSEG(pal));
    SWO(intro_pic_list, 4 * i, FOFF(pic));
    SWO(intro_pic_list, 4 * i + 2, FSEG(pic));
    SW(intro_pics, i + 1);
}

/* T03FA:0267: the last two blocks (a palette and a picture) back */
static void pop_pic(void)
{
    if (GW(intro_pics)) {
        t2621_0008();
        t2621_0008();
        SW(intro_pics, GW(intro_pics) - 1);
    }
}

/* T03FA:0347: picture n is the one drawn from and its palette the one in use */
static void select_pic(int n)
{
    SW(intro_frame, GWO(intro_pic_list, 4 * n));
    SWO(intro_frame, 2, GWO(intro_pic_list, 4 * n + 2));
    SW(intro_pal, GWO(intro_pal_list, 4 * n));
    SWO(intro_pal, 2, GWO(intro_pal_list, 4 * n + 2));
}

/* T03FA:031E: the frame to come drawn at 0, 0 */
static void draw_frame(void)
{
    fptr next = decode(0, 0, GFP(intro_frame));

    SFP(intro_frame, next);
}

/* T03FA:02BF: up to `count` frames from the one to come, each shown and
 * copied to the other page, then a wait of `ticks` */
static void play_frames(int x, int y, int count, int ticks)
{
    int i;

    for (i = 0; GFP(intro_frame) != 0xFFFFFFFFu && i < count; i++) {
        fptr next = decode(x, y, GFP(intro_frame));

        SFP(intro_frame, next);
        flip();
        copy_shown();
        wait_ticks(ticks);
    }
}

/* the colours 0 to 1Fh at a level of 64 (the texts' ground), the others */
static void fade_in_low(void)
{
    int i;

    palette(GFP(intro_pal), 0, 0x1F, 0);
    flip();
    for (i = 0; i <= 0x40; i++) {
        palette(GFP(intro_pal), 0, 0x1F, (unsigned)i);
        wait_ticks(1);
    }
}

static void fade_out_low(void)
{
    int i;

    for (i = 0x40; i >= 0; i--) {
        palette(GFP(intro_pal), 0, 0x1F, (unsigned)i);
        wait_ticks(1);
    }
    clear_pages();
    select_pic(0);
    draw_frame();
    flip();
    clear_pages();
    select_pic(0);
    draw_frame();
}

static void fade_out_all(void)
{
    int i;

    for (i = 0x40; i >= 0; i -= 2) {
        palette(GFP(intro_pal), 0, 0x80, (unsigned)i);
        palette(GFP(intro_pal), 0x81, 0xFF, (unsigned)i);
    }
    clear_pages();
}

static void fade_in_all(void)
{
    int i;

    flip();
    for (i = 0; i <= 0x40; i += 2) {
        palette(GFP(intro_pal), 0, 0x80, (unsigned)i);
        palette(GFP(intro_pal), 0x81, 0xFF, (unsigned)i);
    }
    copy_shown();
}

/* T03FA:08A8: picture 0's frame and its colours faded in */
static void show_first(void)
{
    clear_pages();
    select_pic(0);
    draw_frame();
    fade_in_all();
}

/* T03FA:037F: a character at the text's place, which moves on (0Ah: the
 * next line, 20h: a gap of 6; above FFh the big font, drawn 9 higher) */
static void draw_char(int c)
{
    if (c == 0x0A) {
        SW(intro_text_y, GW(intro_text_y) + 0x11);
        SW(intro_text_x, 0);
    } else if (c == 0x20) {
        SW(intro_text_x, GW(intro_text_x) + 6);
    } else {
        draw_glyph((int16_t)GW(intro_text_x), (int16_t)GW(intro_text_y) - (c > 0xFF ? 9 : 0), glyph(c));
        SW(intro_text_x, GW(intro_text_x) + char_width(c) + 1);
    }
}

/* T03FA:040F: a line; @ before a character takes the big font's */
static void text_print(fptr s)
{
    for (; pb(s, 0) != 0; s++) {
        if (pb(s, 0) == 0x40) {
            s++;
            draw_char(pb(s, 0) + 0x100);
        } else
            draw_char(pb(s, 0));
    }
}

/* T03FA:0456: a line in the middle of the screen */
static void text_center(fptr s)
{
    SW(intro_text_x, (int16_t)(0x140 - str_width(s)) / 2);
    text_print(s);
}

/* T03FA:0490: how much a line of width w is short of (the gaps count 6,
 * the others their width and 1) */
static void measure(fptr s, int w)
{
    int spaces = 0, width = 0;

    for (; pb(s, 0) != 0; s++) {
        if (pb(s, 0) == 0x20)
            spaces++;
        else if (pb(s, 0) == 0x40) {
            s++;
            width += char_width(pb(s, 0) + 0x100) + 1;
        } else
            width += char_width(pb(s, 0)) + 1;
    }
    width += spaces * 6;
    SW(intro_slack, w - width);
}

/* T03FA:0525: a mouth moving: n times one of the pictures 1 to 3 at
 * random, its frames shown */
static void mouth(int n)
{
    int i;

    for (i = 1; i <= n; i++) {
        select_pic(1 + (int)((long)bi_rand() * 3 / 0x8000));
        play_frames(0, 0, 0x64, 0);
    }
}

/* the slack of a line shared among the characters to come (T03FA:0601..) */
static void spread(int left)
{
    int q;

    if ((int16_t)GW(intro_slack) > 0) {
        q = (int16_t)GW(intro_slack) / left;
        SW(intro_text_x, GW(intro_text_x) + q);
        if (q == 0) {
            SW(intro_slack, GW(intro_slack) - 1);
            SW(intro_text_x, GW(intro_text_x) + 1);
        } else
            SW(intro_slack, GW(intro_slack) - q);
    }
}

/* T03FA:0584: a text of lines 165 wide at x 9Ch, justified; the mouth moves
 * for each vowel before a gap and at the end */
static void text_block(fptr s)
{
    int left = 0, k = 0, c;
    fptr p;

    measure(s, 0xA5);
    SW(intro_text_x, 0x9C);
    for (p = s; pb(p, 0) != 0; p++) {
        if (pb(p, 0) == 0x0A)
            SW(intro_slack, 0);
        left++;
    }
    for (p = s; pb(p, 0) != 0; p++) {
        c = pb(p, 0);
        if (c == 0x20) {
            mouth(k);
            draw_char(0x20);
            spread(left);
            left--;
            k = 0;
        } else if (c == 0x40) {
            p++;
            draw_char(pb(p, 0) + 0x100);
            spread(left);
            left--;
        } else {
            draw_char(c);
            spread(left);
            left--;
        }
        if (strchr(fstr(FP(intro_vowels)), pb(p, 0)))
            k++;
    }
    mouth(k);
    draw_char(0x0A);
}

/* an effect asked for in the second channel's record (F07D4:0006) */
static void effect(unsigned number)
{
    fptr e = FP(intro_effects);

    spw(e, 6, number);
    spw(e, 8, 0x7F);
    spw(e, 10, 1);
    effects_start();
}

/* T03FA:0037: the tables and fonts of the texts */
static void load_fonts(void)
{
    fptr work = GFP(intro_work), f;

    load_file(FPD(A_intro_small_table), FPD(0x6DA), work);
    load_file(FPD(A_intro_big_table), FPD(0x6E3), work);
    f = load_file(0, FPD(0x6EC), work);
    SFP(intro_big_font, f);
    f = load_file(0, FPD(0x6F5), work);
    SFP(intro_small_font, f);
}

/* T03FA:08C9: the show */
static void show(void)
{
    int v, i;

    set_mode();
    palette(GFP(intro_pal), 0, 0xFF, 0);
    clear_pages();
    flip();
    clear_pages();
    load_song(0, FPD(0x710), GFP(intro_work));
    load_pic(0);
    load_pic(1);
    load_pic(2);
    load_pic(3);
    play_song(0, 0);
    wait_ticks(100);
    show_first();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x28);
    text_center(FPD(0x71A));
    text_center(FPD(0x72E));
    text_center(FPD(0x74C));
    text_center(FPD(0x765));
    text_center(FPD(0x78F));
    text_center(FPD(0x7AE));
    fade_in_low();
    wait_ticks(500);
    fade_out_low();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x4B);
    text_center(FPD(0x7C8));
    text_center(FPD(0x7E5));
    fade_in_low();
    wait_ticks(350);
    fade_out_low();
    fade_out_all();
    wait_ticks(50);
    select_pic(1);
    draw_frame();
    fade_in_all();
    wait_ticks(250);
    play_frames(0, 0, 3, 3);
    wait_ticks(110);
    play_frames(0, 0, 2, 3);
    wait_ticks(110);
    play_frames(0, 0, 1, 1);
    wait_ticks(110);
    play_frames(0, 0, 5, 5);
    wait_ticks(140);
    play_frames(0, 0, 100, 3);
    wait_ticks(350);
    fade_out_all();
    wait_ticks(50);
    show_first();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x5D);
    text_center(FPD(0x7FE));
    fade_in_low();
    wait_ticks(250);
    fade_out_low();
    fade_out_all();
    select_pic(2);
    draw_frame();
    fade_in_all();
    wait_ticks(240);
    play_frames(0, 0, 100, 6);
    fade_out_all();
    wait_ticks(20);
    select_pic(3);
    draw_frame();
    fade_in_all();
    play_frames(0, 0, 10, 1);
    play_frames(0, 0, 100, 1);
    wait_ticks(100);
    fade_out_all();
    wait_ticks(30);
    show_first();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x5A);
    text_center(FPD(0x80F));
    fade_in_low();
    wait_ticks(350);
    fade_out_low();
    fade_out_all();
    pop_pic();
    load_pic(4);
    select_pic(3);
    draw_frame();
    fade_in_all();
    wait_ticks(250);
    play_frames(0, 0, 28, 2);
    play_frames(0, 0, 20, 2);
    play_frames(0, 0, 100, 2);
    wait_ticks(150);
    fade_out_all();
    wait_ticks(30);
    show_first();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x5A);
    text_center(FPD(0x82A));
    fade_in_low();
    wait_ticks(350);
    fade_out_low();
    fade_out_all();
    select_pic(2);
    draw_frame();
    fade_in_all();
    wait_ticks(400);
    fade_out_all();
    show_first();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x50);
    text_center(FPD(0x849));
    text_center(FPD(0x869));
    fade_in_low();
    wait_ticks(400);
    fade_out_low();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x50);
    text_center(FPD(0x882));
    text_center(FPD(0x8A2));
    fade_in_low();
    wait_ticks(360);
    fade_out_low();
    fade_out_all();
    pop_pic();
    pop_pic();
    pop_pic();
    load_pic(5);
    select_pic(1);
    draw_frame();
    fade_in_all();
    play_frames(0, 0, 200, 9);
    wait_ticks(50);
    fade_out_all();
    pop_pic();
    load_pic(6);
    load_pic(7);
    load_pic(8);
    load_pic(9);
    wait_ticks(50);
    select_pic(4);
    draw_frame();
    fade_in_all();
    wait_ticks(100);
    play_frames(0, 0, 100, 6);
    wait_ticks(80);
    SW(intro_text_y, 0x28);
    text_block(FPD(0x8BD));
    text_block(FPD(0x8C9));
    wait_ticks(80);
    text_block(FPD(0x8D7));
    text_block(FPD(0x8E8));
    text_block(FPD(0x8F6));
    text_block(FPD(0x905));
    text_block(FPD(0x913));
    wait_ticks(90);
    select_pic(4);
    draw_frame();
    play_frames(0, 0, 100, 6);
    wait_ticks(50);
    clear_pages();
    select_pic(1);
    draw_frame();
    wait_ticks(60);
    SW(intro_text_y, 0x21);
    text_block(FPD(0x925));
    text_block(FPD(0x933));
    text_block(FPD(0x944));
    text_block(FPD(0x952));
    text_block(FPD(0x964));
    text_block(FPD(0x96E));
    text_block(FPD(0x97C));
    text_block(FPD(0x986));
    wait_ticks(90);
    select_pic(4);
    draw_frame();
    play_frames(0, 0, 100, 6);
    wait_ticks(50);
    clear_pages();
    select_pic(1);
    draw_frame();
    wait_ticks(60);
    SW(intro_text_y, 0x32);
    text_block(FPD(0x990));
    text_block(FPD(0x9A1));
    text_block(FPD(0x9B5));
    text_block(FPD(0x9C6));
    text_block(FPD(0x9D0));
    wait_ticks(250);
    fade_out_all();
    wait_ticks(50);
    show_first();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x5A);
    text_center(FPD(0x9DC));
    text_center(FPD(0x9FF));
    fade_in_low();
    wait_ticks(300);
    fade_out_low();
    fade_out_all();
    pop_pic();
    pop_pic();
    pop_pic();
    pop_pic();
    load_pic(10);
    select_pic(1);
    draw_frame();
    fade_in_all();
    play_frames(0, 0, 300, 1);
    wait_ticks(200);
    fade_out_all();
    wait_ticks(40);
    show_first();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x40);
    text_center(FPD(0xA19));
    text_center(FPD(0xA35));
    text_center(FPD(0xA58));
    text_center(FPD(0xA7B));
    fade_in_low();
    wait_ticks(30);
    /* the music fades out: 7Fh down to 0 on all eleven channels */
    for (v = 0x7F; v >= 0; v--) {
        for (i = 0; i <= 10; i++)
            effects_volume(i, v);
        wait_ticks(5);
    }
    stop_song();
    fade_out_low();
    SW(intro_text_x, 0x0);
    SW(intro_text_y, 0x5C);
    text_center(FPD(0xA94));
    fade_in_low();
    wait_ticks(200);
    fade_out_all();
    pop_pic();
    pop_pic();
    load_pic(11);
    load_pic(12);
    load_effects(0, FPD(0xAA6), GFP(intro_work));
    select_pic(0);
    draw_frame();
    fade_in_all();
    wait_ticks(50);
    spw(FP(intro_effects), 6, 0);
    spw(FP(intro_effects), 8, 0x7F);
    spw(FP(intro_effects), 10, 1);
    for (i = 0; i <= 10; i++)
        effects_volume(i, 100);
    effects_start();
    play_frames(0, 0, 6, 6);
    wait_ticks(100);
    effect(1);
    play_frames(0, 0, 129, 6);
    wait_ticks(100);
    fade_out_all();
    wait_ticks(40);
    select_pic(1);
    draw_frame();
    fade_in_all();
    wait_ticks(20);
    play_frames(0, 0, 7, 6);
    effect(2);
    play_frames(0, 0, 8, 6);
    wait_ticks(40);
    play_frames(0, 0, 50, 3);
    wait_ticks(50);
    fade_out_all();
}

/* T03FA:122D: the main; the switch /s (the speaker's sound) is not taken:
 * the port plays the AdLib's */
void intro_main(void)
{
    fptr work;

    /* a program starts in DOS's text mode.  Without it the VGA's registers
     * are zero until set_mode, its refresh rate absurd, the clock's pictures
     * thousands for each timer tick and the first wait (wait_ticks(0x32), before
     * the mode is set) lasts for minutes: in a window a black screen */
    vga_set_mode(3);
    t2354_0011();
    work = t2619_0004(0x1000);
    SFP(intro_work, work);
    load_fonts();
    sound_init(0);
    if (!setjmp(escape)) {
        SW(intro_escaped, 0);
        wait_ticks(0x32);
        show();
    } else
        SW(intro_escaped, 1);
    stop_song();
    restore_mode();
    t261e_0008();
    t2354_005e();
}
