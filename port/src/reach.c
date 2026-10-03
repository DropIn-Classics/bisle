/* reach.c - BATTLE.EXE's T0BA0: where a unit can move and fire, and the
 * way to a square (BATTLE.hints at clear_marks; tools/moves.py has the
 * rules).
 *
 * The buffers these routines fill are a byte a square in rows of 64, as
 * the marks.
 */
#include "bi.h"

#define SQUARES 0x1104

/* the bits of `mask` cleared in all marks (FFh: all bits) */
void clear_marks(int mask)
{
    unsigned i;

    mask &= 0xFF;
    for (i = 0; i < SQUARES; i++)
        SBO(marks, i, mask == 0xFF ? 0 : GBO(marks, i) & ~mask);
}

/* the squares of `around` that are off the map get FFFFh */
static void off_map(int col, int row)
{
    int w = (int16_t)GW(map_width), h = (int16_t)GW(map_height);

    if (col & 1) {
        if (row <= 0)
            SWO(around, 0, 0xFFFF);
        else if (h - 1 <= row)
            SWO(around, 6, 0xFFFF), SWO(around, 4, 0xFFFF), SWO(around, 8, 0xFFFF);
        if (col <= 0)
            SWO(around, 0x0A, 0xFFFF), SWO(around, 8, 0xFFFF);
        else if (w - 1 <= col)
            SWO(around, 2, 0xFFFF), SWO(around, 4, 0xFFFF);
    } else {
        if (row <= 0)
            SWO(around, 0, 0xFFFF), SWO(around, 0x0A, 0xFFFF), SWO(around, 2, 0xFFFF);
        else if (h - 1 <= row)
            SWO(around, 6, 0xFFFF);
        if (col <= 0)
            SWO(around, 8, 0xFFFF), SWO(around, 0x0A, 0xFFFF);
        else if (w - 1 <= col)
            SWO(around, 2, 0xFFFF), SWO(around, 4, 0xFFFF);
    }
}

/* the six squares around one into `around`, as offsets in a buffer of
 * rows of 64: up, right up, right down, down, left down, left up (a
 * square of an odd column lies half a row lower) */
void neighbours64(int col, int row)
{
    int at;

    if (bi_prog == BI_MOON) {
        /* MOON.EXE puts a column and a row outside the map on its edge first */
        int w = (int16_t)GW(map_width), h = (int16_t)GW(map_height);

        if (w <= col)
            col = w - 1;
        if (col < 0)
            col = 0;
        if (h <= row)
            row = h - 1;
        if (row < 0)
            row = 0;
    }
    at = col + (row << 6);

    SWO(around, 0, at - 0x40);
    SWO(around, 6, at + 0x40);
    if (col & 1) {
        SWO(around, 2, at + 1);
        SWO(around, 0x0A, at - 1);
        SWO(around, 8, at + 0x3F);
        SWO(around, 4, at + 0x41);
    } else {
        SWO(around, 4, at + 1);
        SWO(around, 8, at - 1);
        SWO(around, 0x0A, at - 0x41);
        SWO(around, 2, at - 0x3F);
    }
    off_map(col, row);
}

/* the same for a square of the map, as offsets in the map */
void neighbours(int off)
{
    int w = (int16_t)GW(map_width), w2 = w << 1;
    int col = ((int16_t)off >> 1) % w, row = ((int16_t)off >> 1) / w;

    SWO(around, 0, off - w2);
    SWO(around, 6, off + w2);
    if (col & 1) {
        SWO(around, 2, off + 2);
        SWO(around, 0x0A, off - 2);
        SWO(around, 8, off + w2 - 2);
        SWO(around, 4, off + w2 + 2);
    } else {
        SWO(around, 4, off + 2);
        SWO(around, 8, off - 2);
        SWO(around, 2, off - w2 + 2);
        SWO(around, 0x0A, off - w2 - 2);
    }
    off_map(col, row);
}

/* what entering each square costs the unit, negated: the ground's cost
 * (+3; +4 for a unit with 10h in its +4; FEh for one with 10h in its
 * +6), B0h where it cannot go (the type's ground mask, the ground flags
 * `flags`, a unit of the other side or of nobody), 9Ah or 9Bh on the
 * squares around a unit of the other side, FEh into a unit of the own
 * side that holds others */
