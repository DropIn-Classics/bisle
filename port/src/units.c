/* units.c - BATTLE.EXE's T169E: the records of units and buildings made
 * and given up.
 *
 * A unit's record (1Ah bytes): +0 its points of movement, +1 experience,
 * +2 its count (+3 a ship's second count), +4 a word: the type's class
 * with 1 player 1's, 2 nobody's, 8000h no unit, 4000h inside a building,
 * 100h and 200h chosen or with an order; +6 the type's +10h, +8 the type,
 * +9 its serial, +0Ah the record of what it holds (FFh none), +0Bh and
 * +0Dh its square in each player's map (the offset plus 1), +0Fh, +10h
 * its direction in each, +11h.. its order.
 * A building's or holder's record (1Ch bytes): +0 seven slots as player
 * 0 sees them and +7 seven as player 1 does (a unit, FFh none), +0Eh its
 * four squares, +16h, +17h the energy for each player, +18h, +19h a word:
 * the owner's bits and 4 a headquarters, 8 a factory, 10h a depot, 20h a
 * unit, 8000h not there; +1Bh the unit that it is.
 */
#include "bi.h"

/* the unit `number` made of `type` for `player` (2: nobody) on the map's
 * byte at `square` */
void make_unit(int number, int type, int square, int player)
{
    fptr u = UNIT(number & 0xFF), t = TYPE(type & 0xFF);
    int holder;

    spb(u, 8, type);
    spb(u, 0, pb(t, 0));
    spb(u, 2, pb(t, 2));
    spb(u, 3, pb(t, 3));
    spw(u, 6, pw(t, 0x10));
    spw(u, 4, pw(t, 0x0C) | (unsigned)player);
    if (!(player & 2)) {
        spb(u, 9, pb(t, 0x3C + player));
        spb(t, 0x3C + player, pb(t, 0x3C + player) + 2);
    }
    spb(u, 0x13, 0), spb(u, 0x16, 0), spb(u, 0x19, 0), spb(u, 0x15, 0), spb(u, 0x14, 0), spb(u, 1, 0);
    spw(u, 0x17, 0);
    spw(u, 0x11, 0);
    spb(u, 0x10, player ? 0 : 3);
    spb(u, 0x0F, player ? 0 : 3);
    spw(u, 0x0D, square);
    spw(u, 0x0B, square);
    if (pw(t, 0x0C) & 0x1000) {
        holder = t169e_01f5(0x8000);
        spb(u, 0x0A, holder);
        t169e_04a0((int8_t)pb(u, 0x0A), square, type & 0xFF, player, number & 0xFF);
        SB(cargo_made, GB(cargo_made) + 1);
    } else if (pw(t, 0x10) & 0x4000)
        spb(u, 0x0A, 2);
    else
        spb(u, 0x0A, 0xFF);
}

/* T169E:01A9: the first unit with `mask` in its +4 (8000h: a free
 * record); FFh none */
int t169e_01a9(int mask)
{
    int i;

    for (i = 0; i <= 0xF0; i++)
        if (pw(UNIT(i), 4) & mask)
            return i;
    return 0xFF;
}

/* T169E:01F5: the same for the holders' records; -1 none */
int t169e_01f5(int mask)
{
    int i;

    for (i = 0; i < 0x46; i++)
        if (GWO(cargo, 0x1C * i + 0x19) & mask)
            return i;
    return -1;
}

static void empty_slots(fptr rec)
{
    unsigned i;

    for (i = 0; i < 7; i++) {
        spb(rec, i + 7, 0xFF);
        spb(rec, i, 0xFF);
    }
}

/* T169E:0244: factory `n` at the square `sq`, its four squares, an
 * energy of 30 */
void t169e_0244(int n, int sq, int owner)
{
    fptr rec = MKFP(S_factories, A_factories + 0x1C * n);
    int w2 = GW(map_width) << 1, odd = ((int16_t)sq >> 1) & 1;

    spw(rec, 0x0E, sq);
    spw(rec, 0x10, sq - 4);
    spw(rec, 0x12, odd ? sq - 2 : sq - 2 - w2);
    spw(rec, 0x14, odd ? sq + w2 - 2 : sq - 2);
    spw(rec, 0x19, owner | 8);
    spb(rec, 0x17, 0x1E);
    spb(rec, 0x16, 0x1E);
    spb(rec, 0x1B, 0xFF);
    spb(rec, 0x18, 0x0F);
    empty_slots(rec);
}

/* T169E:0324: depot `n`, an energy of 20 */
void t169e_0324(int n, int sq, int owner)
{
    fptr rec = MKFP(S_depots, A_depots + 0x1C * n);
    int w2 = GW(map_width) << 1, odd = ((int16_t)sq >> 1) & 1;

    spw(rec, 0x0E, sq);
    spw(rec, 0x10, odd ? sq + w2 - 2 : sq - 2);
    spw(rec, 0x12, sq + w2);
    spw(rec, 0x14, odd ? sq + w2 + 2 : sq + 2);
    spw(rec, 0x19, owner | 0x10);
    spb(rec, 0x17, 0x14);
    spb(rec, 0x16, 0x14);
    spb(rec, 0x1B, 0xFF);
    spb(rec, 0x18, 0x0A);
    empty_slots(rec);
}

