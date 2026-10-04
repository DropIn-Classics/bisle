/* phase.c - BATTLE.EXE's T0408: the change of phase.  The fights of the
 * attacking player's orders (each shown in the fight scene, fight_step),
 * the dead units removed, the ends of the moves (a unit or a building
 * taken, a road or a depot built), the buildings' slots and the mover's
 * units copied into the other view, the modes exchanged, the units'
 * flags cleared, the test for the map's end, the score.
 */
#include "bi.h"

#define PLAYER(n) MKFP(S_players, A_players + 0x17 * (n))
#define REC(table, n) MKFP(S_##table, A_##table + 0x1C * (n))
#define EXPLOSION(n) MKFP(S_explosions, A_explosions + 7 * (n))

static unsigned place(fptr u, int side) { return pw(u, 0x0B + 2 * side); }

static unsigned mark_at(unsigned off)
{
    int s = (int16_t)off >> 1, w = (int16_t)GW(map_width);

    return (uint16_t)(s % w + ((s / w) << 6));
}

static void swap_maps(void)
{
    fptr m = GFP(map0);

    SFP(map0, GFP(map1));
    SFP(map1, m);
}

/* every unit's two directions exchanged */
static void swap_directions(void)
{
    unsigned n;

    for (n = 0; n <= 0xF0; n++) {
        unsigned d = pb(UNIT(n), 0x0F);

        spb(UNIT(n), 0x0F, pb(UNIT(n), 0x10));
        spb(UNIT(n), 0x10, d);
    }
}

/* a unit's square, and its other half's, marked */
static void mark_unit(int n, int side, int m)
{
    fptr u = UNIT(n);
    unsigned at = mark_at((uint16_t)(place(u, side) - 1));

    SBO(marks, at, GBO(marks, at) | m);
    if (pw(u, 4) & 0x40) {
        u = UNIT((pw(u, 4) & 0x80) ? n + 1 : n - 1);
        at = mark_at((uint16_t)(place(u, side) - 1));
        SBO(marks, at, GBO(marks, at) | m);
    }
}

/* the building a unit's +17h and +15h name */
static fptr building_of(fptr u)
{
    if (pw(u, 0x17) & 4)
        return REC(hqs, pb(u, 0x15));
    if (pw(u, 0x17) & 8)
        return REC(factories, pb(u, 0x15));
    return REC(depots, pb(u, 0x15));
}

/* a unit to player `to`: its flags, its serial from the type's count */
static void unit_to(fptr v, int to)
{
    fptr t = TYPE(pb(v, 8));

    spw(v, 4, (pw(v, 4) & 0xFFFC) | to);
    spb(v, 9, pb(t, 0x3C + to));
    spb(t, 0x3C + to, pb(t, 0x3C + to) + 2);
}

/* the score: the armour of all units that count (not a second half, not
 * one with 4 in its +6), 100 more with HIDE SHOP, times 4, 3 or 2 for a
 * limit of 4, 8 or 16 turns, at most 7EF4h */
long score(void)
{
    long sum = 0;
    unsigned i, limit = GBO(players, 0x16);

    for (i = 0; i <= 0xF0; i++) {
        unsigned f = pw(UNIT(i), 4);

        if ((f & 0x8000) || ((f & 0x40) && !(f & 0x80)) || (pw(UNIT(i), 6) & 4))
            continue;
        sum += pb(TYPE(pb(UNIT(i), 8)), 1);
    }
    if (GW(menu_flags) & 8)
        sum += 0x64;
    if (limit == 4)
        sum *= 4;
    else if (limit == 8)
        sum *= 3;
    else if (limit == 0x10)
        sum *= 2;
    if (sum > 0x7EF4)
        sum = 0x7EF4;
    return sum;
}

/* a unit without points is given up */
static void drop_empty(void)
{
    unsigned n;

    for (n = 0; n <= 0xF0; n++)
        if (!(pw(UNIT(n), 4) & 0x8000) && pb(UNIT(n), 2) == 0)
            spw(UNIT(n), 4, pw(UNIT(n), 4) | 0x8000);
}

/* a dead unit: its square (and its other half's) into the explosions'
 * records `slot` (and the next), what the map held there kept, the
 * unit's byte in the map made v1 (v2), the unit's +14h bit 4 */
