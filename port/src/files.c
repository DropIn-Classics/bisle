/* files.c - BATTLE.EXE's T2619..T2728 and T164D's paths: blocks of
 * memory from DOS, files read whole (the packed ones unpacked, TPWM),
 * and the names of files.
 */
#include <stdio.h>
#include <string.h>
#include "bi.h"

/* ---- memory ---- */

/* T2619:0004: a block of size / 16 + 1 paragraphs from DOS, kept in the
 * list of blocks; 0 when there is none (file_error 1) */
fptr t2619_0004(uint32_t size)
{
    uint16_t seg = dos_alloc((uint16_t)((size >> 4) + 1));
    unsigned n;

    if (!seg) {
        SW(file_error, 1);
        return 0;
    }
    n = GW(blocks_count);
    SW(blocks_count, n + 1);
    SWO(blocks, 2 * n, seg);
    return MKFP(seg, 0);
}

void t261e_0008(void)
{
    unsigned n = GW(blocks_count), i;

    for (i = 0; i < n; i++)
        dos_free(GWO(blocks, 2 * i));
    if (n)
        SW(blocks_count, 0);
}

void t2621_0008(void)
{
    unsigned n = GW(blocks_count);

    if (!n)
        return;
    n--;
    SW(blocks_count, n);
    dos_free(GWO(blocks, 2 * n));
}

/* ---- files ---- */

int file_open(int mode, fptr name)
{
    return dos_open(fstr(name), mode == 1);
}

int file_close(int handle)
{
    dos_close(handle);
    return 0;
}

/* a file's length, of a packed file the length unpacked; -1 when it
 * cannot be opened */
long file_size(fptr name)
{
    uint8_t head[8];
    int h = file_open(0, name);
    long size;

    if (!h)
        return -1;
    /* the original reads the 8 bytes into its code segment (T2653:00B5) */
    size = dos_read_to(h, head, 8);
    if (size == 8 && !memcmp(head, "TPWM", 4))
        size = head[4] | head[5] << 8 | (long)head[6] << 16 | (long)head[7] << 24;
    else
        size = dos_seek(h, 0, 2);
    file_close(h);
    return size;
}

/* T2695's reading of a packed file: a byte at a time from 1000h bytes
 * read into the caller's buffer.  Past the file's end the buffer keeps
 * what it held: the original takes its last byte from there (below). */
static int rd_handle;
static uint32_t rd_work;
static unsigned rd_pos;

static uint8_t rd_byte(void)
{
    if (rd_pos >= 0x1000) {
        dos_read(rd_handle, rd_work, 0x1000);
        rd_pos = 0;
    }
    return mem[(rd_work + rd_pos++) & (MEM_SIZE - 1)];
}

/* T2695:02BB: the message for a disk, until a key.  What is put over the
 * dots 10 bytes before the second byte after the message's first '|':
 * with disk_record's second word 0 the disk's name in the record, above 0
 * the file's name, below 0 the string behind the file's name; from its end
 * up to the next '|' spaces. */
static void ask_disk(fptr name)
{
    fptr msg = FP(disk_message), src = FP(disk_record) + 4;
    uint16_t seg = FSEG(msg), di = FOFF(msg), w = GWO(disk_record, 2);
    uint8_t c;

    while (frb(seg, di++) != 0x7C)
        ;
    di = (uint16_t)(di + 1 - 0x0A);
    if (w) {
        src = name;
        if ((int16_t)w < 0) {
            while (frb(FSEG(src), FOFF(src)))
                src = MKFP(FSEG(src), FOFF(src) + 1);
            src = MKFP(FSEG(src), FOFF(src) + 1);
        }
    }
    do {
        c = frb(FSEG(src), FOFF(src));
        src = MKFP(FSEG(src), FOFF(src) + 1);
        fwb(seg, di++, c);
    } while (c);
    for (di--; frb(seg, di) != 0x7C; di++)
        fwb(seg, di, ' ');
    t262a_000e(msg);
}

/* The file `name` into memory at `dest`, or into a new block when dest is
 * 0 or FFFF:FFFF; a file that begins TPWM is unpacked (tools/tpwmfiles.py
 * has the format), `work` the 1000h bytes it is read through.  Returns
 * where it is (never with offset 0: seg - 1 and offset 10h then), 0 when
 * it cannot be read.
 * As the original: an unpacked file is read to its end, and one byte more
 * than its length says is written of a packed one. */