/* T169E:0401: the headquarters of `owner`, one square, an energy of 50 */
void t169e_0401(int sq, int owner)
{
    fptr rec = MKFP(S_hqs, A_hqs + 0x1C * owner);

    spw(rec, 0x14, sq), spw(rec, 0x12, sq), spw(rec, 0x10, sq), spw(rec, 0x0E, sq);
    spb(rec, 0x17, 0x32);
    spb(rec, 0x16, 0x32);
    spw(rec, 0x19, owner | 4);
    spb(rec, 0x1B, 0xFF);
    empty_slots(rec);
    spb(rec, 0x18, 0x14);
}

/* T169E:04A0: the record `n` of what the unit `unit` holds: its room
 * (the type's +3Fh) where a building has its energy, the owner's bits
 * with 20h and the type's +12h */
void t169e_04a0(int n, int sq, int type, int player, int unit)
{
    fptr rec = MKFP(S_cargo, A_cargo + 0x1C * n);

    spw(rec, 0x14, sq), spw(rec, 0x12, sq), spw(rec, 0x10, sq), spw(rec, 0x0E, sq);
    spb(rec, 0x17, pb(TYPE(type), 0x3F));
    spb(rec, 0x16, pb(TYPE(type), 0x3F));
    spw(rec, 0x19, player | 0x20);
    spb(rec, 0x1B, unit);
    spw(rec, 0x19, pw(rec, 0x19) | pw(TYPE(pb(UNIT(unit & 0xFF), 8)), 0x12));
    empty_slots(rec);
    spb(rec, 0x18, 0);
}

/* T169E:06D7: a holder's record given up: the units in its slots of
 * `side` removed, the players' counts of buildings */
static void holder_remove(fptr rec, int side)
{
    unsigned i;

    for (i = 0; i < 7; i++) {
        unsigned at = (unsigned)(side * 7) + i;

        if (pb(rec, at) > 0xF0)
            continue;
        unit_remove(pb(rec, at), side);
        spb(rec, at, 0xFF);
    }
    if (pw(rec, 0x19) & 1)
        side = 1;
    else if (!(pw(rec, 0x19) & 2))
        side = 0;
    if (pw(rec, 0x19) & 0x10) {
        SBO(players, 0x17 * side + 4, GBO(players, 0x17 * side + 4) - 1);
        SB(depots_made, GB(depots_made) - 1);
    } else if (pw(rec, 0x19) & 8) {
        SBO(players, 0x17 * side + 3, GBO(players, 0x17 * side + 3) - 1);
        SB(factories_made, GB(factories_made) - 1);
    } else if (pw(rec, 0x19) & 0x20)
        SB(cargo_made, GB(cargo_made) - 1);
    spw(rec, 0x19, 0x8000);
}

/* a unit's record given up, with its other half and what it holds (as
 * `side` sees it), and the counts of units.  As the original: nested
 * more than five deep, or for a record that is free already, nothing is
 * done and the depth stays counted. */
void unit_remove(int unit, int side)
{
    fptr u = UNIT(unit & 0xFF);
    int n = 1;

    SB(remove_depth, GB(remove_depth) + 1);
    if ((int8_t)GB(remove_depth) > 5 || (pw(u, 4) & 0x8000))
        return;
    if (pw(u, 4) & 0x40) {
        int other;

        if (pw(u, 4) & 0x80)
            other = (unit + 1) & 0xFF;
        else {
            other = unit & 0xFF;
            unit--;
        }
        n++;
        spw(UNIT(other), 4, 0x8000);
    }
    u = UNIT(unit & 0xFF);
    if (pw(u, 4) & 0x1000)
        holder_remove(MKFP(S_cargo, A_cargo + 0x1C * (int8_t)pb(u, 0x0A)), side);
    SB(units_made, GB(units_made) - n);
    if (pw(u, 4) & 1)
        SBO(players, 0x17 + 2, GBO(players, 0x17 + 2) - n);
    else if (!(pw(u, 4) & 2))
        SBO(players, 2, GBO(players, 2) - n);
    spw(u, 4, 0x8000);
    SB(remove_depth, GB(remove_depth) - 1);
}

/* the units a unit holds get its square in the map of `side`, and its
 * holder's record too */
void cargo_follow(int unit, int side)
{
    fptr rec = MKFP(S_cargo, A_cargo + 0x1C * (int8_t)pb(UNIT(unit), 0x0A));
    unsigned sq = pw(UNIT(unit), 0x0B + 2 * side), i;

    for (i = 0; i < 7; i++) {
        unsigned s = pb(rec, (unsigned)(7 * side) + i);

        if (s <= 0xF0)
            spw(UNIT(s), 0x0B + 2 * side, sq);
    }
    spw(rec, 0x0E, sq);
}
