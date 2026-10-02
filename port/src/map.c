/* map.c - BATTLE.EXE's T0E9B: a map's files, the two windows on it and
 * the squares drawn again, the overview.
 *
 * A map is two bytes a square, the ground and the unit on it (F1h and
 * up: none), row by row; each player has a map of his own (map0, map1)
 * and a window of 9 columns by 7 rows on it, player 1's 160 pixels to
 * the right.  A square is 24 by 24 pixels, a column 16 wide, the odd
 * columns 12 lower.  The marks (reach, targets, a fight's squares) are a
 * byte a square in rows of 64, whatever the map's width.
 */
#include "bi.h"

/* the map of a player */
static fptr map_of(int player)
{
    return player ? GFP(map1) : GFP(map0);
}

/* where a square of the map is in the marks: column + 64 * row */
static unsigned mark_index(int square)
{
    int w = (int16_t)GW(map_width);

    return (uint16_t)(square % w + ((square / w) << 6));
}

/* T0E9B:000B: where the windows' rows and columns are on the screen */
void t0e9b_000b(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        SWO(window_y_even, 2 * i, 0x18 * i);
        SWO(window_y_odd, 2 * i, 0x18 * i + 0x0C);
    }
    for (i = 0; i < 0x0A; i++)
        SWO(window_x, 2 * i, 0x10 * i);
}

/* a map's .SHP file: for each unit type whether it can be made (27
 * bytes; the type's flag 2), then what the buildings hold: records of an
 * owner, the table (0 the headquarters, 1 the factories, 2 the depots,
 * else the units that hold others), the record's number, two bytes
 * (+16h, +17h: the energy; +18h) and seven unit types (above F0h none).
 * 0, or -1 when it cannot be loaded. */
int load_shp(fptr path, fptr dest, fptr work)
{
    unsigned n, i, slot, at = 0;

    dest = hnorm(dest);
    if (!load_file(dest, path, work))
        return -1;
    for (i = 0; i < 0x1B; i++) {
        fptr type = TYPE(i);

        if (pb(dest, at++) & 1)
            spw(type, 0x0E, pw(type, 0x0E) | 2);
        else
            spw(type, 0x0E, pw(type, 0x0E) & ~2);
    }
    n = pb(dest, at++);
    for (i = 0; i < n; i++) {
        unsigned owner = pb(dest, at), kind = pb(dest, at + 1), index = pb(dest, at + 2);
        fptr rec;

        rec = kind == 0 ? MKFP(S_hqs, A_hqs + 0x1C * index)
            : kind == 1 ? MKFP(S_factories, A_factories + 0x1C * index)
            : kind == 2 ? MKFP(S_depots, A_depots + 0x1C * index)
            : MKFP(S_cargo, A_cargo + 0x1C * index);
        spb(rec, 0x17, pb(dest, at + 3));
        spb(rec, 0x16, pb(dest, at + 3));
        spb(rec, 0x18, pb(dest, at + 4));
        at += 5;
        for (slot = 0; slot < 7; slot++, at++) {
            unsigned u;

            if (pb(dest, at) > 0xF0)
                continue;
            u = GB(units_made);
            SB(units_made, u + 1);
            make_unit(u, pb(dest, at), pw(rec, 0x0E), owner);
            spw(UNIT(u), 4, pw(UNIT(u), 4) | 0x4000);
            if (!(owner & 1) && !(owner & 2))
                SBO(players, 2, GBO(players, 2) + 1);
            else if (owner & 1)
                SBO(players, 0x17 + 2, GBO(players, 0x17 + 2) + 1);
            spb(rec, slot + 7, u);
            spb(rec, slot, u);
        }
    }
    return 0;
}

/* a map's .FIN file: the width and height, then the squares.  The
 * buildings' records are made from the ground (flags 400h a factory,
 * 100h a depot, 40h a headquarters; 1 and 2 the owner), the units from
 * the squares' second bytes (the type * 2 + the player; a type of class
 * 2 is nobody's, one of class 40h and 80h takes a second square, the row
 * below for player 1 and above for player 0).  0, or -1. */