static void cost_map(fptr buf, int unit, int side, int flags, fptr map)
{
    fptr mover = UNIT(unit & 0xFF), type = TYPE(pb(mover, 8));
    int w = (int16_t)GW(map_width), h = (int16_t)GW(map_height), w2 = w << 1, row, col, kind, k;
    unsigned cls = pw(type, 0x0C), mask = pb(type, 4), i;

    for (i = 0; i < SQUARES; i++)
        spb(buf, i, 0xFF);
    kind = (pw(mover, 4) & 0x10) ? 0 : 2;
    if (pw(mover, 6) & 0x10)
        kind = 1;
    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++) {
            unsigned at = (unsigned)(row << 6) + (unsigned)col;
            fptr g = GROUND(pb(map, (unsigned)(row * w2 + 2 * col)));
            unsigned v;

            if (!(pb(g, 2) & mask))
                v = 0xB0;
            else if (kind == 0)
                v = (unsigned)-pb(g, 4);
            else if (kind == 1)
                v = 0xFE;
            else
                v = (unsigned)-pb(g, 3);
            if (flags != -1 && (flags & 0xFFFF) != 0xFFFF && (pw(g, 0) & (unsigned)flags))
                v = 0xB0;
            /* MOON (T0C0E:0BBB): a unit of two squares not onto a
             * building */
            if (bi_prog == BI_MOON && (pw(g, 0) & 0x540) && (pw(mover, 4) & 0x40))
                v = 0xB0;
            spb(buf, at, v);
        }
    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++) {
            unsigned at = (unsigned)(row << 6) + (unsigned)col, off = (unsigned)(row * w2 + 2 * col);
            unsigned b = pb(map, off + 1), f, f6;
            int theirs;

            if (b > 0xF0)
                continue;
            f = pw(UNIT(b), 4);
            if (f & 2)
                spb(buf, at, 0xB0);
            else {
                theirs = (f & 1) ? side != 1 : side != 0;
                if (theirs) {
                    spb(buf, at, 0xB0);
                    neighbours64(col, row);
                    for (k = 0; k < 6; k++) {
                        int a = (int16_t)GWO(around, 2 * k);

                        if (a >= 0 && pb(buf, (unsigned)a) != 0xB0)
                            spb(buf, (unsigned)a, (f & 1) ? 0x9A : 0x9B);
                    }
                } else if (f & (0x1000 | 0x2000))
                    spb(buf, at, 0xFE);
            }
            if ((cls & 0x40) && b != (unsigned)(unit & 0xFF))
                spb(buf, at, 0xB0);
            /* a unit the mover can take in */
            f6 = pw(UNIT(b), 6);
            if (((f6 & 4) && (pw(mover, 6) & 0x2000)) || ((f6 & 0x20) && (pw(mover, 6) & 0x1000))) {
                fptr g = GROUND(pb(map, off));

                if (kind == 0)
                    spb(buf, at, (unsigned)-pb(g, 4));
                else if (!(pb(g, 2) & mask))
                    spb(buf, at, 0xB0);
                else
                    spb(buf, at, kind == 1 ? 0xFE : (unsigned)-pb(g, 3));
            }
        }
}

/* The squares the unit reaches from the map's byte at `square` with
 * `points`: marked 1 (2 for side 1), their number returned.  `buf` gets
 * what is left of the points on each (below 0: not reached).  The
 * arguments are kept in reach_args for a call again. */
int reach(int square, fptr buf, int unit, int points, int side, int flags, fptr map)
{
    fptr a = FP(reach_args);
    int w = (int16_t)GW(map_width), h = (int16_t)GW(map_height);
    int x0, y0, x1, y1, row, col, k, level, count = 0;
    unsigned beside = side ? 0x9B : 0x9A, i, at;

    spw(a, 0, square);
    spfp(a, 2, buf);
    spw(a, 6, unit);
    spw(a, 8, points);
    spw(a, 0x0A, side);
    spw(a, 0x0C, flags);
    spfp(a, 0x0E, map);
    square = (int16_t)square >> 1;
    cost_map(buf, unit, side, flags, map);
    x0 = square % w - (points >> 1);
    if (x0 < 0)
        x0 = 0;
    y0 = square / w - (points >> 1);
    if (y0 < 0)
        y0 = 0;
    x1 = x0 + points;
    if (w < x1)
        x1 = w;
    y1 = y0 + points;
    if (h < y1)
        y1 = h;
    at = (unsigned)(square % w + ((square / w) << 6));
    spb(buf, at, points);
    for (level = (int8_t)pb(buf, at); level > 0; level--)
        for (row = y0; row < y1; row++)
            for (col = x0; col < x1; col++) {
                int here = (int8_t)pb(buf, (unsigned)(row << 6) + (unsigned)col);

                if (here != level)
                    continue;
                neighbours64(col, row);
                for (k = 0; k < 6; k++) {
                    int n = (int16_t)GWO(around, 2 * k);

                    if (n < 0 || (int8_t)pb(buf, (unsigned)n) >= 0)
                        continue;
                    if (pb(buf, (unsigned)n) == beside)
                        spb(buf, (unsigned)n, 0);
                    else if ((int8_t)pb(buf, (unsigned)n) + here >= 0)
                        spb(buf, (unsigned)n, pb(buf, (unsigned)n) + (unsigned)here);
                }
            }
    for (i = 0; i < SQUARES; i++)
        if ((int8_t)pb(buf, i) >= 0) {
            count++;
            SBO(marks, i, GBO(marks, i) | (side == 1 ? 2 : 1));
        }
    return count;
}

