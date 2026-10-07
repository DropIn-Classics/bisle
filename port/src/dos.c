/* dos.c - what BATTLE.EXE got from DOS, the BIOS and the PC's hardware:
 * files, the keyboard, the timer's interrupt and the screen's retrace.
 *
 * Files: the program runs in the game's ISLE folder and names its files
 * as DOS did (LIB\CHAR24.LIB); a file it creates (a saved game, a map's
 * scores) goes into the port's own folder, where it is looked for first.
 *
 * The clock counts the PIT's counts (1193182 a second).  The program
 * waits in a few ways only: for the vertical retrace (wait_retrace,
 * flip_page), for a count of its timer's ticks, and a scan line at a time
 * in set_palette; each of them moves this clock on, the timer's interrupt
 * (int08, timer.c) runs when it is due and a picture is shown at each
 * retrace (frame.h), which also paces the program against real time.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bi.h"
#include "frame.h"
#include "platform.h"
#include "sys.h"
#include "vga.h"

/* ---- files ---- */

static char game_dir[SYS_PATH], save_dir[SYS_PATH];
static FILE *files[16];

void dos_set_dirs(const char *game, const char *saves)
{
    snprintf(game_dir, sizeof game_dir, "%s", game);
    snprintf(save_dir, sizeof save_dir, "%s", saves);
}

/* `name` (DOS's, with backslashes) below `dir`, each part found whatever
 * its case; 1 if all but the last were found (the last as given when it
 * is not there, for a file to create, its folder made with `make`) */
static int resolve(const char *dir, const char *name, int make, char *out, size_t n)
{
    char part[64], next[SYS_PATH];
    size_t len;
    int found = 1;

    snprintf(out, n, "%s", dir);
    while (*name) {
        len = strcspn(name, "\\/");
        if (len >= sizeof part)
            return 0;
        memcpy(part, name, len);
        part[len] = 0;
        name += len;
        if (*name)
            name++;
        if (!len || !strcmp(part, "."))
            continue;
        if (sys_find(out, part, next, sizeof next))
            snprintf(out, n, "%s", next);
        else {
            if (*name) {                      /* a folder that is not there */
                if (!make)
                    return 0;
                sys_join(out, n, out, part);
                sys_mkdir(out);
            } else {
                sys_join(out, n, out, part);
                found = 0;
            }
        }
    }
    return make || found;
}

int dos_open(const char *name, int create)
{
    char path[SYS_PATH];
    FILE *f = NULL;
    int h;

    for (h = 5; h < 16 && files[h]; h++)
        ;
    if (h == 16)
        return 0;
    if (create) {
        if (resolve(save_dir, name, 1, path, sizeof path))
            f = fopen(path, "wb+");
    } else {
        if (resolve(save_dir, name, 0, path, sizeof path))
            f = fopen(path, "rb");
        if (!f && resolve(game_dir, name, 0, path, sizeof path))
            f = fopen(path, "rb");
    }
    if (!f)
        return 0;
    files[h] = f;
    return h;
}

void dos_close(int handle)
{
    if (handle >= 0 && handle < 16 && files[handle]) {
        fclose(files[handle]);
        files[handle] = NULL;
    }
}

long dos_read_to(int handle, void *to, long count)
{
    if (handle < 0 || handle >= 16 || !files[handle])
        return -1;
    return (long)fread(to, 1, (size_t)count, files[handle]);
}

long dos_read(int handle, uint32_t at, long count)
{
    if (at + (uint32_t)count > MEM_SIZE)
        return -1;
    return dos_read_to(handle, mem + at, count);
}

long dos_write(int handle, uint32_t at, long count)
{
    if (handle < 0 || handle >= 16 || !files[handle] || at + (uint32_t)count > MEM_SIZE)
        return -1;
    return (long)fwrite(mem + at, 1, (size_t)count, files[handle]);
}

long dos_seek(int handle, long offset, int whence)
{
    if (handle < 0 || handle >= 16 || !files[handle])
        return -1;
    if (fseek(files[handle], offset, whence == 2 ? SEEK_END : whence == 1 ? SEEK_CUR : SEEK_SET))
        return -1;
    return ftell(files[handle]);
}

/* ---- the keyboard ---- */

static int own_keyboard;
static unsigned key_queue[16];
static int key_head, key_count;
static int shift, e0;

/* the characters of the scancodes 00h..39h, plain and with shift */
static const char plain[0x3B] =
    "\0\x1B" "1234567890-=\b\tqwertyuiop[]\r\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
static const char shifted[0x3B] =
    "\0\x1B" "!@#$%^&*()_+\b\tQWERTYUIOP{}\r\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 ";