int load_fin(fptr path, fptr dest, fptr work)
{
    unsigned i, off, bytes;
    fptr map;

    SBO(players, 0x17 + 2, 0);
    SBO(players, 0x17 + 4, 0);
    SBO(players, 0x17 + 3, 0);
    SBO(players, 2, 0);
    SBO(players, 4, 0);
    SBO(players, 3, 0);
    SB(depots_made, 0);
    SB(factories_made, 0);
    SB(units_made, 0);
    for (i = 0; i < 0x1B; i++) {
        spb(TYPE(i), 0x3C, 1);
        spb(TYPE(i), 0x3D, 1);
    }
    dest = hnorm(dest);
    if (!load_file(dest, path, work))
        return -1;
    SW(map_width, pb(dest, 1));
    SW(map_height, pb(dest, 3));
    map = MKFP(FSEG(dest), FOFF(dest) + 4);
    SFP(map0, map);
    bytes = (uint16_t)(GW(map_width) * GW(map_height) << 1);
    SW(map_bytes, bytes);
    for (i = 0; i <= 0xF0; i++)
        spw(UNIT(i), 4, 0x8000);
    for (i = 0; i < 0x0A; i++) {
        SWO(factories, 0x1C * i + 0x19, 0x8000);
        SWO(depots, 0x1C * i + 0x19, 0x8000);
    }
    for (i = 0; i < 0x46; i++)
        SWO(cargo, 0x1C * i + 0x19, 0x8000);
    for (off = 1; (int16_t)off < (int16_t)bytes; off += 2) {
        unsigned flags = pw(GROUND(pb(map, off - 1)), 0), owner, b, type, cls, player, w2;

        owner = (flags & 2) ? 2 : (flags & 1) ? 1 : 0;
        if (flags & 0x400) {
            t169e_0244(GB(factories_made), (int)(off - 1), (int)owner);
            SB(factories_made, GB(factories_made) + 1);
            if (owner != 2)
                SBO(players, 0x17 * owner + 3, GBO(players, 0x17 * owner + 3) + 1);
        } else if (flags & 0x100) {
            t169e_0324(GB(depots_made), (int)(off - 1), (int)owner);
            SB(depots_made, GB(depots_made) + 1);
            if (owner != 2)
                SBO(players, 0x17 * owner + 4, GBO(players, 0x17 * owner + 4) + 1);
        } else if (flags & 0x40)
            t169e_0401((int)(off - 1), (int)owner);
        b = pb(map, off);
        if (b >= 0xF4) {
            spb(map, off, 0xFF);
            continue;
        }
        type = b >> 1;
        if (type >= 0x1B || GB(units_made) > 0xF0)
            continue;
        cls = pw(TYPE(type), 0x0C);
        player = b & 1;
        w2 = GW(map_width) << 1;
        if (cls & 0x40) {
            if (cls & 0x80) {
                make_unit(GB(units_made), type, off, player);
                SB(units_made, GB(units_made) + 1);
                make_unit(GB(units_made), type + 1, player ? off + w2 : off - w2, player);
                SB(units_made, GB(units_made) + 1);
                SBO(players, 0x17 * player + 2, GBO(players, 0x17 * player + 2) + 2);
            }
        } else if (cls & 2) {
            make_unit(GB(units_made), type, off, 2);
            SB(units_made, GB(units_made) + 1);
        } else {
            make_unit(GB(units_made), type, off, player);
            SB(units_made, GB(units_made) + 1);
            SBO(players, 0x17 * player + 2, GBO(players, 0x17 * player + 2) + 1);
        }
    }
    for (i = 0; i <= 0xF0; i++)
        if (!(pw(UNIT(i), 4) & 0x8000))
            spb(GFP(map0), pw(UNIT(i), 0x0B), i);
    t0e9b_19ab(GFP(map0), GFP(map1));
    return 0;
}

/* a dot of the overview: four pixels in three colours */
static void overview_dot(int x, int y, int colour)
{
    SW(draw_colour, colour), SW(draw_x1, x), SW(draw_y1, y);
    put_pixel();
    SW(draw_colour, colour + 1), SW(draw_x1, x + 1), SW(draw_y1, y);
    put_pixel();
    SW(draw_colour, colour + 2), SW(draw_x1, x + 1), SW(draw_y1, y + 1);
    put_pixel();
    SW(draw_colour, colour + 1), SW(draw_x1, x), SW(draw_y1, y + 1);
    put_pixel();
}