static void unit_explode(int unit, int v1, int v2, int slot, int side, fptr map)
{
    unsigned sq;
    fptr x;

    unit &= 0xFF;
    sq = (uint16_t)(place(UNIT(unit), side) - 1);
    x = EXPLOSION(slot);
    spw(x, 0, sq);
    spb(x, 2, GBO(amok, 8));
    spw(x, 3, pb(map, sq));
    spb(map, sq + 1, v1);
    spw(x, 5, v1 & 0xFF);
    spb(UNIT(unit), 0x14, pb(UNIT(unit), 0x14) | 4);
    if (!(pw(UNIT(unit), 4) & 0x40))
        return;
    unit = ((pw(UNIT(unit), 4) & 0x80) ? unit + 1 : unit - 1) & 0xFF;
    sq = (uint16_t)(place(UNIT(unit), side) - 1);
    x = EXPLOSION(slot + 1);
    spw(x, 0, sq);
    spb(x, 2, GBO(amok, 8) + 1);
    spw(x, 3, pb(map, sq));
    spb(map, sq + 1, v2);
    spw(x, 5, v2 & 0xFF);
    spb(UNIT(unit), 0x14, pb(UNIT(unit), 0x14) | 4);
}

/* T0408:25C8: an explosion's picture (BUM.LIB's entry of its count) over
 * its square's ground, when the square is in the window that begins at
 * `first` */
static void t0408_25c8(int i, fptr map, int first, int side)
{
    int w = (int16_t)GW(map_width);
    unsigned sq = pw(EXPLOSION(i), 0);
    int col = ((int16_t)sq >> 1) % w, row = ((int16_t)sq >> 1) / w;
    int c0, r0, x, y;

    first = (int16_t)first >> 1;
    c0 = first % w;
    r0 = first / w;
    if (col < c0 || c0 + 9 <= col || row < r0 || r0 + 7 <= row)
        return;
    y = (((col - c0) & 1) ? 0x0C : 0) + (row - r0) * 0x18;
    x = (col - c0) << 4;
    if (side)
        x += 0xA0;
    draw_hexagon(x, y, pfp(pfp(FP(lib_part), 0x0E), 4 * pb(map, sq)));
    draw_entry(x, y, pfp(pfp(FP(lib_bum), 0x0E), (unsigned)(4 * (int8_t)pb(EXPLOSION(i), 2))), 0, 0, 0);
}

/* the slots and the energy of `count` records copied between the two
 * players' views: from b's to a's where same_side says so, else from
 * a's to b's; a slot of b's with a unit given up is emptied */
static void sync_slots(fptr table, int a, int b, int count)
{
    int n, j;

    for (n = 0; n < (count & 0xFF); n++) {
        fptr r = MKFP(FSEG(table), FOFF(table) + 0x1C * n);

        if (pw(r, 0x19) & 0x8002)
            continue;
        for (j = 0; j < 7; j++) {
            if (same_side(b, pw(r, 0x19))) {
                unsigned s = pb(r, (unsigned)(7 * b + j));

                if (pw(UNIT(s), 4) & 0x8000) {
                    s = 0xFF;
                    spb(r, (unsigned)(7 * b + j), s);
                }
                spb(r, (unsigned)(7 * a + j), s);
                spb(r, 0x16 + a, pb(r, 0x16 + b));
            } else {
                spb(r, (unsigned)(7 * b + j), pb(r, (unsigned)(7 * a + j)));
                spb(r, 0x16 + b, pb(r, 0x16 + a));
            }
        }
    }
}

/* what a holder lost in a fight its cargo loses too: a unit with no
 * more points than that dies */
static void cargo_loses(int side, int unit, int lost)
{
    fptr r = REC(cargo, (int8_t)pb(UNIT(unit & 0xFF), 0x0A));
    int j;

    lost &= 0xFF;
    for (j = 0; j < 7; j++) {
        unsigned s = pb(r, (unsigned)(7 * side + j));

        if (s > 0xF0)
            continue;
        if (pb(UNIT(s), 2) <= lost) {
            spb(UNIT(s), 0x14, pb(UNIT(s), 0x14) | 4);
            spb(r, (unsigned)(7 * side + j), 0xFF);
        } else
            spb(UNIT(s), 2, pb(UNIT(s), 2) - lost);
    }
}

static unsigned mod6(unsigned n)
{
    n &= 0xFF;
    while (n > 5)
        n = (n - 6) & 0xFF;
    return n;
}

/* the squares around unit d in `around` made 1 where a unit stands that
 * helps against it (its type's +5 meets d's flags, not bit 40h, on a's
 * side by same_side), else 0; the direction in which a stands */
static int flankers(int a, int d, fptr map, int side)
{
    unsigned bits = pw(UNIT(d), 4) & 0x3C, bits2 = pw(UNIT(d), 6) & 2, i;
    int result = 0;

    neighbours((uint16_t)(place(UNIT(d), side) - 1));
    for (i = 0; i < 6; i++) {
        unsigned sq = GWO(around, 2 * i), t = pb(map, sq + 1), f;

        SWO(around, 2 * i, 0);
        if (sq == 0xFFFF || t > 0xF0)
            continue;
        f = pw(TYPE(pb(UNIT(t), 8)), 5);
        if (((bits & f) || (bits2 & f)) && !(f & 0x40) && same_side(pw(UNIT(t), 4), pw(UNIT(a), 4)))
            SWO(around, 2 * i, 1);
        if (t == (unsigned)a)
            result = (int)i;
    }
    return result;
}