/* The units the unit can fire at from `square`: those of the other side
 * less than `range` squares away (not the six around it when `targets`
 * has 40h) whose class `targets` names, marked 4 (8 for side 1); their
 * number. */
int fire_reach(fptr buf, int square, int range, int side, int unit, int targets, fptr map)
{
    int w = (int16_t)GW(map_width), h = (int16_t)GW(map_height), w2 = w << 1;
    int x0, y0, x1, y1, row, col, k, level, sq;
    unsigned i, at, count = 0;

    for (i = 0; i < SQUARES; i++)
        spb(buf, i, 0xFF);
    square = (int16_t)square >> 1;
    x0 = square % w - range;
    if (x0 < 0)
        x0 = 0;
    y0 = square / w - range;
    if (y0 < 0)
        y0 = 0;
    x1 = x0 + (range << 1) + 1;
    if (x1 > w)
        x1 = w;
    y1 = y0 + (range << 1) + 1;
    if (h < y1)
        y1 = h;
    at = (unsigned)(square % w + ((square / w) << 6));
    spb(buf, at, pb(buf, at) + (unsigned)range);
    for (level = (int8_t)pb(buf, at); level > 0; level--)
        for (row = y0; row < y1; row++)
            for (col = x0; col < x1; col++) {
                int here = (int8_t)pb(buf, (unsigned)(row << 6) + (unsigned)col);

                if (here != level)
                    continue;
                neighbours64(col, row);
                for (k = 0; k < 6; k++) {
                    int n = (int16_t)GWO(around, 2 * k);

                    if (n < 0 || (int8_t)pb(buf, (unsigned)n) >= 0)
                        continue;
                    if ((int8_t)pb(buf, (unsigned)n) + here >= 0)
                        spb(buf, (unsigned)n, pb(buf, (unsigned)n) + (unsigned)here);
                }
            }
    sq = (int16_t)(pw(UNIT(unit), 0x0B + 2 * side) - 1) >> 1;
    if (targets & 0x40) {
        neighbours64(sq % w, sq / w);
        for (k = 0; k < 6; k++)
            if ((int16_t)GWO(around, 2 * k) >= 0)
                spb(buf, GWO(around, 2 * k), 0xFF);
    }
    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++) {
            unsigned b, f;

            at = (unsigned)(row << 6) + (unsigned)col;
            if ((int8_t)pb(buf, at) < 0)
                continue;
            spb(buf, at, 0xFF);
            b = pb(map, (unsigned)(row * w2 + 2 * col) + 1);
            if (b > 0xF0)
                continue;
            f = pw(UNIT(b), 4);
            if ((f & 2) || !((f & 1) ? side != 1 : side != 0))
                continue;
            if (f & (unsigned)targets & 0x3C)
                spb(buf, at, 1);
            f = pw(UNIT(b), 6);
            if ((f & 2) && (f & (unsigned)targets & 2))
                spb(buf, at, 1);
        }
    for (i = 0; i < SQUARES; i++)
        if ((int8_t)pb(buf, i) > 0) {
            SBO(marks, i, GBO(marks, i) | ((side & 1) ? 8 : 4));
            count++;
        }
    return (int)(count & 0xFF);
}

/* A way over the squares marked in reach from the map's byte `from` to
 * `to`: 0, the path at list + 1F40h as words (the squares' offsets from
 * the aim back to the start) and its length in path_count; or -1.
 * `list` holds nodes of 10 bytes (column, row, +2 the node it was
 * reached from, +4 a key, +6 and +8 the nodes before and after in the
 * key's order); the key is the distance to the aim times the type's
 * +14h plus the ground's cost times its +15h.
 *
 * Not the original's: the count of `steps`.  When the aim is not reached
 * the original goes on with the list's last node (2, the key 7530h) as
 * with a square, at whatever column and row its bytes hold; a marked
 * square beside that, not taken yet, is then put in from node 0 on, and
 * the search for its place can go round for ever (seen in a game of two
 * computers on map 30, the original stopping there as well).  A list has
 * no more than 316h nodes, so a search of more steps is such a round:
 * no path. */