/* MOON.EXE's T039B:0007: the map's picture drawn square by square from
 * the overview file (`od`: a table of 24 entries, 12 bytes each, the
 * entry's offset in its last 4) and MAPINFO.DAT (`info`: 4 bytes for each
 * ground: the entry of its square, the kind of what stands on it (an index
 * of the table below), the colour index and a mask of up to 6 more entries
 * drawn over it).  Columns col0.. and rows row0.. of the map, w + 1 by h +
 * 1 of them, at x, y, (1 << sc) pixels a square, the odd columns sc lower.
 * The ground's entry is the table's entry of its first byte, the kinds'
 * entries are those of kind_entry, the overlays are entries 0Eh.. and
 * have colour base 70h, plus 7 for colour index 1 (the original reads a
 * word of its stack for index 2: not checked what MAPINFO.DAT holds,
 * presumably only 0 and 1). */
static void moon_overview_map(fptr map, fptr od, fptr info, unsigned col0, unsigned row0,
                              unsigned w, unsigned h, int x, int y, int sc)
{
    static const uint8_t kind_entry[9] = { 3, 8, 9, 0, 1, 2, 4, 5, 6 };
    uint16_t mw = GW(map_width), mh = GW(map_height);
    fptr entry[24], tbl;
    unsigned r, c, i;
    uint16_t keep_x1 = GW(clip_x1), keep_y1 = GW(clip_y1), keep_x2 = GW(clip_x2), keep_y2 = GW(clip_y2);

    SW(clip_x1, x);
    SW(clip_y1, y + 2);
    SW(clip_x2, x + (w << sc) + (1 << sc));
    SW(clip_y2, y + (h << sc) + (1 << sc));
    tbl = hadd(od, (long)((uint32_t)pw(od, 0) | (uint32_t)pw(od, 2) << 16));
    for (i = 0; i < 24; i++) {
        fptr p = hadd(tbl, 12 * (long)i + 8);

        entry[i] = hadd(od, (long)((uint32_t)pw(p, 0) | (uint32_t)pw(p, 2) << 16));
    }
    for (r = 0; r <= h; r++) {
        for (c = 0; c <= w; c++) {
            uint16_t idx, sx, sy;
            unsigned g, k, count;

            if ((uint16_t)(r + row0) >= (uint16_t)(mh - 1) || (uint16_t)(c + col0) >= (uint16_t)(mw - 1))
                continue;
            idx = (uint16_t)((uint16_t)(r + row0) * mw + c + col0);
            sx = (uint16_t)((c << sc) + x);
            sy = (uint16_t)((r << sc) + y);
            if (c & 1)
                sy = (uint16_t)(sy + sc);
            g = pb(hadd(map, 2 * (long)idx), 0);
            draw_entry((int16_t)sx, (int16_t)sy, entry[pb(info, 4 * g) % 24], 0, 0, 0x70);
            count = pb(info, 4 * g + 2);
            if (!count)
                continue;
            for (k = 0; k < 6; k++)
                if (pb(info, 4 * g + 3) & (1u << k))
                    draw_entry((int16_t)sx, (int16_t)sy, entry[0x0E + k], 0, 0,
                               (count == 1 ? 7 : 0) + 0x70);
        }
    }
    for (r = 0; r <= h; r++) {
        for (c = 0; c <= w; c++) {
            uint16_t idx;
            int ox = -4, oy = 0, px, py;
            unsigned g, kind;

            if ((uint16_t)(r + row0) >= (uint16_t)(mh - 1) || (uint16_t)(c + col0) >= (uint16_t)(mw - 1))
                continue;
            idx = (uint16_t)((uint16_t)(r + row0) * mw + c + col0);
            g = pb(hadd(map, 2 * (long)idx), 0);
            kind = pb(info, 4 * g + 1);
            if (!kind)
                continue;
            if (kind >= 6 && kind <= 8)
                ox = -8, oy = -2;
            px = (int)((c << sc) + x) + (ox >> (2 - sc));
            py = (int)((r << sc) + y) + (oy >> (2 - sc));
            if (c & 1)
                py += sc;
            draw_entry(px, py, entry[kind_entry[kind % 9]], 0, 0, 0x70);
        }
    }
    SW(clip_x1, keep_x1), SW(clip_y1, keep_y1), SW(clip_x2, keep_x2), SW(clip_y2, keep_y2);
}