/* the fight's two bonuses into the fight's record (+24h the attacker's,
 * +25h the defender's), 0 for a fight over a distance: the attacker's by
 * the table F27EE:000D for each helper around the defender, counted from
 * where the attacker stands; the defender's 50h for a helper on each
 * side of where the defender stands, seen from the attacker */
static void fight_bonus(int a, int d, fptr map, int side)
{
    fptr fr = FP(fight_record);
    unsigned i, far_off = 0, b, dir;

    a &= 0xFF;
    d &= 0xFF;
    neighbours((uint16_t)(place(UNIT(d), side) - 1));
    for (i = 0; i < 6; i++) {
        unsigned sq = (uint16_t)(GWO(around, 2 * i) + 1);

        if (sq != 0xFFFF && pb(map, sq) != (unsigned)a)
            far_off++;
    }
    if (far_off == 6) {
        spb(fr, 0x24, 0);
        spb(fr, 0x25, 0);
        return;
    }
    b = 0;
    dir = (unsigned)flankers(a, d, map, side);
    for (i = 1; i < 6; i++)
        if (GWO(around, 2 * mod6(dir + i)))
            b = (b + GBO(flank_bonus, i - 1)) & 0xFF;
    spb(fr, 0x24, b);
    b = 0;
    dir = (unsigned)flankers(d, a, map, side);
    if (GWO(around, 2 * mod6(dir + 1)))
        b += 0x50;
    if (GWO(around, 2 * mod6(dir + 5)))
        b += 0x50;
    if (b > 0x96)
        b = 0x96;
    spb(fr, 0x25, b);
}

/* T0408:2D57: the sound of the attacker's (1) or the defender's type
 * (its +41h, when its +43h has bit 1 and the sound is on) on channel 0
 * or 2 by the unit's player */
static void t0408_2d57(int which)
{
    fptr fr = FP(fight_record), e = GFP(effects_ptr);
    fptr t = pfp(fr, which ? 0x0A : 0x0E), u = pfp(fr, which ? 0 : 4);
    unsigned ch = (pw(u, 4) & 1) ? 2 : 0, n = pb(t, 0x41);

    if (!(pw(fr, 0x2C) & 1) || !(pb(t, 0x43) & 1) || n == 0xFF)
        return;
    spw(e, 6 * ch, n);
    spw(e, 6 * ch + 2, 0);
    spw(e, 6 * ch + 4, 0xFFFF);
    effects_start();
}

/* a fight of the attacker's list: the scene, then the losses, the
 * experience, the dead units' explosions in the other player's window */
