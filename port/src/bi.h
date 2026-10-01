/* bi.h - Battle Isle's main program (ISLE/BATTLE.EXE) in C, over the
 * program's own memory image (doskit/runtime/rmem.h): what every module
 * of the port shares.
 *
 * The program is loaded from the player's file where the runner loads it
 * (PSP 0067h, the image at 0077h); a routine of the original is a C
 * function of the hints' name (src/BATTLE.hints), which reads and writes
 * the memory by the hints' names (gen/names.h, symmap.py).  A routine
 * without a name is called after its address, t2485_0001.
 */
#ifndef BI_H
#define BI_H

#include <stddef.h>
#include <stdint.h>
#include "rmem.h"
#include "gen/names.h"

/* the segment the image is loaded at */
#define LOAD_SEG (RM_LOAD_PSP + 0x10u)
/* a segment of the hints as loaded: SEG(DATA), SEG(F27EE) */
#define SEG(s) ((uint16_t)(BATTLE_##s + LOAD_SEG))

/* the hints' names: A_name the offset, S_name the segment as loaded */
#define X(seg, name, a) A_##name = a,
enum { BI_NAMES(X) A_names_end };
#undef X
#define X(seg, name, a) S_##name = BATTLE_##seg + LOAD_SEG,
enum { BI_NAMES(X) S_names_end };
#undef X

/* a named variable: byte, word, long; o bytes further on */
#define GB(n) frb(S_##n, A_##n)
#define GW(n) frw(S_##n, A_##n)
#define GD(n) (frw(S_##n, A_##n) | (uint32_t)frw(S_##n, A_##n + 2) << 16)
#define SB(n, v) fwb(S_##n, A_##n, (uint8_t)(v))
#define SW(n, v) fww(S_##n, A_##n, (uint16_t)(v))
#define SD(n, v) (fww(S_##n, A_##n, (uint16_t)(v)), fww(S_##n, A_##n + 2, (uint16_t)((uint32_t)(v) >> 16)))
#define GBO(n, o) frb(S_##n, (uint16_t)(A_##n + (o)))
#define GWO(n, o) frw(S_##n, (uint16_t)(A_##n + (o)))
#define SBO(n, o, v) fwb(S_##n, (uint16_t)(A_##n + (o)), (uint8_t)(v))
#define SWO(n, o, v) fww(S_##n, (uint16_t)(A_##n + (o)), (uint16_t)(v))

/* a far pointer as the program keeps it: the offset in the low word */
typedef uint32_t fptr;
#define MKFP(seg, off) ((fptr)(uint16_t)(seg) << 16 | (uint16_t)(off))
#define FSEG(p) ((uint16_t)((p) >> 16))
#define FOFF(p) ((uint16_t)((p) & 0xFFFF))
#define FP(n) MKFP(S_##n, A_##n)
#define GFP(n) MKFP(frw(S_##n, A_##n + 2), frw(S_##n, A_##n))
#define SFP(n, p) (fww(S_##n, A_##n, FOFF(p)), fww(S_##n, A_##n + 2, FSEG(p)))

static inline uint8_t pb(fptr p, unsigned o) { return frb(FSEG(p), (uint16_t)(FOFF(p) + o)); }
static inline uint16_t pw(fptr p, unsigned o) { return frw(FSEG(p), (uint16_t)(FOFF(p) + o)); }
static inline uint32_t pd(fptr p, unsigned o) { return pw(p, o) | (uint32_t)pw(p, o + 2) << 16; }
static inline fptr pfp(fptr p, unsigned o) { return MKFP(pw(p, o + 2), pw(p, o)); }
static inline void spb(fptr p, unsigned o, unsigned v) { fwb(FSEG(p), (uint16_t)(FOFF(p) + o), (uint8_t)v); }
static inline void spw(fptr p, unsigned o, unsigned v) { fww(FSEG(p), (uint16_t)(FOFF(p) + o), (uint16_t)v); }
static inline void spd(fptr p, unsigned o, uint32_t v)
{
    spw(p, o, (uint16_t)v);
    spw(p, o + 2, (uint16_t)(v >> 16));
}
static inline void spfp(fptr p, unsigned o, fptr v) { spd(p, o, v); }
/* where a far pointer points in the megabyte */
static inline uint32_t flin(fptr p) { return lin(FSEG(p), FOFF(p)); }