/* MOON.EXE's T0F3E:094A: the overview at x, y, as the above, a frame
 * round it (two rows, two columns, in two colours) and a dot for each
 * unit as `player` sees it */
static void moon_draw_overview(int player, int x, int y)
{
    int sc = (int16_t)GW(overview_scale), i, w = (int16_t)GW(map_width);
    int right = (int16_t)(x + (((int16_t)GW(map_width) - 1) << sc) - 1);
    int bottom = (int16_t)(y + (((int16_t)GW(map_height) - 1) << sc) - 2);

    moon_overview_map(map_of(player), GFP(overview_data), GFP(mapinfo_data), 0, 0,
                      (uint16_t)(GW(map_width) - 2), (uint16_t)(GW(map_height) - 2), x - 1, y - 2, sc);
    SW(draw_colour, GBO(amok, 0x0D)), SW(draw_x1, x - 1), SW(draw_y1, y - 1), SW(draw_x2, right);
    draw_row();
    SW(draw_x1, x - 1), SW(draw_y1, y - 1), SW(draw_y2, bottom);
    draw_column();
    SW(draw_colour, GBO(amok, 0x0C)), SW(draw_x1, x - 1), SW(draw_y1, bottom), SW(draw_x2, right);
    draw_row();
    SW(draw_x1, right), SW(draw_y1, y - 1), SW(draw_y2, bottom);
    draw_column();
    for (i = 0; i <= 0xF0; i++) {
        fptr u = UNIT(i);
        int side, sq, colour, py;

        if (pw(u, 4) & (0x8000 | 0x4000 | 2))
            continue;
        side = pw(u, 4) & 1;
        if (pw(u, 6) & 2)
            continue;
        sq = (int16_t)(pw(u, 0x0B + 2 * player) - 1) >> 1;
        if (same_side(side, player) && (pw(u, 4) & 0x200))
            colour = GBO(amok, 0x20 + side);
        else
            colour = GBO(amok, 0x22 + side);
        py = y + ((sq / w) << sc) + ((sq & 1) ? 2 : 0);
        if (bottom - 1 > py)
            overview_dot(x + ((sq % w) << sc), py, colour);
    }
}

/* the map's overview (the .PMP's picture, 2 pixels a square and 2
 * around) at x, y, and a dot for each unit as `player` sees it */
void draw_overview(int player, int x, int y, fptr pmp)
{
    int i, w = (int16_t)GW(map_width);

    if (bi_prog == BI_MOON) {
        moon_draw_overview(player, x, y);
        return;
    }
    draw_entry(x, y, MKFP(FSEG(pmp), FOFF(pmp) + 4), 0, 0, 0x70);
    for (i = 0; i <= 0xF0; i++) {
        fptr u = UNIT(i);
        int side, sq, colour;

        if (pw(u, 4) & (0x8000 | 0x4000 | 2))
            continue;
        side = pw(u, 4) & 1;
        if (pw(u, 6) & 2)
            continue;
        sq = (int16_t)(pw(u, 0x0B + 2 * player) - 1) >> 1;
        if (same_side(side, player) && (pw(u, 4) & 0x200))
            colour = GBO(amok, 0x20 + side);
        else
            colour = GBO(amok, 0x22 + side);
        overview_dot(x + ((sq % w) << 1) + 2, y + ((sq / w) << 1) + 2, colour);
    }
}

/* the unit of a square as `player` sees it: nothing for no unit and for
 * a hidden one of the other side */
static void draw_square_unit(int x, int y, unsigned b, int player)
{
    fptr u;

    if (b > 0xF0)
        return;
    u = UNIT(b);
    if ((pw(u, 6) & 2) && !same_side(pw(u, 4), player))
        return;
    draw_unit24(x, y, (pw(u, 4) & 1) ? 0x30 : 0x20,
                pfp(pfp(FP(lib_unit), 0x0E), 4 * ((pb(u, 0x0F + player) + pb(u, 8) * 6) & 0xFF)));
}

/* a square's marks as `player` sees them: PATT.LIB's entry 1 for C0h, 2
 * for 30h, entry 0 in the player's colours for 0Fh */