static void fight(int a, unsigned target, int side, int other, fptr map)
{
    fptr fr = FP(fight_record), ua, ud;
    int w = (int16_t)GW(map_width), d, i, done = 0, first = 1, boom = 0;
    unsigned sq, a_before, d_before, a_after, d_after, a_lost, d_lost, kills_a = 0, kills_d = 0, v;

    if ((pw(UNIT(a), 4) & 0x40) && !(pw(UNIT(a), 4) & 0x80))
        a = (a - 1) & 0xFF;
    sq = (uint16_t)(place(UNIT(a), side) - 1);
    d = pb(map, target + 1);
    if (d > 0xF0)
        return;
    if ((pw(UNIT(d), 4) & 0x40) && !(pw(UNIT(d), 4) & 0x80))
        d = (d - 1) & 0xFF;
    ua = UNIT(a);
    ud = UNIT(d);
    spw(fr, 0x22, 0);
    neighbours(sq);
    for (i = 0; i < 6; i++)
        if (GWO(around, 2 * i) == target)
            spw(fr, 0x22, 1);
    fight_bonus(a, d, map, side);
    spfp(fr, 0, ua);
    spfp(fr, 0x0A, TYPE(pb(ua, 8)));
    spfp(fr, 0x12, GROUND(pb(map, sq)));
    spb(fr, 8, a);
    a_before = pb(ua, (pw(pfp(fr, 0x0A), 0x0E) & 4) ? 3 : 2);
    spfp(fr, 4, ud);
    spfp(fr, 0x0E, TYPE(pb(ud, 8)));
    spfp(fr, 0x16, GROUND(pb(map, target)));
    spb(fr, 9, d);
    d_before = pb(ud, (pw(pfp(fr, 0x0E), 0x0E) & 4) ? 3 : 2);
    spw(fr, 0x1A, ((int16_t)sq >> 1) % w);
    spw(fr, 0x1C, ((int16_t)sq >> 1) / w);
    spw(fr, 0x1E, ((int16_t)target >> 1) % w);
    spw(fr, 0x20, ((int16_t)target >> 1) / w);
    spw(fr, 0x28, 0);
    spw(fr, 0x26, 0);
    spd(fr, 0x2E, GD(passes));
    spw(fr, 0x2C, (GW(game_flags) & 0x80) ? 1 : 0);
    sq = (uint16_t)(((int16_t)sq >> 1) << 1);
    /* the other player's window on the attacker, both units marked, in
     * the attacking player's map and directions */
    swap_maps();
    mark_unit(a, side, other ? 0x80 : 0x40);
    mark_unit(d, side, other ? 2 : 1);
    swap_directions();
    t0e9b_15dd(other, (int)sq);
    draw_window(GWO(cursors, 0x31 * other + 2), other);
    draw_marks(GWO(cursors, 0x31 * other + 2), other);
    swap_directions();
    swap_maps();
    clear_marks(0xFF);
    unit_line(d, other);
    unit_line(a, side);
    while (!done) {
        done = fight_step(fr) & 0xFF;
        if (first)
            first = 0;
        else
            SW(pass_ticks, 2);
        if (!done) {
            flip_page();
            copy_page();
        }
        while ((int16_t)GW(tick_count) < (int16_t)GW(pass_ticks))
            clock_idle();
        if (!done)
            restore_sprites();
    }
    a_after = pb(pfp(fr, 0), (pw(pfp(fr, 0x0A), 0x0E) & 4) ? 3 : 2);
    d_after = pb(pfp(fr, 4), (pw(pfp(fr, 0x0E), 0x0E) & 4) ? 3 : 2);
    if (a_after == 0) {
        unit_explode(pb(fr, 8), 0xFF, 0xFF, 0, side, map);
        boom = 1;
        kills_a++;
        t0408_2d57(1);
    }
    if (d_after == 0) {
        unit_explode(pb(fr, 9), 0xFF, 0xFF, 2, side, map);
        boom = 1;
        kills_d++;
        t0408_2d57(0);
    }
    a_lost = (a_before - a_after) & 0xFF;
    if (pw(pfp(fr, 0), 4) & 0x1000)
        cargo_loses(side, a, (int)a_lost);
    d_lost = (d_before - d_after) & 0xFF;
    if (pw(pfp(fr, 4), 4) & 0x1000)
        cargo_loses(side, d, (int)d_lost);
    if (d_lost > 0 && !(pw(pfp(fr, 4), 6) & 0x20))
        kills_d = (kills_d + 1) & 0xFF;
    if (a_lost > 0 && !(pw(pfp(fr, 0), 6) & 0x20))
        kills_a = (kills_a + 1) & 0xFF;
    /* the experience, at most 6 */
    v = pb(pfp(fr, 0), 1) + kills_d;
    spb(pfp(fr, 0), 1, v > 6 ? 6 : v);
    v = pb(pfp(fr, 4), 1) + kills_a;
    spb(pfp(fr, 4), 1, v > 6 ? 6 : v);
    SW(pass_ticks, 4);
    copy_page();
    unit_line(d, other);
    unit_line(a, side);
    flip_page();
    copy_page();
    restore_sprites();
    if (boom) {
        int busy = 1;

        t0d36_000f(0x14);
        if (GW(game_flags) & 0x80) {
            fptr e = GFP(effects_ptr);

            for (i = 0; i < 4; i++) {
                spw(e, 6 * (unsigned)i, 3);
                spw(e, 6 * (unsigned)i + 2, 0x7F);
                spw(e, 6 * (unsigned)i + 4, 1);
            }
            effects_start();
        }
        while (busy) {
            busy = 0;
            for (i = 0; i < 4; i++) {
                fptr x = EXPLOSION(i);
                unsigned kept;

                if ((int8_t)pb(x, 2) < 0)
                    continue;
                busy++;
                if (pb(x, 2) < GBO(amok, 8))
                    t0408_25c8(i, map, GWO(cursors, 0x31 * other + 2), other);
                spb(x, 2, pb(x, 2) - 1);
                if ((int8_t)pb(x, 2) >= 0)
                    continue;
                /* over: the square as it is now, in the other's window */
                kept = GBO(cursors, 0x31 * other + 0x17);
                SBO(cursors, 0x31 * other + 0x17, 0);
                spb(map, pw(x, 0), pb(x, 3));
                spb(map, pw(x, 0) + 1, pb(x, 5));
                swap_maps();
                redraw_square(other, pw(x, 0));
                swap_maps();
                SBO(cursors, 0x31 * other + 0x17, kept);
            }
            flip_page();
            copy_page();
            while ((int16_t)GW(tick_count) < (int16_t)GW(pass_ticks))
                wait_retrace();
        }
    }
    t0d36_000f(0x96);
    t0408_2d57(0);
    t0408_2d57(1);
}

