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