/* The compiler's huge pointers: p + n with the offset kept below 10h
 * (CODE:3E1F, CODE:3EA7), and a far pointer made so (T2718:0008). */
static inline fptr hadd(fptr p, long n)
{
    uint32_t a = (uint32_t)(flin(p) + n) & (MEM_SIZE - 1);
    return MKFP(a >> 4, a & 15);
}
static inline fptr hnorm(fptr p) { return hadd(p, 0); }

/* a far pointer into the megabyte as a C string or bytes */
static inline char *fstr(fptr p) { return (char *)mem + flin(p); }

/* the records of the map: a unit, a unit type, a ground, a cursor */
#define UNIT(n) MKFP(S_units, A_units + 0x1A * (n))
#define TYPE(n) MKFP(S_unit_types, A_unit_types + 0x44 * (n))
#define GROUND(n) MKFP(S_ground, A_ground + 6 * (n))
#define CURSOR(n) MKFP(S_cursors, A_cursors + 0x31 * (n))

/* ---- dos.c: what the program got from DOS and the PC ---- */

/* the folder the program runs in (the game's ISLE) and the one its saved
 * files go to */
void dos_set_dirs(const char *game, const char *saves);
/* INT 21h 3Dh/3Ch: a handle, or 0 */
int dos_open(const char *name, int create);
void dos_close(int handle);
/* INT 21h 3Fh/40h at a place in the megabyte: the bytes read or written,
 * -1 for an error */
long dos_read(int handle, uint32_t at, long count);
long dos_read_to(int handle, void *to, long count);
long dos_write(int handle, uint32_t at, long count);
/* INT 21h 42h: the position, -1 for an error */
long dos_seek(int handle, long offset, int whence);
/* the BIOS's keyboard (INT 16h AH=01h, 00h): 1 when a key waits; the
 * key taken, scancode << 8 | character */
int bios_key_waits(void);
unsigned bios_key(void);
/* the keyboard's bytes go to the program's own INT 09h (int09) while it
 * is set, else to the BIOS's */
void dos_keyboard_own(int on);
/* the clock: `counts` of the PIT (1193182 a second) pass, the timer's
 * interrupt (int08) run when it is due, unless the interrupts are off;
 * a picture shown at each retrace.  clock_idle: on to the next interrupt
 * (a loop that waits for a count of ticks). */
void clock_run(long counts, int interrupts_off);
void clock_idle(void);
/* on to the next vertical retrace's start */
void clock_retrace(void);
/* a scan line passes with the interrupts off (set_palette) */
void clock_lines(int lines);
/* the clock's time in seconds */
double clock_seconds(void);
/* the PIT's period as int08 sets it (0: 10000h counts) */
void clock_set_period(unsigned counts);
/* the end, also when the window was closed */
void bi_exit(int code);
/* a message for the player and the end */
void bi_fatal(const char *text);
/* a place for comparisons with the original (BI_BREAK, main.c) */
void bi_at(const char *name);
/* a routine not translated yet was reached */
void bi_todo(const char *name);

/* ---- timer.c: T2354, the timers, the keys, the players' input ---- */
void t2354_0011(void);              /* set up: the timers, the joysticks, INT 24h */
void t2354_005e(void);              /* undo */
unsigned timer_add(fptr handler, uint32_t period);
void timer_remove(unsigned handle);
void timer_period(unsigned handle, uint32_t period);
void int08(void);
void int09(unsigned char scancode);
void t2354_0530(void);              /* the program's INT 09h on */
void t2354_056f(void);              /* and off */
/* a timer's handler that is a C function: timer.c calls `fn` for the far
 * pointer `handler` */
void timer_handler(fptr handler, void (*fn)(void));
void t265e_000e(void);