/* a library of the fight scene behind the others at *p */
static int scene_lib(fptr record, fptr *p, fptr work, int sorted)
{
    if (load_lib(make_path(1, 0, record, 3), *p, work, record, sorted) == -1)
        return -1;
    *p = hadd(*p, (long)pd(record, 0x0A));
    return 0;
}

/* a building taken by the unit n that stands on it: the film in the
 * taker's half, the units inside and the building to the taker */
static void take_building(int n, int side, int other, fptr map, fptr buffer, int *first_anim, int *stopped, int *ret)
{
    fptr u = UNIT(n), r = building_of(u);
    int j, k, x, who, oth;
    unsigned sq, at;

    if (GW(game_flags) & 0x40) {
        stop_song();
        ++*stopped;
    }
    SW(game_flags, GW(game_flags) | 0x1000);
    SW(number_asked, 1);
    sq = pw(r, 0x0E);
    t0e9b_15dd(side, (int)sq);
    draw_window(GWO(cursors, 0x31 * side + 2), side);
    at = mark_at(sq);
    SBO(marks, at, GBO(marks, at) | 0x30);
    draw_marks(GWO(cursors, 0x31 * side + 2), side);
    SBO(marks, at, GBO(marks, at) & 0xCF);
    x = other ? 0xA0 : 0;
    who = other ? 0 : 1;
    play_anim(who, buffer, x, 0, (int8_t)*first_anim, make_path(1, 3, 0, -1));
    *first_anim = 0;
    for (j = 0; j < 7; j++)
        spb(r, (unsigned)(7 * other + j), pb(r, (unsigned)(7 * side + j)));
    for (j = 0; j < 7; j++)
        if (pb(r, (unsigned)(7 * other + j)) > 0xF0)
            break;
    if (j == 7) {
        /* no slot free: the first unit inside is given up */
        spb(u, 0x19, 0);
        unit_remove(pb(r, (unsigned)(7 * other)), other);
    } else
        spb(u, 0x19, j);
    oth = (other & 1) ? 0 : 1;
    for (k = 0; k <= 0xF0; k++) {
        fptr v = UNIT(k);

        if (place(v, other) != pw(r, 0x0E) || (pw(v, 4) & 0x8000))
            continue;
        if (!(pw(v, 4) & 2))
            spb(PLAYER(oth), 2, pb(PLAYER(oth), 2) - 1);
        unit_to(v, other);
        spb(PLAYER(other), 2, pb(PLAYER(other), 2) + 1);
        if (pw(v, 4) & 0x1000) {
            fptr c = REC(cargo, pb(v, 0x0A));

            spw(c, 0x19, (pw(c, 0x19) & 0xFFFC) | other);
        }
    }
    if (pw(r, 0x19) & 8) {
        if (!(pw(r, 0x19) & 2))
            spb(PLAYER(oth), 3, pb(PLAYER(oth), 3) - 1);
        spb(PLAYER(other), 3, pb(PLAYER(other), 3) + 1);
        spb(map, pw(r, 0x0E), GBO(amok, (other & 1) ? 1 : 0));
    } else if (pw(r, 0x19) & 0x10) {
        if (!(pw(r, 0x19) & 2))
            spb(PLAYER(oth), 4, pb(PLAYER(oth), 4) - 1);
        spb(PLAYER(other), 4, pb(PLAYER(other), 4) + 1);
        spb(map, pw(r, 0x0E), GBO(amok, (other & 1) ? 4 : 3));
    } else if (pw(r, 0x19) & 4) {
        *ret = other & 0xFF;
        spb(map, pw(r, 0x0E), GBO(amok, (other & 1) ? 7 : 6));
    }
    spw(r, 0x19, (pw(r, 0x19) & 0xFFFC) | other);
    spb(r, 0x18, 5);
    spb(r, (unsigned)(7 * other) + pb(u, 0x19), n);
    spb(r, 0x16 + other, pb(r, 0x16 + side));
    spb(map, place(u, other), 0xF4);
    spw(u, 0x0B + 2 * other, pw(r, 0x0E));
    spw(u, 4, (pw(u, 4) & 0xFEFF) | 0x4200);
    if (pw(u, 4) & 0x1000)
        cargo_follow(n, other);
    copy_page();
    draw_window(GWO(cursors, 0x31 * side + 2), side);
    draw_marks(GWO(cursors, 0x31 * side + 2), side);
    flip_page();
    t0d36_000f(0x32);
}