/* the BIOS's INT 09h: a key pressed goes into its buffer */
static void bios_int09(unsigned char b)
{
    unsigned code = b & 0x7F, ch = 0;

    if (b == 0xE0) {
        e0 = 1;
        return;
    }
    if (!e0 && (code == 0x2A || code == 0x36)) {
        shift = !(b & 0x80);
        e0 = 0;
        return;
    }
    if (b & 0x80) {
        e0 = 0;
        return;
    }
    if (!e0 && code < 0x3A)
        ch = (unsigned char)(shift ? shifted : plain)[code];
    if (e0 && code == 0x1C)
        ch = '\r';
    e0 = 0;
    if (code == 0x1D || code == 0x38 || code == 0x3A)   /* Ctrl, Alt, Caps Lock */
        return;
    if (key_count < 16) {
        key_queue[(key_head + key_count) & 15] = code << 8 | ch;
        key_count++;
    }
}

static void key_byte(unsigned char b)
{
    if (own_keyboard)
        int09(b);
    else
        bios_int09(b);
}

void dos_keyboard_own(int on)
{
    own_keyboard = on;
    e0 = 0;
}

int bios_key_waits(void)
{
    return key_count > 0;
}

unsigned bios_key(void)
{
    unsigned k = key_queue[key_head];

    key_head = (key_head + 1) & 15;
    key_count--;
    return k;
}

/* ---- the clock ---- */

#define PIT_HZ 1193182.0

static double now, next_irq, next_frame;
static unsigned period = 0x10000, period_set = 0x10000;
static int irqs_on, started;

static double frame_counts(void)
{
    double hz = vga_refresh_hz();

    return PIT_HZ / (hz > 10.0 ? hz : 70.0);
}

double clock_seconds(void)
{
    return now / PIT_HZ;
}

void clock_set_period(unsigned counts)
{
    period_set = counts ? counts : 0x10000;
    if (!irqs_on) {
        irqs_on = 1;
        period = period_set;
        next_irq = now + period;
    }
}

static void start(void)
{
    if (started)
        return;
    started = 1;
    frame_set_keyboard(key_byte);
    next_frame = now + frame_counts();
}

static void picture(void)
{
    if (!frame_wait())
        bi_exit(0);
}

void clock_run(long counts, int interrupts_off)
{
    double end = now + counts;

    start();
    for (;;) {
        int irq = irqs_on && !interrupts_off && next_irq <= end;
        int frame = next_frame <= end;

        if (irq && (!frame || next_irq <= next_frame)) {
            if (next_irq > now)
                now = next_irq;
            /* one interrupt for all that were held off: the others are lost */
            while (next_irq <= now)
                next_irq += period;
            period = period_set;
            int08();
        } else if (frame) {
            now = next_frame;
            next_frame += frame_counts();
            picture();
        } else
            break;
    }
    now = end;
}

void clock_idle(void)
{
    start();
    if (!irqs_on) {
        clock_retrace();
        return;
    }
    clock_run((long)(next_irq - now) + 1, 0);
}

void clock_retrace(void)
{
    start();
    clock_run((long)(next_frame - now) + 1, 0);
}

void clock_lines(int lines)
{
    /* 449 lines a picture in the 400-line modes */
    clock_run((long)(lines * frame_counts() / 449.0), 1);
}

/* ---- for comparisons with the original ---- */

/* BI_SEEN=FILE: the names of the places a run came through (bi_seen:
 * the kinds of the computer player's steps), each once, to say what a
 * comparison covered */
void bi_seen(const char *what, int n)
{
    static char seen[256][24];
    static int count, off;
    static const char *path;
    char name[24];
    FILE *f;
    int i;

    if (off)
        return;
    if (!path) {
        path = getenv("BI_SEEN");
        if (!path) {
            off = 1;
            return;
        }
    }
    snprintf(name, sizeof name, "%s %X", what, n);
    for (i = 0; i < count; i++)
        if (!strcmp(seen[i], name))
            return;
    if (count == 256)
        return;
    strcpy(seen[count++], name);
    f = fopen(path, "a");
    if (f) {
        fprintf(f, "%s\n", name);
        fclose(f);
    }
}

/* A place the original passes too (a loop's start, a routine's entry),
 * counted by its name.
 * BI_BREAK=NAME#N: at the Nth pass of NAME the memory goes to $BI_RAM and
 * the video memory to $BI_VRAM, and the program ends.
 * BI_KEYSAT="NAME N:HEX N:HEX ...;NAME N:HEX ...": at the Nth pass of
 * NAME the keyboard sends the byte HEX (a scancode; E0 first for the grey
 * keys), as the runner's -keysat and -keyat do.
 * BI_MOUSEAT="NAME N:X,Y,B N:X,Y,B ...;NAME ...": at the Nth pass of NAME
 * the mouse's driver (mouse.c) is at X, Y (decimal, 0..639 and 0..199)
 * with the buttons B (1 left, 2 right, 4 middle), as the runner's -mouse
 * puts it at a time; it also makes the port a PC with a mouse.
 * BI_MOUSEMOVE="NAME N:DX,DY,B ...;NAME ...": at the Nth pass of NAME the
 * mouse was moved by DX, DY counts (decimal, signed) with the buttons B
 * held, as the window's mouse gives it (mouse.c, bi_mouse_move).
 * BI_POKE="NAME#N ADDR HEX ADDR HEX ...;NAME#N ...": at the Nth pass of
 * NAME the bytes HEX go to the linear address ADDR (hex), as the
 * runner's -poke does. */