int find_path(fptr list, int unit, int from, int to, int side, fptr map)
{
#define NODE(n) MKFP(FSEG(list), FOFF(list) + 10 * (n))
    int w = (int16_t)GW(map_width), w2 = w << 1, k;
    unsigned in_reach = side ? 2 : 1, taken = side ? 0x80 : 0x40;
    fptr type = TYPE(pb(UNIT(unit), 8)), path = MKFP(FSEG(list), FOFF(list) + 0x1F40);
    int by_distance = (int8_t)pb(type, 0x14), by_cost = (int8_t)pb(type, 0x15);
    int tx = (int8_t)(((int16_t)to >> 1) % w), ty = (int8_t)(((int16_t)to >> 1) / w);
    unsigned next = 3, at = 0;

    SW(path_count, 0);
    spw(NODE(0), 8, 1);
    spb(NODE(1), 0, ((int16_t)from >> 1) % w);
    spb(NODE(1), 1, ((int16_t)from >> 1) / w);
    spw(NODE(1), 2, 1);
    spw(NODE(1), 4, 0);
    spw(NODE(1), 6, 0);
    spw(NODE(1), 8, 2);
    if (bi_prog == BI_MOON)
        spb(NODE(2), 0, 0), spb(NODE(2), 1, 0);
    spw(NODE(2), 4, 0x7530);
    spw(NODE(2), 6, 1);
    spw(NODE(2), 8, 0);
    for (;;) {
        at = pw(NODE(at), 8);
        if (!at)
            break;
        if ((int8_t)pb(NODE(at), 0) == tx && (int8_t)pb(NODE(at), 1) == ty) {
            spw(path, 0, w2 * ty + (tx << 1));
            SW(path_count, GW(path_count) + 1);
            while ((int16_t)at > 1) {
                at = pw(NODE(at), 2);
                spw(path, 2 * GW(path_count),
                    (int8_t)pb(NODE(at), 1) * w2 + ((int8_t)pb(NODE(at), 0) << 1));
                SW(path_count, GW(path_count) + 1);
                if ((int16_t)GW(path_count) > 0x316)
                    goto none;
            }
            clear_marks((int)taken);
            return 0;
        }
        if (bi_prog == BI_MOON && at == 2)
            break;                      /* MOON.EXE: the end node is no square */
        neighbours64((int8_t)pb(NODE(at), 0), (int8_t)pb(NODE(at), 1));
        for (k = 0; k < 6; k++) {
            int n = (int16_t)GWO(around, 2 * k), nx, ny, distance, key, cost, steps = 0;
            unsigned m, j, before;

            if (n < 0)
                continue;
            m = GBO(marks, (unsigned)n);
            if (!(m & in_reach) || (m & taken))
                continue;
            nx = (int8_t)(n % 0x40);
            ny = (int8_t)(n / 0x40);
            distance = (int8_t)square_distance(nx, ny, tx, ty);
            cost = (int8_t)pb(GROUND(pb(map, (unsigned)(w2 * ny + (nx << 1)))),
                              (pw(UNIT(unit), 4) & 0x10) ? 4 : 3);
            key = distance ? (int16_t)(distance * by_distance) + (int16_t)(cost * by_cost) : 0;
            key = (int16_t)key;
            for (j = pw(NODE(at), 8); (int16_t)pw(NODE(j), 4) <= key; j = pw(NODE(j), 8))
                if (++steps > 0x316)
                    goto none;
            spb(NODE(next), 0, nx);
            spb(NODE(next), 1, ny);
            spw(NODE(next), 4, key);
            spw(NODE(next), 2, at);
            before = pw(NODE(j), 6);
            spw(NODE(next), 6, before);
            spw(NODE(next), 8, j);
            spw(NODE(before), 8, next);
            spw(NODE(j), 6, next);
            SBO(marks, (unsigned)n, GBO(marks, (unsigned)n) | taken);
            next++;
            if ((int16_t)next > 0x316)
                goto none;
        }
    }
none:
    clear_marks((int)taken);
    SW(path_count, 0);
    return -1;
#undef NODE
}

/* the squares in reach on which stop_check lets the unit stop, as words
 * into `list`, their number in path_count */
void list_reach(fptr list, int side, int unit)
{
    fptr map = side ? GFP(map1) : GFP(map0);
    int w = (int16_t)GW(map_width), h = (int16_t)GW(map_height), col, row;

    SW(path_count, 0);
    SB(move_record, unit);
    for (col = 0; col < w; col++)
        for (row = 0; row < h; row++) {
            int off;

            if (!(GBO(marks, (unsigned)(row << 6) + (unsigned)col) & (side ? 2 : 1)))
                continue;
            off = (w * row + col) << 1;
            if (stop_check(off, side, map))
                continue;
            spw(list, 2 * GW(path_count), off);
            SW(path_count, GW(path_count) + 1);
            if ((int16_t)GW(path_count) >= 0x316)
                return;
        }
}