fptr load_file(fptr dest, fptr name, fptr work)
{
    uint8_t head[8];
    uint32_t at, left;
    fptr where;
    int h, tries = 4, bit;
    long n;

    for (;;) {
        SW(file_error, 0);
        h = file_open(0, name);
        if (h && !GW(file_error))
            break;
        /* the original asks for the disk while disk_record's first word
         * is set, three times */
        if (--tries == 0 || !GW(disk_record))
            return 0;
        ask_disk(name);
    }
    /* the 8 bytes go into the code segment (T2695:03CF) in the original */
    n = dos_read_to(h, head, 8);
    if (n == 8 && !memcmp(head, "TPWM", 4)) {
        left = head[4] | head[5] << 8 | (uint32_t)head[6] << 16 | (uint32_t)head[7] << 24;
        if (dest == 0 || dest == 0xFFFFFFFFu)
            dest = t2619_0004(left);
        where = hnorm(dest);
        at = flin(where);
        rd_handle = h;
        rd_work = flin(work);
        rd_pos = 0x1001;
        for (;;) {
            unsigned flags = rd_byte();

            for (bit = 0; bit < 8; bit++, flags <<= 1) {
                if (!(flags & 0x80)) {
                    mem[at++ & (MEM_SIZE - 1)] = rd_byte();
                    if (left-- == 0)
                        goto done;
                } else {
                    unsigned b1 = rd_byte(), b2 = rd_byte();
                    unsigned back = (b1 >> 4) << 8 | b2, count = (b1 & 0x0F) + 3;
                    uint32_t from = at - back;

                    while (count--) {
                        mem[at++ & (MEM_SIZE - 1)] = mem[from++ & (MEM_SIZE - 1)];
                        if (left-- == 0)
                            goto done;
                    }
                }
            }
        }
    } else {
        dos_seek(h, 0, 0);
        if (dest == 0 || dest == 0xFFFFFFFFu) {
            long size = dos_seek(h, 0, 2);

            dos_seek(h, 0, 0);
            dest = t2619_0004((uint32_t)size);
            if (!FSEG(dest))
                return 0;                 /* as the original: the file stays open */
        }
        where = hnorm(dest);
        /* in pieces up to the segment's end, then of 8000h, to the first
         * short one */
        at = flin(dest);
        if (FOFF(dest)) {
            long piece = 0x10000L - FOFF(dest);

            n = dos_read(h, at, piece);
            if (n != piece)
                goto done;
            at += (uint32_t)piece;
        }
        do {
            n = dos_read(h, at, 0x8000);
            at += 0x8000;
        } while (n == 0x8000);
    }
done:
    file_close(h);
    if (FOFF(where) == 0)
        where = MKFP(FSEG(where) - 1, 0x10);
    return where;
}

long t2624_0006(fptr dest, fptr name, fptr work, fptr *loaded)
{
    fptr p = load_file(dest, name, work);

    if (loaded)
        *loaded = p;
    return file_size(name);
}

/* T264B:0000 and T26DE:0008: `count` bytes read from, written to an open
 * file in pieces of at most 8000h; 0, or -1 */
int t264b_0000(int handle, fptr to, uint32_t count)
{
    return dos_read(handle, flin(to), (long)count) == (long)count ? 0 : -1;
}

int t26de_0008(int handle, fptr from, uint32_t count)
{
    return dos_write(handle, flin(from), (long)count) == (long)count ? 0 : -1;
}

/* a file made of `count` bytes at `from`; 0, or FFh */
int save_file(fptr name, fptr from, uint32_t count)
{
    int h = file_open(1, name);
    long n;

    if (!h)
        return 0xFF;
    n = dos_write(h, flin(from), (long)count);
    file_close(h);
    return n == (long)count ? 0 : 0xFF;
}

/* ---- names ---- */

/* T164D:0363: a path in path_made: the directory `dir` of path_dirs (6
 * bytes each: LIB\, MAP\, FX\, ANIM\; below 0 none), the name, the
 * extension `ext` of path_exts (5 bytes each; below 0 none).  The first
 * argument is not used. */
fptr make_path(int unused, int dir, fptr name, int ext)
{
    char *out = fstr(FP(path_made));

    (void)unused;
    out[0] = 0;
    if (dir >= 0)
        strcpy(out, fstr(FP(path_dirs)) + 6 * dir);
    if (name)
        strcat(out, fstr(name));
    if (ext >= 0)
        strcat(out, fstr(FP(path_exts)) + 5 * ext);
    return FP(path_made);
}

/* T164D:0482: the disk's name (8 bytes each: INTRO, VGA, SAVE) into the
 * disk's record */
void t164d_0482(int disk)
{
    strcpy(fstr(FP(disk_record)) + 4, fstr(FP(disk_names)) + 8 * disk);
}
