/* shop.c - BATTLE.EXE's T1479: the screens over a player's window: the
 * status, a unit, a building with its slots and its list of types
 * (BATTLE.hints at draw_status and draw_building; tools/screens.py draws
 * them from a run's memory).  All in SHOP.LIB's window, the colours
 * AMOK.DAT's.
 */
#include <string.h>
#include "bi.h"

static fptr shop_entry(unsigned n) { return pfp(pfp(FP(lib_shop), 0x0E), 4 * n); }
static fptr unit_entry(unsigned n) { return pfp(pfp(FP(lib_unit), 0x0E), 4 * n); }

/* T2701:000E: a packed entry (TPWM, as a packed file) unpacked to `to`:
 * 0, or -1 when it is not packed.  As load_file: a byte more than the
 * length says. */
static int unpack_entry(fptr from, fptr to)
{
    uint32_t s = flin(from), d = flin(to), left;
    int bit;

    if (memcmp(mem + s, "TPWM", 4))
        return -1;
    left = pd(from, 4);
    s += 8;
    for (;;) {
        unsigned flags = mem[s++ & (MEM_SIZE - 1)];

        for (bit = 0; bit < 8; bit++, flags <<= 1) {
            if (!(flags & 0x80)) {
                mem[d++ & (MEM_SIZE - 1)] = mem[s++ & (MEM_SIZE - 1)];
                if (left-- == 0)
                    return 0;
            } else {
                unsigned b1 = mem[s++ & (MEM_SIZE - 1)], b2 = mem[s++ & (MEM_SIZE - 1)];
                unsigned count = (b1 & 0x0F) + 3;
                uint32_t back = d - ((b1 >> 4) << 8 | b2);

                while (count--) {
                    mem[d++ & (MEM_SIZE - 1)] = mem[back++ & (MEM_SIZE - 1)];
                    if (left-- == 0)
                        return 0;
                }
            }
        }
    }
}

/* an entry that may be packed (MOON's are): unpacked to `work` first */
void draw_packed(int x, int y, fptr entry, unsigned keep_off, unsigned keep_seg, int base, fptr work)
{
    if (unpack_entry(entry, work) == 0)
        draw_entry(x, y, work, keep_off, keep_seg, base);
    else
        draw_entry(x, y, entry, keep_off, keep_seg, base);
}

/* the window: SHOP.LIB's entries 0, 3, 1, 2 and a filled rectangle */
void draw_shop_window(int side)
{
    int x0 = side ? 0xA0 : 0;

    draw_entry(x0, 0, shop_entry(0), 0, 0, 0x40);
    draw_entry(x0, 0, shop_entry(3), 0, 0, 0x40);
    draw_entry(x0 + 0x8E, 0, shop_entry(1), 0, 0, 0x40);
    draw_entry(x0, 0xA6, shop_entry(2), 0, 0, 0x40);
    SW(draw_x1, x0 + 8), SW(draw_y1, 0x0C), SW(draw_x2, x0 + 0x8E), SW(draw_y2, 0xA6);
    SW(draw_colour, GBO(amok, 0x0E));
    fill_rect();
}

/* a box: filled (not for a colour below 0), a light row above and column
 * left, a dark column right and row below */
void draw_box(int x, int y, int w, int h, int side, int colour)
{
    if (side)
        x += 0xA0;
    if (colour >= 0) {
        SW(draw_x1, x), SW(draw_y1, y), SW(draw_x2, x + w - 1), SW(draw_y2, y + h - 1);
        SW(draw_colour, colour);
        fill_rect();
    }
    x--, y--, w++, h++;
    SW(draw_x1, x), SW(draw_y1, y), SW(draw_x2, x + w);
    SW(draw_colour, GBO(amok, 0x11));
    draw_row();
    SW(draw_x1, x + w), SW(draw_y1, y + 1), SW(draw_y2, y + h);
    SW(draw_colour, GBO(amok, 0x12));
    draw_column();
    SW(draw_x1, x + w), SW(draw_y1, y + h), SW(draw_x2, x + 1);
    SW(draw_colour, GBO(amok, 0x12));
    draw_row();
    SW(draw_x1, x), SW(draw_y1, y + h), SW(draw_y2, y);
    SW(draw_colour, GBO(amok, 0x11));
    draw_column();
}