/* ---- files.c: T2619..T2728, files and memory ---- */
fptr t2619_0004(uint32_t size);     /* a block from DOS */
void t261e_0008(void);              /* all blocks back */
void t2621_0008(void);              /* the last block back */
int file_open(int mode, fptr name);
int file_close(int handle);
long file_size(fptr name);
fptr load_file(fptr dest, fptr name, fptr work);
/* load_file and file_size: the pointer and the length */
long t2624_0006(fptr dest, fptr name, fptr work, fptr *loaded);
int save_file(fptr name, fptr from, uint32_t count);
fptr make_path(int unused, int dir, fptr name, int ext);
void t164d_0482(int disk);
void fatal_error(int number);

/* ---- gfx.c: T23DC..T259F, the screen ---- */
int t2485_0001(int width, int height, int unused, int second, int unused2, int pages);
int t2593_0006(void);               /* the pages' segments and lists */
void wait_retrace(void);
void t0d36_000f(int retraces);
void flip_page(void);
void clear_page(void);
void copy_page(void);
void restore_sprites(void);
void set_palette(int level, fptr palette);
void set_dac_entry(int index, int r, int g, int b);
void fade_in(void);
void fade_out(void);
void fill_rect(void);
void draw_row(void);
void draw_column(void);
void draw_line(void);
void put_pixel(void);
void t2590_000a(void);
void t2592_0002(void);
void t24d3_0000(fptr item);
void t24a8_0008(int x1, int y1, int x2, int y2, int light, int dark, int fill);
void draw_entry(int x, int y, fptr entry, unsigned keep_off, unsigned keep_seg, int base);
void draw_chars(int x, int y, fptr text);
fptr store_part(fptr entry);
void draw_hexagon(int x, int y, fptr part);
void draw_unit24(int x, int y, int base, fptr entry);
int load_picture(fptr dest, fptr name, fptr work, int *width, int *height);
void t2433_0006(fptr buffer, fptr picture, fptr sprites, fptr work);

/* ---- sound.c: CODE:0215..1B79; audio.c: the port's sound output ---- */
int sound_init(int mode);
int load_song(fptr dest, fptr name, fptr work);
int play_song(int a, int b);
void stop_song(void);
fptr load_effects(fptr dest, fptr name, fptr work);
void effects_start(void);
void effects_volume(int channel, int volume);
/* a value into a register of the AdLib */
void adlib_out(unsigned reg, unsigned value);

/* ---- text.c, lib.c, title.c: T164D, T0CEB, T1727 ---- */
int draw_text24(int x, int y, fptr text, unsigned keep_off, unsigned keep_seg);
fptr next_line(fptr at, fptr start);
long load_lib(fptr path, fptr dest, fptr work, fptr record, int sorted);
int title(fptr work);

/* ---- menu.c: T1090 ---- */
int menu(fptr work);
void draw_menu(int menu);
int edit_text(int x, int y, int most, fptr item);
int ask_position(void);
void show_scores(fptr scores, int map, fptr codes);
void load_scores(fptr scores, fptr work, int n);
int t0d36_0c85(void);
fptr t26ea_000f(long value, fptr dest, int digits, int flags);

/* ---- map.c: T0E9B ---- */
void t0e9b_000b(void);
int load_shp(fptr path, fptr dest, fptr work);
int load_fin(fptr path, fptr dest, fptr work);
void draw_overview(int player, int x, int y, fptr pmp);
void draw_window(int first, int player);
void draw_marks(int first, int player);
void redraw_cursor_square(fptr cursor, int player);
void undraw_marks(int first, int player, int bits);
void redraw_square(int player, int off);
int find_building(int square, fptr table);
void t0e9b_15dd(int player, int off);
int t0e9b_1704(int off, fptr map);
void four_squares(int off);
void t0e9b_19ab(fptr from, fptr to);
void unit_flag(int unit, int flags, int player, int set);
void t0e9b_1e0d(int channel);
void t0e9b_1b9d(int a, int b);
void t0e9b_1cb2(void);
int square_distance(int x1, int y1, int x2, int y2);