static void draw_square_marks(int x, int y, unsigned index, int player)
{
    fptr patt = pfp(FP(lib_patt), 0x0E);
    unsigned mask = player ? 0xAA : 0x55, m = GBO(marks, index);

    if (!(mask & m))
        return;
    m &= mask;
    if (m & 0xC0)
        draw_unit24(x, y, 0, pfp(patt, 4));
    else if (m & 0x30)
        draw_unit24(x, y, 0, pfp(patt, 8));
    if (m & 0x0F)
        draw_unit24(x, y, player ? 0x30 : 0x20, pfp(patt, 0));
}

/* a player's window from the square at offset `first` on */
void draw_window(int first, int player)
{
    fptr map = map_of(player), parts = pfp(FP(lib_part), 0x0E);
    int x = player ? 0xA0 : 0, w2 = GW(map_width) << 1, col, row;

    for (col = 0; col < 9; col++, x += 0x10) {
        int y = (col & 1) ? 0x0C : 0;
        unsigned off = (uint16_t)(first + 2 * col);

        for (row = 0; row < 7; row++, y += 0x18, off = (uint16_t)(off + w2)) {
            draw_hexagon(x, y, pfp(parts, 4 * pb(map, off)));
            draw_square_unit(x, y, pb(map, off + 1), player);
        }
    }
}

void draw_marks(int first, int player)
{
    int x = player ? 0xA0 : 0, col, row;
    unsigned index = mark_index((int16_t)first >> 1);

    for (col = 0; col < 9; col++, x += 0x10) {
        int y = (col & 1) ? 0x0C : 0;
        unsigned i = index + (unsigned)col;

        for (row = 0; row < 7; row++, y += 0x18, i += 0x40)
            draw_square_marks(x, y, i & 0xFFFF, player);
    }
}

/* the square under a player's cursor again: ground, unit, marks */
void redraw_cursor_square(fptr cursor, int player)
{
    fptr map = map_of(player);
    int x = (int16_t)pw(cursor, 0x10), y = (int16_t)pw(cursor, 0x12);
    unsigned off = pw(cursor, 0);

    draw_hexagon(x, y, pfp(pfp(FP(lib_part), 0x0E), 4 * pb(map, off)));
    draw_square_unit(x, y, pb(map, off + 1), player);
    draw_square_marks(x, y, mark_index((int16_t)off >> 1), player);
}

/* the squares of a player's window that have marks of `bits` drawn
 * without them, and those marks cleared everywhere */
void undraw_marks(int first, int player, int bits)
{
    fptr map = map_of(player), parts = pfp(FP(lib_part), 0x0E);
    int x = player ? 0xA0 : 0, w2 = GW(map_width) << 1, col, row;
    unsigned index = mark_index((int16_t)first >> 1);

    for (col = 0; col < 9; col++, x += 0x10) {
        int y = (col & 1) ? 0x0C : 0;
        unsigned off = (uint16_t)(first + 2 * col), i = index + (unsigned)col;

        for (row = 0; row < 7; row++, y += 0x18, off = (uint16_t)(off + w2), i += 0x40) {
            if (!(GBO(marks, i & 0xFFFF) & bits & 0xFF))
                continue;
            draw_hexagon(x, y, pfp(parts, 4 * pb(map, off)));
            draw_square_unit(x, y, pb(map, off + 1), player);
        }
    }
    clear_marks(bits);
}

/* the square at offset `off` drawn again in a player's window when the
 * window shows the map (the cursor's states 0, 1, 7, 8) and has it */
void redraw_square(int player, int off)
{
    int w = (int16_t)GW(map_width);
    int first = (int16_t)GWO(cursors, 0x31 * player + 2) >> 1;
    int col = ((int16_t)off >> 1) % w, row = ((int16_t)off >> 1) / w;
    int fcol = first % w, frow = first / w, x, y;
    unsigned state = GBO(cursors, 0x31 * player + 0x17);
    fptr map = map_of(player);

    if (state != 0 && state != 1 && state != 8 && state != 7)
        return;
    if (col < fcol || fcol + 9 <= col || row < frow || frow + 7 <= row)
        return;
    y = (((col - fcol) & 1) ? 0x0C : 0) + (row - frow) * 0x18;
    x = ((col - fcol) << 4) + (player ? 0xA0 : 0);
    draw_hexagon(x, y, pfp(pfp(FP(lib_part), 0x0E), 4 * pb(map, (uint16_t)off)));
    draw_square_unit(x, y, pb(map, (uint16_t)off + 1), player);
    draw_square_marks(x, y, mark_index((int16_t)off >> 1), player);
}