/* T1479:028E: the numbers' box cleared */
void t1479_028e(int x, int y)
{
    SW(draw_x1, x), SW(draw_y1, y), SW(draw_x2, x + 0x45), SW(draw_y2, y + 0x27);
    SW(draw_colour, GBO(amok, 0x10));
    fill_rect();
}

/* a unit's numbers in the box at x, y: for a unit of the viewer's the
 * type's hit values, the unit's points of movement halved, the armour,
 * the ranges less 1, and its line below the window; text 1 for another's,
 * text 0Eh for one with 4 in its +6 */
void draw_unit_numbers(int x, int y, int unit, int side)
{
    fptr u = UNIT(unit & 0xFF), t;
    int mode, col;

    if (side)
        x += 0xA0;
    t1479_028e(x, y);
    if (side & 1)
        mode = (pw(u, 4) & 1) ? 2 : 1;
    else
        mode = (pw(u, 4) & 1) ? 1 : 2;
    if (pw(u, 6) & 4)
        mode = 0;
    if (mode == 1) {
        draw_text(x, y, 1, GBO(amok, 0x0D));
        return;
    }
    if (mode == 0) {
        draw_text(x, y, 0x0E, GBO(amok, 0x0D));
        return;
    }
    t = TYPE(pb(u, 8));
    x += 6;
    y += 2;
    draw_entry(x, y + 1, shop_entry(6), 0, 0, 0);
    draw_text(x + 0x2A, y, 0, GBO(amok, 0x0D));
    SW(draw_colour, GBO(amok, 0x0D));
    col = x + 0x18;
    draw_number(pb(t, 0x0A), col, y);
    draw_number(pb(t, 9), col, y + 6);
    draw_number(pb(t, 0x0B), col, y + 0x0C);
    draw_number(pb(u, 0) >> 1, col, y + 0x15);
    draw_number(pb(t, 1), col, y + 0x1B);
    col = x + 0x30;
    draw_number((pw(t, 5) & 4) ? pb(t, 8) - 1 : 0, col, y);
    draw_number((pw(t, 5) & 8) ? pb(t, 8) - 1 : 0, col, y + 0x0C);
    draw_number(pb(t, 7) - 1, col, y + 6);
    unit_line(unit, side);
}

/* T1479:0C93: a type's big picture (BIGUNIT.LIB) at x, y in the colours
 * of `owner`, and its name centred below */
void t1479_0c93(int x, int y, int type, int side, int owner)
{
    fptr t = TYPE(type), name = MKFP(FSEG(t), FOFF(t) + 0x1A);

    int dx = 0, dy = pb(t, 0x18);

    if (side)
        x += 0xA0;
    if (bi_prog == BI_MOON) {
        /* MOON (T154F:0C9E): the type's offsets from two tables of its own */
        dx = (int8_t)GBO(bigunit_dx, type);
        dy = (int8_t)GBO(bigunit_dy, type);
    }
    draw_packed(x + dx, y + dy, pfp(pfp(FP(lib_bigunit), 0x0E), 4 * pb(t, 0x19)), 0, 0, owner ? 0x30 : 0x20,
                pfp(MKFP(S_players, A_players + 0x17 * owner), 0x0D));
    SW(draw_colour, GBO(amok, 0x0D));
    draw_chars(x + 0x30 - 3 * (int)strlen(fstr(name)), y + 0x56, name);
}

/* the unit's screen: its big picture and name, its numbers, and the
 * ground's hexagon with the unit as it stands on the map (both halves of
 * a unit of two squares) */
