/* lib.c - BATTLE.EXE's T0CEB: the libraries of sprites. */
#include <string.h>
#include "bi.h"

/* T0CEB:036A: a library's directory at `dir` put into the order of the
 * names in its .DAT file (8 bytes a name), which is loaded behind the
 * library, at `end`: the records of the names found, in that order, are
 * gathered behind the names and copied back over the directory.  A name
 * the library does not have leaves a record's place to what was there. */
static void sort_lib(fptr end, fptr dir, fptr record, fptr work)
{
    long size = t2624_0006(end, make_path(1, 0, record, 5), work, NULL);
    fptr out, to;
    int n, i, j;

    if (size == -1)
        return;
    out = hadd(end, size);
    to = out;
    n = (int16_t)size / 8;
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            if (!strncmp(fstr(MKFP(FSEG(dir), FOFF(dir) + 12 * j)), fstr(MKFP(FSEG(end), FOFF(end) + 8 * i)), 8)) {
                t0d36_15e9(hadd(dir, 12L * (int16_t)j), to, 12);
                to = hadd(to, 12);
                break;
            }
    t0d36_15e9(out, dir, (uint32_t)(12L * n));
}

/* The library file `path` loaded to `dest`: a long (where its directory
 * is), the entries, the directory of 12-byte records (a name of 8, the
 * entry's offset).  The directory is made a table of far pointers to the
 * entries, in place; the record gets the file's length (+0Ah), the table
 * (+0Eh) and the number of entries (+12h).  With `sorted` the directory
 * is first put into the order of the library's .DAT file.  Returns the
 * length the library takes, -1 when it cannot be loaded. */
long load_lib(fptr path, fptr dest, fptr work, fptr record, int sorted)
{
    fptr base = dest, dir, table;
    long size = t2624_0006(dest, path, work, NULL);
    uint32_t offset;
    unsigned i, n;

    if (size == -1)
        return -1;
    offset = pd(dest, 0);
    if (sorted)
        sort_lib(hadd(dest, size), hadd(dest, (long)offset), record, work);
    for (i = 0; i < 0x0C; i++)
        spb(hadd(hadd(dest, size), i), 0, 0);
    dir = hadd(dest, (long)offset);
    table = dir;
    spw(record, 0x12, 0);
    spfp(record, 0x0E, dir);
    spd(record, 0x0A, (uint32_t)size);
    do {
        dir = hadd(dir, 8);
        offset = pb(dir, 0);
        dir = hadd(dir, 1);
        offset |= (uint32_t)pb(dir, 0) << 8;
        dir = hadd(dir, 1);
        offset |= (uint32_t)pb(dir, 0) << 16;
        dir = hadd(dir, 1);
        offset |= (uint32_t)pb(dir, 0) << 24;
        dir = hadd(dir, 1);
        /* the entry's far pointer: the offset, then the segment */
        spw(table, 0, FOFF(hadd(base, (long)offset)));
        spw(hadd(table, 2), 0, FSEG(hadd(base, (long)offset)));
        table = hadd(table, 4);
        spw(record, 0x12, pw(record, 0x12) + 1);
    } while (offset);
    spw(record, 0x12, pw(record, 0x12) - 1);
    n = pw(record, 0x12);
    size -= 0x0CL * (int16_t)n;
    size += (long)(int16_t)n << 3;
    return size;
}