/* which of the ten building records at `table` has the square among its
 * four (+0Eh); -1 none */
int find_building(int square, fptr table)
{
    int i, j;

    for (i = 0; i < 0x0A; i++)
        for (j = 0; j < 4; j++)
            if (pw(table, 0x1C * i + 0x0E + 2 * j) == (uint16_t)square)
                return i;
    return -1;
}

/* T0E9B:15DD: a player's window put around the square at offset `off`
 * (its first column and row even, the square 5 columns and 4 rows in
 * where the map allows) and the cursor on the square */
void t0e9b_15dd(int player, int off)
{
    fptr cur = CURSOR(player);
    int w = (int16_t)GW(map_width), sq = (int16_t)off >> 1;
    int col = sq % w, row = sq / w, c0 = col - 5, r0 = row - 4, edge;

    if (c0 & 1)
        c0--;
    if (r0 & 1)
        r0--;
    if (c0 < 0)
        c0 = 0;
    if (r0 < 0)
        r0 = 0;
    edge = (GB(map_width) - 10) & 0xFF;
    if (edge < c0)
        c0 = edge;
    edge = (GB(map_height) - 8) & 0xFF;
    if (edge < r0)
        r0 = edge;
    col -= c0;
    row -= r0;
    if (col < 0)
        col = 0;
    if (row < 0)
        row = 0;
    spw(cur, 2, (r0 * w << 1) + (c0 << 1));
    spw(cur, 0x0C, col);
    spw(cur, 0x0E, row);
    spb(cur, 0x15, r0 >> 1);
    spb(cur, 0x14, c0 >> 1);
    t0d36_0044(cur, player);
}

/* T0E9B:1704: the ten squares around the one at `off` (by the tables for
 * even and odd columns) into `around`, and of those the ones a building
 * can be put on: the square itself is the building site's ground
 * (AMOK's +18h) or its four squares are free ground of flag 8000h; the
 * others get FFFFh.  How many there are. */
int t0e9b_1704(int off, fptr map)
{
    int w = (int16_t)GW(map_width), sq = (int16_t)off >> 1, col = sq % w, row = sq / w;
    int i, k, total = 0;

    for (i = 0; i < 0x0A; i++) {
        int c = col + (int8_t)((col & 1) ? GBO(around10_odd, i) : GBO(around10_even, i));
        int r = row + (int8_t)((col & 1) ? GBO(around10_odd, 0x0A + i) : GBO(around10_even, 0x0A + i));
        int count = 0;
        unsigned at = (uint16_t)((c << 1) + (w << 1) * r);

        SWO(around, 2 * i, at);
        if (pb(map, at) == GBO(amok, 0x18))
            count = 4;
        else
            for (k = 0; k < 4; k++) {
                int cc = c + (int8_t)((c & 1) ? GBO(around4_odd, k) : GBO(around4_even, k));
                int rr = r + (int8_t)((c & 1) ? GBO(around4_odd, 4 + k) : GBO(around4_even, 4 + k));
                unsigned o;

                if (cc < 0 || cc >= w || rr < 0 || (int16_t)GW(map_height) <= rr)
                    continue;
                o = (uint16_t)((cc << 1) + (w << 1) * rr);
                if ((pw(GROUND(pb(map, o)), 0) & 0x8000) && pb(map, o + 1) > 0xF0)
                    count++;
            }
        if (count != 4)
            SWO(around, 2 * i, 0xFFFF);
        else
            total++;
    }
    return total;
}

/* the four squares of a building at `off` into `around` */
void four_squares(int off)
{
    int w = (int16_t)GW(map_width), sq = (int16_t)off >> 1, col = sq % w, row = sq / w, i;

    for (i = 0; i < 4; i++) {
        int c = col + (int8_t)((col & 1) ? GBO(around4_odd, i) : GBO(around4_even, i));
        int r = row + (int8_t)((col & 1) ? GBO(around4_odd, 4 + i) : GBO(around4_even, 4 + i));

        SWO(around, 2 * i, (c << 1) + (w << 1) * r);
    }
}