void draw_unit_info(int side, int square, int unit, fptr map)
{
    fptr u = UNIT(unit & 0xFF), frame = shop_entry(5), part;
    int x0 = side ? 0xA0 : 0, x = x0 + 0x0F, base;
    unsigned f, e;

    draw_shop_window(side);
    draw_box(0x1E, 0x14, 0x60, 0x60, side, GBO(amok, 0x10));
    draw_box(0x2B, 0x7C, 0x46, 0x28, side, GBO(amok, 0x10));
    draw_unit_numbers(0x2B, 0x7C, unit, side);
    f = pw(u, 4);
    t1479_0c93(0x1E, 0x14, pb(u, 8), side, (f & 1) ? 1 : (f & 2) ? side : 0);
    base = (f & 1) ? 0x30 : 0x20;
    draw_entry(x0 + 0x74, 0x86, frame, 0, 0, 0);
    part = pfp(pfp(FP(lib_part), 0x0E), 4 * pb(map, (uint16_t)square));
    if (f & 0x40) {
        draw_entry(x, 0x79, frame, 0, 0, 0);
        draw_entry(x, 0x91, frame, 0, 0, 0);
        draw_hexagon(x + 1, 0x7A, part);
        draw_hexagon(x + 1, 0x92, part);
        if (!(f & 0x80))
            u = UNIT((unit - 1) & 0xFF);
        e = pb(u, 8) * 6u;
        draw_entry(x + 1, 0x7A, unit_entry(e), 0, 0, base);
        draw_entry(x + 1, 0x92, unit_entry(e + 6), 0, 0, base);
    } else {
        draw_entry(x, 0x86, frame, 0, 0, 0);
        draw_hexagon(x + 1, 0x87, part);
        draw_entry(x + 1, 0x87, unit_entry(pb(u, 0x0F + side) + pb(u, 8) * 6u), 0, 0, base);
    }
}

/* the seven slots of a building as `side` sees them, from x, y down: the
 * slot (SHOP.LIB's entry 4), the unit in it, and PATT.LIB's entry 2 over
 * a unit with bit 2 of its +4, or with bit 200h when same_side says so */
void draw_slots(int x, int y, int side, fptr rec)
{
    int i;

    if (side)
        x += 0xA0;
    for (i = 0; i < 7; i++, y += 0x18) {
        unsigned s, f;

        draw_entry(x, y, shop_entry(4), 0, 0, 0x40);
        s = pb(rec, (unsigned)(7 * side + i));
        if (s > 0xF0)
            continue;
        f = pw(UNIT(s), 4);
        draw_entry(x, y, unit_entry(pb(UNIT(s), 8) * 6u + 1), 0, 0,
                   (f & 1) ? 0x30 : (f & 2) ? (side ? 0x30 : 0x20) : 0x20);
        if (((f & 0x200) && same_side(side, (int)f)) || (f & 2))
            draw_entry(x, y, pfp(pfp(FP(lib_patt), 0x0E), 8), 0, 0, 0);
    }
}

/* a building's screen as it opens: the window, the two boxes, the slots,
 * its title (HQ, FACTORY, DEPOT, or text 6 for a unit that holds others) */
void draw_building(int side, fptr rec)
{
    unsigned f = pw(rec, 0x19);

    draw_shop_window(side);
    draw_box(0x2C, 0x16, 0x60, 0x60, side, GBO(amok, 0x10));
    draw_box(0x38, 0x7D, 0x48, 0x28, side, GBO(amok, 0x10));
    draw_slots(0x10, 0x0C, side, rec);
    draw_text((side ? 0xA0 : 0) + 0x2C, 0x0D, (f & 4) ? 3 : (f & 8) ? 5 : (f & 0x10) ? 4 : 6, GBO(amok, 0x0B));
}

/* seven of the types a factory can make, from the `first` of the list,
 * in the slots' places */
void draw_type_list(int x, int y, int side, int first)
{
    int i;

    if (side)
        x += 0xA0;
    for (i = 0; i < 7; i++, y += 0x18) {
        unsigned t;

        draw_entry(x, y, shop_entry(4), 0, 0, 0x40);
        t = GBO(types_list, (uint16_t)(first + i));
        if (t != 0xFF)
            draw_entry(x, y, unit_entry(t * 6 + 1), 0, 0, (GB(loop_player) & 1) ? 0x30 : 0x20);
    }
}