/* a unit's points back by one towards its type's (+3 for a type with 4
 * in its +0Eh, else +2) */
static void repair(fptr u)
{
    fptr t = TYPE(pb(u, 8));
    unsigned o = (pw(t, 0x0E) & 4) ? 3 : 2;

    if (pb(t, o) > pb(u, o))
        spb(u, o, pb(u, o) + 1);
}

/* the change of phase.  `buffer` takes the fight scene's files (and is
 * the work buffer for what is loaded again afterwards into `libs`),
 * `pmp` the map's overview, `palette` the palette again.  FFh, or the
 * player who took a headquarters; FFh also when a file is missing. */
int change_phase(fptr buffer, fptr libs, fptr pmp, fptr palette)
{
    fptr fr = FP(fight_record), map, p, work, name;
    int ret = 0xFF, stopped = 0, side, other, first_anim = 1, i, n0 = 0, n1 = 0;
    unsigned idx = 0, n;

    clear_marks(0xFF);
    show_message(-1, 0);
    show_message(-1, 1);
    SW(game_flags, GW(game_flags) & 0xFFF3);
    /* the player whose orders are carried out, and the other */
    if (pb(CURSOR(0), 0x16) == 2)
        side = 0, other = 1;
    else
        side = 1, other = 0;
    map = side ? GFP(map1) : GFP(map0);
    if (pb(PLAYER(side), 0x15) > 0) {
        /* the fight scene's libraries and sounds */
        p = buffer;
        if (GW(game_flags) & 0x40) {
            stop_song();
            stopped++;
        }
        spfp(fr, 0x46, p);
        work = p;
        p = hadd(p, 0x2710);
        if (scene_lib(FP(lib_fight), &p, work, 0) || scene_lib(FP(lib_bum), &p, work, 0)
            || scene_lib(FP(lib_rand), &p, work, 0))
            return 0xFF;
        if (!load_effects(p, make_path(1, -1, FP(name_fight), 8), work))
            return 0xFF;
        spfp(fr, 0x3A, pfp(FP(lib_bum), 0x0E));
        spfp(fr, 0x3E, pfp(FP(lib_rand), 0x0E));
        spfp(fr, 0x42, pfp(FP(lib_fight), 0x0E));
        SW(game_flags, GW(game_flags) | 0x1000);
    }
    while (pb(PLAYER(side), 0x15) > idx) {
        int a = pb(pfp(PLAYER(side), 0x11), idx);

        idx = (idx + 1) & 0xFF;
        fight(a, pw(UNIT(a), 0x11), side, other, map);
    }
    /* the dead: a move of the phase taken back, the unit given up, and
     * what it held */
    for (n = 0; n <= 0xF0; n++) {
        fptr u = UNIT(n);
        int holder;

        if ((pw(u, 4) & 0x8000) || !(pb(u, 0x14) & 4))
            continue;
        holder = (pw(u, 4) & 0x1000) != 0;
        if (pb(u, 0x14) & 1)
            move_undo((int)n, map, other);
        unit_remove((int)n, other);
        if (holder) {
            fptr r = REC(cargo, (int8_t)pb(u, 0x0A));

            for (i = 0; i < 7; i++) {
                unsigned s = pb(r, (unsigned)(7 * side + i));

                if (s > 0xF0)
                    continue;
                if (pb(UNIT(s), 0x14) & 1)
                    move_undo((int)s, map, other);
                unit_remove((int)s, other);
            }
        }
    }
    show_message(-1, 0);
    show_message(-1, 1);
    check_vga_disk();
    /* the ends of the moves, by the units' +13h */
    for (n = 0; n <= 0xF0; n++) {
        fptr u = UNIT(n), r, h, m2;
        unsigned cnt, d;

        if (pw(u, 4) & 0x8000)
            continue;
        switch ((int8_t)pb(u, 0x13)) {
        case 1:
            /* a unit taken aboard is the taker's */
            h = UNIT(pb(u, 0x16));
            if (pw(h, 4) & 0x8000) {
                SBO(cargo, 0x1C * (int8_t)pb(u, 0x0A) + 7 * other + pb(u, 0x19), 0xFF);
                break;
            }
            if (!same_side(pw(u, 4), pw(h, 4)) && !(pw(h, 4) & 2))
                spb(PLAYER(other ? 0 : 1), 2, pb(PLAYER(other ? 0 : 1), 2) - 1);
            if (!same_side(pw(u, 4), pw(h, 4)) || (pw(h, 4) & 2))
                spb(PLAYER(other), 2, pb(PLAYER(other), 2) + 1);
            unit_to(h, other);
            break;
        case 5:
            /* into its own building: what a holder brought with 4 in its
             * +6 becomes energy, 8 a point, at most FAh */
            if (!(pw(u, 4) & 0x1000))
                break;
            r = building_of(u);
            for (i = 0; i < 7; i++) {
                unsigned s = pb(r, (unsigned)(7 * other + i)), e;

                if (s > 0xF0 || !(pw(UNIT(s), 6) & 4))
                    continue;
                /* MOON (T03EB:1290): 6 a point */
                e = (pb(UNIT(s), 2) * (bi_prog == BI_MOON ? 6 : 8)) & 0xFF;
                if (pb(r, 0x16 + other) + e > 0xFA)
                    spb(r, 0x16 + other, 0xFA);
                else
                    spb(r, 0x16 + other, pb(r, 0x16 + other) + e);
                unit_remove((int)s, other);
                spb(r, (unsigned)(7 * other + i), 0xFF);
            }
            break;
        case 4:
            take_building((int)n, side, other, map, buffer, &first_anim, &stopped, &ret);
            break;
        case 0x0B:
            /* four squares of ground with 8000h and no unit: AMOK's four
             * parts +18h there */
            cnt = 0;
            m2 = other ? GFP(map1) : GFP(map0);
            four_squares(pw(u, 0x11));
            for (i = 0; i < 4; i++) {
                unsigned sq = GWO(around, 2 * i);

                if ((GWO(ground, 6 * pb(m2, sq)) & 0x8000) && pb(m2, sq + 1) > 0xF0)
                    cnt++;
            }
            if (cnt == 4) {
                for (i = 0; i < 4; i++)
                    spb(map, GWO(around, 2 * i), GBO(amok, 0x18 + i));
                spb(u, 0x0A, pb(u, 0x0A) - 1);
            }
            break;
        case 0x0D:
            /* a depot built on four squares */
            d = GB(depots_made);
            SB(depots_made, d + 1);
            t169e_0324((int)d, pw(u, 0x11), side);
            spb(PLAYER(side), 4, pb(PLAYER(side), 4) + 1);
            spb(REC(depots, d), 0x16 + other, 0x0A);
            four_squares(pw(u, 0x11));
            for (i = 1; i < 4; i++)
                spb(map, GWO(around, 2 * i), GBO(amok, 0x1C + i));
            spb(map, GWO(around, 0), GBO(amok, side ? 4 : 3));
            spb(u, 0x0A, pb(u, 0x0A) - 1);
            break;
        }
    }
    if (GW(game_flags) & 0x1000) {
        /* the map's own files again where the scene's or the film's were */
        work = buffer;
        p = libs;
        if (bi_prog == BI_MOON) {
            /* MOON.EXE has no .PMP: MAPINFO.DAT and MAP02.DAT or MAP04.DAT
             * are read one behind the other into `libs`, which goes on
             * behind them (the overview's scale is left as it is) */
            long size = t2624_0006(p, make_path(1, -1, FP(name_mapinfo), -1), work, NULL);

            if (size == -1)
                return 0xFF;
            SFP(mapinfo_data, p);
            p = hadd(p, size);
            name = (int16_t)GW(map_width) > 0x20 || (int16_t)GW(map_height) > 0x28
                       ? FP(name_map02) : FP(name_map04);
            size = t2624_0006(p, make_path(1, -1, name, -1), work, NULL);
            if (size == -1)
                return 0xFF;
            SFP(overview_data, p);
            p = hadd(p, size);
        } else {
            name = t26ea_000f((int16_t)GW(map_number), FP(save_name), 2, 4);
            if (!load_file(pmp, make_path(1, 1, name, 1), work))
                return 0xFF;
        }
        name = t26ea_000f((int8_t)GBO(menu_items, 11 * 0x2E + 0x2B), FP(save_name), 2, 4);
        if (!load_file(palette, make_path(1, -1, name, 4), work))
            fatal_error(2);
        for (i = 0; i < 0x300; i++)
            SBO(picture_palette, i, pb(palette, (unsigned)i));
        set_palette(0xFF, FP(picture_palette));
        if (scene_lib(FP(lib_cursor), &p, work, 0) || scene_lib(FP(lib_shop), &p, work, 0)
            || scene_lib(FP(lib_bigunit), &p, work, 1))
            return 0xFF;
        if (load_song(p, make_path(1, -1, FP(name_game), 7), work) == -1)
            return 0xFF;
        p = hadd(p, 0xA028);
        if (!load_effects(p, make_path(1, -1, FP(name_game), 8), work))
            return 0xFF;
        SW(game_flags, GW(game_flags) & 0xEFFF);
    }
    sync_slots(FP(cargo), side, other, 0x46);
    sync_slots(FP(depots), side, other, 0x0A);
    sync_slots(FP(factories), side, other, 0x0A);
    sync_slots(FP(hqs), side, other, 2);
    /* this player's map: the units out, the other's units and those of
     * nobody where the other's view has them, the units in again; then
     * all of it into the other's map */
    for (i = 1; (int16_t)GW(map_bytes) > i; i += 2)
        spb(map, (unsigned)i, 0xFF);
    for (n = 0; n <= 0xF0; n++) {
        fptr u = UNIT(n);
        unsigned f = pw(u, 4);

        if (f & 0x8000)
            continue;
        if (other ? !(f & 1) : (f & 3) != 0)
            continue;
        spw(u, 0x0B + 2 * side, place(u, other));
        spb(u, 0x0F + side, pb(u, 0x0F + other));
    }
    drop_empty();
    for (n = 0; n <= 0xF0; n++)
        if (!(pw(UNIT(n), 4) & 0xC000))
            spb(map, pw(UNIT(n), 0x0B), n);
    if (side)
        t0e9b_19ab(GFP(map1), GFP(map0));
    else
        t0e9b_19ab(GFP(map0), GFP(map1));
    /* the cursors free, the modes exchanged */
    for (i = 0; i < 2; i++) {
        spb(CURSOR(i), 0x17, 0);
        spb(CURSOR(i), 0x18, 0);
        spb(CURSOR(i), 0x19, 0);
        spb(CURSOR(i), 0x1B, 5);
    }
    if (pb(CURSOR(0), 0x16) == 1) {
        spb(CURSOR(0), 0x16, 2);
        spb(CURSOR(1), 0x16, 1);
    } else {
        spb(CURSOR(0), 0x16, 1);
        spb(CURSOR(1), 0x16, 2);
    }
    spb(PLAYER(1), 0x15, 0);
    spb(PLAYER(0), 0x15, 0);
    show_message(pb(CURSOR(0), 0x16) == 1 ? 3 : 4, 0);
    show_message(pb(CURSOR(0), 0x16) == 1 ? 4 : 3, 1);
    SW(game_flags2, GW(game_flags2) | 0x18);
    if (pb(CURSOR(0), 0x16) == 1)
        SW(round, GW(round) + 1);
    clear_marks(0xFF);
    /* the units' flags of the phase; a type with 1 in its +0Eh keeps
     * "has moved"; a unit with 8000h in its +6 mends itself and what it
     * holds */
    for (n = 0; n <= 0xF0; n++) {
        fptr u = UNIT(n);

        spw(u, 4, pw(u, 4) & 0xF0FF);
        if (!(pw(u, 4) & 0x8000) && (pb(u, 0x14) & 1) && (pw(TYPE(pb(u, 8)), 0x0E) & 1) && !(pw(u, 4) & 0x4000)) {
            unit_flag((int)n, 0x200, pw(u, 4) & 1, 1);
            spw(u, 4, pw(u, 4) | 0x200);
        }
        if (pw(u, 6) & 0x8000) {
            repair(u);
            if (pw(u, 4) & 0x1000) {
                fptr r = REC(cargo, (int8_t)pb(u, 0x0A));

                for (i = 0; i < 7; i++)
                    if (pb(r, (unsigned)i) <= 0xF0)
                        repair(UNIT(pb(r, (unsigned)i)));
            }
        }
        spb(u, 0x14, 0);
        spw(u, 0x11, 0);
        spb(u, 0x16, 0);
        spb(u, 0x15, 0);
        spb(u, 0x13, 0);
    }
    /* a player without a unit that counts has lost */
    for (n = 0; n <= 0xF0; n++) {
        fptr u = UNIT(n);

        if ((pw(u, 4) & 0x8000) || (pw(u, 6) & 0x24))
            continue;
        if (!(pw(u, 4) & 3))
            n0++;
        else if (pw(u, 4) & 1)
            n1++;
    }
    if (n0 == 0 || n1 == 0) {
        spb(CURSOR(1), 0x17, 7);
        spb(CURSOR(0), 0x17, 7);
        if (n0 == 0 && n1 == 0) {
            spw(CURSOR(1), 0x22, 0x15);
            spw(CURSOR(0), 0x22, 0x15);
        } else if (n0 == 0) {
            spw(CURSOR(0), 0x22, 0x12);
            spw(CURSOR(1), 0x22, 0x11);
        } else {
            spw(CURSOR(0), 0x22, 0x11);
            spw(CURSOR(1), 0x22, 0x12);
        }
    }
    SD(score_now, (uint32_t)score());
    if ((GW(game_flags) & 0x40) && stopped)
        play_song(0, 0);
    return ret;
}