/* T0E9B:19AB: a map copied */
void t0e9b_19ab(fptr from, fptr to)
{
    int i;

    for (i = 0; (int16_t)GW(map_bytes) > i; i++)
        spb(to, (unsigned)i, pb(from, (unsigned)i));
}

/* a unit's flag (100h or 200h of its +4) and its square's mark in a
 * player's view set or cleared, for both halves of a unit of two
 * squares, and the squares drawn again */
void unit_flag(int unit, int flags, int player, int set)
{
    fptr u = UNIT(unit & 0xFF);
    unsigned mark = 0;
    int half;

    if (flags & 0x100)
        mark = (player & 1) ? 0x80 : 0x40;
    else if (flags & 0x200)
        mark = (player & 1) ? 0x20 : 0x10;
    for (half = 0; half < 2; half++) {
        unsigned index = mark_index((int16_t)(pw(u, 0x0B + 2 * player) - 1) >> 1);

        if (set) {
            spw(u, 4, pw(u, 4) | (unsigned)flags);
            SBO(marks, index, GBO(marks, index) | mark);
        } else {
            spw(u, 4, pw(u, 4) & ~(unsigned)flags);
            SBO(marks, index, GBO(marks, index) & ~mark);
        }
        redraw_square(player, pw(u, 0x0B + 2 * player) - 1);
        if (half || !(pw(u, 4) & 0x40))
            break;
        u = (pw(u, 4) & 0x80) ? UNIT((unit & 0xFF) + 1) : UNIT((unit & 0xFF) - 1);
    }
}

/* T0E9B:1E0D: effect 1 on a channel, when the effects are on */
void t0e9b_1e0d(int channel)
{
    if (!(GW(game_flags) & 0x80))
        return;
    SWO(effects_asked, 6 * channel, 1);
    SWO(effects_asked, 6 * channel + 2, 0x6E);
    SWO(effects_asked, 6 * channel + 4, 1);
    effects_start();
}

/* T0E9B:1B9D: both windows drawn again square by square, a retrace
 * each, row after row to and fro (a and b the windows' first squares) */
void t0e9b_1b9d(int a, int b)
{
    int w2 = GW(map_width) << 1, row, i, forward = 1;

    for (row = 0; row < 7; row++) {
        int pa = a + w2 * row + (forward ? 0 : 0x10), pb2 = b + w2 * row + (forward ? 0 : 0x10);

        for (i = 0; i < 9; i++) {
            int n = forward ? i : 8 - i;

            wait_retrace();
            redraw_square(0, pa);
            redraw_square(1, pb2);
            if (n & 1)
                t0e9b_1e0d(3);
            pa += forward ? 2 : -2;
            pb2 += forward ? 2 : -2;
        }
        forward = !forward;
    }
}

/* T0E9B:1CB2: PATT.LIB's entry 3 over every square of both windows, a
 * retrace each, from the lowest row up, to and fro */
void t0e9b_1cb2(void)
{
    fptr e = pfp(pfp(FP(lib_patt), 0x0E), 0x0C);
    int y0 = 0x90, n, i, forward = 1;

    for (n = 6; n >= 0; n--, y0 -= 0x18) {
        for (i = 0; i < 9; i++) {
            int col = forward ? i : 8 - i, x = col << 4, y = (col & 1) ? y0 + 0x0C : y0;

            wait_retrace();
            draw_entry(x, y, e, 0, 0, 0);
            draw_entry(x + 0xA0, y, e, 0, 0, 0);
            if (col & 1)
                t0e9b_1e0d(3);
        }
        forward = !forward;
    }
}

/* how many squares apart two squares are on the map of hexagons (an odd
 * column lies half a row lower) */
int square_distance(int x1, int y1, int x2, int y2)
{
    int t, dy;

    if (y1 > y2) {
        t = x1, x1 = x2, x2 = t;
        t = y1, y1 = y2, y2 = t;
    }
    if (x2 >= x1) {
        y1 -= x1 / 2;
        y2 -= x2 / 2;
    } else {
        t = x1, x1 = x2, x2 = t;
        y1 = y1 - x1 / 2 - x1 % 2;
        y2 = y2 - x2 / 2 - x2 % 2;
    }
    dy = y2 - y1;
    if (dy < 0)
        dy = 0;
    return x2 - x1 + dy;
}