/* ---- cursor.c: T0D36 ---- */
void t0d36_0044(fptr cur, int player);
int same_side(int a, int b);
void timer_set(int kind, int delay, int arg);
void timer_due(int kind, int arg);
void t0d36_032c(fptr cur, int back);
int t0d36_04d8(fptr in, fptr cur);
void bi_srand(unsigned seed);
int bi_rand(void);
int bi_random(int lo, int hi);
int t0d36_0642(fptr cur, fptr in, int bits, fptr map, int side);
void t0d36_0bb6(fptr cur);
void t0d36_0d82(fptr cur, int side, int bits);
void t0d36_15e9(fptr from, fptr to, uint32_t count);

/* ---- units.c: T169E ---- */
void make_unit(int number, int type, int square, int player);
int t169e_01a9(int mask);
int t169e_01f5(int mask);
void t169e_0244(int n, int sq, int owner);
void t169e_0324(int n, int sq, int owner);
void t169e_0401(int sq, int owner);
void t169e_04a0(int n, int sq, int type, int player, int unit);
void unit_remove(int unit, int side);
void cargo_follow(int unit, int side);

/* ---- reach.c: T0BA0 ---- */
void clear_marks(int mask);
void neighbours64(int col, int row);
void neighbours(int off);
int reach(int square, fptr buf, int unit, int points, int side, int flags, fptr map);
int fire_reach(fptr buf, int square, int range, int side, int unit, int targets, fptr map);
int find_path(fptr list, int unit, int from, int to, int side, fptr map);
void list_reach(fptr list, int side, int unit);

/* ---- orders.c: T0B70, T11FD ---- */
void show_message(int number, int side);
void unit_line(int unit, int side);
int give_order(fptr cur, fptr map, int side);
void order_release(fptr cur, int side);

/* ---- text.c ---- */
void draw_text(int x, int y, int n, int colour);
void draw_number(int n, int x, int y);
void draw_bar(int x, int y, int front, int back, int rows, int n, int full);
void check_vga_disk(void);

/* ---- shop.c: T1479, the screens over a window; T24D8, T2701 ---- */
void draw_packed(int x, int y, fptr entry, unsigned keep_off, unsigned keep_seg, int base, fptr work);
void draw_shop_window(int side);
void draw_box(int x, int y, int w, int h, int side, int colour);
void t1479_028e(int x, int y);
void draw_unit_numbers(int x, int y, int unit, int side);
void draw_unit_info(int side, int square, int unit, fptr map);
void draw_slots(int x, int y, int side, fptr rec);
void draw_building(int side, fptr rec);
void draw_type_list(int x, int y, int side, int first);
void t1479_0c93(int x, int y, int type, int side, int owner);
void draw_status(int side);
void list_makeable(int energy);
int cargo_size(fptr rec, int side);

/* ---- move.c: T122D, a move ---- */
void unit_release(fptr cur, fptr map, int side, int kind);
int move_aim(fptr cur, fptr map, int side);
int t122d_05b8(fptr cur, int side);
void move_step(void);
int move_arrive(int side, fptr map);
int stop_check(int off, int side, fptr map);
int direction(int from, int to);
void move_undo(int unit, fptr map, int side);
int count_slots(int side, fptr rec, int used);
void cargo_move(int side, fptr from, fptr to);

/* ---- not translated yet (todo.c) ---- */
int change_phase(fptr buffer, fptr libs, fptr pmp, fptr palette);
int fight_step(fptr rec);
long score(void);
void history_add(int units0, int units1);
void after_map(fptr buffer);
int save_game(fptr buffer);
void load_game(fptr buffer);
void play_anim(int number, fptr buffer, int a, int b, int c, fptr path);
void end_credits(fptr work, fptr path);
void computer_start(int side);
void computer_step(int side);
void computer_unit_new(int unit);
void t262a_000e(fptr text);

/* ---- battle.c: T0708, the program ---- */
void battle_main(int argc, char **argv);

#endif