void bi_at(const char *name)
{
    static struct { const char *name; long count; } places[32];
    static const char *brk, *keys, *pokes, *mice, *moves;
    static long brk_pass;
    const char *ram, *vram, *p;
    size_t len = strlen(name);
    long count;
    int i;

    if (!brk) {
        brk = getenv("BI_BREAK");
        if (!brk)
            brk = "";
        p = strchr(brk, '#');
        brk_pass = p ? atol(p + 1) : 1;
        keys = getenv("BI_KEYSAT");
        if (!keys)
            keys = "";
        pokes = getenv("BI_POKE");
        if (!pokes)
            pokes = "";
        mice = getenv("BI_MOUSEAT");
        if (!mice)
            mice = "";
        moves = getenv("BI_MOUSEMOVE");
        if (!moves)
            moves = "";
    }
    if (!*brk && !*keys && !*pokes && !*mice && !*moves)
        return;
    for (i = 0; i < 32 && places[i].name && strcmp(places[i].name, name) != 0; i++)
        ;
    if (i == 32)
        return;
    places[i].name = name;
    count = ++places[i].count;
    for (p = keys; *p; ) {
        const char *end = strchr(p, ';');
        size_t group = end ? (size_t)(end - p) : strlen(p);

        if (group > len && !strncmp(p, name, len) && p[len] == ' ') {
            const char *q = p + len;

            while (q < p + group) {
                char *e;
                long n = strtol(q, &e, 10);

                if (e == q || *e != ':')
                    break;
                q = e + 1;
                if (n == count)
                    key_byte((unsigned char)strtol(q, &e, 16));
                else
                    strtol(q, &e, 16);
                q = e;
            }
        }
        p += group;
        if (*p == ';')
            p++;
    }
    for (p = mice; *p; ) {
        const char *end = strchr(p, ';');
        size_t group = end ? (size_t)(end - p) : strlen(p);

        if (group > len && !strncmp(p, name, len) && p[len] == ' ') {
            const char *q = p + len;

            while (q < p + group) {
                char *e;
                long n = strtol(q, &e, 10), x, y, b;

                if (e == q || *e != ':')
                    break;
                x = strtol(e + 1, &e, 10);
                if (*e != ',')
                    break;
                y = strtol(e + 1, &e, 10);
                if (*e != ',')
                    break;
                b = strtol(e + 1, &e, 10);
                if (n == count)
                    bi_mouse_place((int)x, (int)y, (int)b);
                q = e;
            }
        }
        p += group;
        if (*p == ';')
            p++;
    }
    for (p = moves; *p; ) {
        const char *end = strchr(p, ';');
        size_t group = end ? (size_t)(end - p) : strlen(p);

        if (group > len && !strncmp(p, name, len) && p[len] == ' ') {
            const char *q = p + len;

            while (q < p + group) {
                char *e;
                long n = strtol(q, &e, 10), x, y, b;

                if (e == q || *e != ':')
                    break;
                x = strtol(e + 1, &e, 10);
                if (*e != ',')
                    break;
                y = strtol(e + 1, &e, 10);
                if (*e != ',')
                    break;
                b = strtol(e + 1, &e, 10);
                if (n == count)
                    bi_mouse_move((int)x, (int)y, (int)b);
                q = e;
            }
        }
        p += group;
        if (*p == ';')
            p++;
    }
    for (p = pokes; *p; ) {
        const char *end = strchr(p, ';');
        size_t group = end ? (size_t)(end - p) : strlen(p);

        if (group > len && !strncmp(p, name, len) && p[len] == '#') {
            char *e;
            const char *q;

            if (strtol(p + len + 1, &e, 10) == count)
                for (q = e; q < p + group; ) {
                    unsigned long at = strtoul(q, &e, 16);
                    int digit = 0, value = 0;

                    if (e == q)
                        break;
                    for (q = e; q < p + group && *q == ' '; q++)
                        ;
                    for (; q < p + group && *q != ' '; q++) {
                        value = value * 16 + (*q <= '9' ? *q - '0' : (*q | 0x20) - 'a' + 10);
                        if (++digit == 2) {
                            mem[at++ & (MEM_SIZE - 1)] = (uint8_t)value;
                            digit = value = 0;
                        }
                    }
                }
        }
        p += group;
        if (*p == ';')
            p++;
    }
    if (!*brk || strncmp(brk, name, len) != 0 || (brk[len] != '#' && brk[len] != 0))
        return;
    if (count != brk_pass)
        return;
    ram = getenv("BI_RAM");
    vram = getenv("BI_VRAM");
    if (ram)
        rm_write(ram);
    if (vram)
        vga_write_planes(vram);
    bi_exit(0);
}

/* ---- the end ---- */

void bi_exit(int code)
{
    int h;

    for (h = 0; h < 16; h++)
        dos_close(h);
    plat_shutdown();
    exit(code);
}

void bi_fatal(const char *text)
{
    plat_message(text);
    bi_exit(1);
}