/* the status screen: round, level, the scores, the mode; the two
 * players' and the map's units, factories and depots; the turns used and
 * their limit */
void draw_status(int side)
{
    int x0 = side ? 0xA0 : 0, x, y, p;
    fptr pl = MKFP(S_players, A_players + 0x17 * side);

    draw_shop_window(side);
    x = x0 + 0x1D;
    y = 0x12;
    draw_box(x, y, 0x60, 0x2B, 0, GBO(amok, 0x10));
    draw_text(x, y, 0x0A, GBO(amok, 0x0D));
    draw_number((int16_t)GW(round), x + 0x36, y + 6);
    draw_number((int16_t)GW(map_number), x + 0x36, y + 0x0C);
    draw_number((int16_t)GWO(score_best, 0), x + 0x36, y + 0x18);
    draw_number((int16_t)GWO(score_now, 0), x + 0x36, y + 0x1E);
    draw_text(x + 0x36, y + 0x12, (GBO(cursors, 0x31 * side + 0x16) & 1) ? 0x0D : 0x0C, GBO(amok, 0x0D));
    x = x0 + 0x0C;
    y = 0x48;
    draw_box(x, y, 0x80, 0x38, 0, GBO(amok, 0x10));
    draw_text(x, y, 0x0B, GBO(amok, 0x0D));
    for (p = 0; p < 2; p++) {
        SW(draw_colour, GBO(amok, 9 + p));
        x = x0 + 0x42 + 0x18 * p;
        draw_number(GBO(players, 0x17 * p + 2), x, 0x5A);
        draw_number(GBO(players, 0x17 * p + 4), x, 0x72);
        draw_number(GBO(players, 0x17 * p + 3), x, 0x66);
    }
    SW(draw_colour, GBO(amok, 0x0D));
    x = x0 + 0x72;
    draw_number(GB(units_made), x, 0x5A);
    draw_number(GB(depots_made), x, 0x72);
    draw_number(GB(factories_made), x, 0x66);
    x = x0 + 0x0C;
    y = 0x8A;
    draw_box(x, y, 0x64, 0x18, 0, GBO(amok, 0x10));
    draw_text(x, y, 0x0F, GBO(amok, 0x0D));
    draw_number(pb(pl, 0x15), x + 0x30, y + 6);
    if (pb(pl, 0x16) < 0xFF)
        draw_number(pb(pl, 0x16), x + 0x30, y + 0x0C);
    else
        draw_text(x + 0x30, y + 0x0C, 0x10, GBO(amok, 0x0D));
    draw_entry(x0 + 0x74, 0x8A, shop_entry(5), 0, 0, 0);
}

/* the types a factory with `energy` can make into types_list (FFh ends
 * it), their number in types_listed: those that cost no more, are
 * allowed on the map (the .SHP's bits) and, for a type that holds
 * others, have a free record; none while no unit's record is free */
void list_makeable(int energy)
{
    unsigned t;

    for (t = 0; t < 0x1B; t++)
        SBO(types_list, t, 0xFF);
    SB(types_listed, 0);
    if (t169e_01a9(0x8000) == 0xFF)
        return;
    for (t = 0; t < 0x1B; t++) {
        int ok = pb(TYPE(t), 0x3E) <= (unsigned)(energy & 0xFF) && !(pw(TYPE(t), 0x0E) & 2);

        if ((pw(TYPE(t), 0x0C) & 0x1000) && t169e_01f5(0x8000) < 0)
            ok = 0;
        if (!ok)
            continue;
        SBO(types_list, GB(types_listed), t);
        SB(types_listed, GB(types_listed) + 1);
    }
}

/* the sizes (the types' +40h) of the units in a record's slots of `side` */
int cargo_size(fptr rec, int side)
{
    int sum = 0, i;

    for (i = 0; i < 7; i++) {
        unsigned s = pb(rec, (unsigned)(7 * side + i));

        if (s <= 0xF0)
            sum += pb(TYPE(pb(UNIT(s), 8)), 0x40);
    }
    return sum;
}
