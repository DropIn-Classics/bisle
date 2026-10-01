/* move.c - BATTLE.EXE's T122D: a move.  move_aim marks the way to the
 * square fire chose, t122d_05b8 takes the second fire there and starts
 * the move, move_step is the timer's step along the way, move_arrive
 * what the last square does with the unit (stop_check says beforehand
 * what that will be, into move_record), move_undo takes it back.
 *
 * move_record (F27EE:1326): +0 the unit moving, +1 the unit on the last
 * square, +2 and +3 what the map held under the unit's two halves, +4
 * the slot, +5 the far pointer to the record that takes the unit, +9
 * that record's number, +0Ah the square, +0Ch what arriving is: 1 the
 * mover takes the unit there aboard, 2 it goes aboard the unit there, 3
 * a plain square, 4 a building not its own, 5 into its own building.
 */
#include "bi.h"

#define PLAYER(n) MKFP(S_players, A_players + 0x17 * (n))
#define REC(table, n) MKFP(S_##table, A_##table + 0x1C * (n))

/* a unit's square + 1 in the map of `side` */
static unsigned place(fptr u, int side) { return pw(u, 0x0B + 2 * side); }
static void set_place(fptr u, int side, unsigned v) { spw(u, 0x0B + 2 * side, v); }

/* the other half of a unit of two squares */
static fptr other_half(int unit)
{
    return UNIT((pw(UNIT(unit), 4) & 0x80) ? unit + 1 : unit - 1);
}

/* a square's place in the marks: 64 a row */
static unsigned mark_at(unsigned off)
{
    int s = (int16_t)off >> 1, w = (int16_t)GW(map_width);

    return (uint16_t)(s % w + ((s / w) << 6));
}

static void message_to(int number, int side)
{
    show_message(number, side);
    SW(game_flags2, GW(game_flags2) | (side ? 0x10 : 8));
}

/* the slots of `side` in a record that hold a unit (used) or none */
int count_slots(int side, fptr rec, int used)
{
    int n = 0, i;

    for (i = 0; i < 7; i++)
        if ((pb(rec, (unsigned)(7 * side + i)) <= 0xF0) == (used != 0))
            n++;
    return n;
}

static int free_slot(fptr rec, int side)
{
    int i;

    for (i = 0; i < 7; i++)
        if (pb(rec, (unsigned)(7 * side + i)) > 0xF0)
            break;
    return i;
}

/* the units in `from`'s slots into free slots of `to`, each at `to`'s
 * square */
void cargo_move(int side, fptr from, fptr to)
{
    int i, j;

    for (i = 0; i < 7; i++) {
        unsigned s = pb(from, (unsigned)(7 * side + i));

        if (s > 0xF0)
            continue;
        for (j = 0; j < 7; j++)
            if (pb(to, (unsigned)(7 * side + j)) > 0xF0) {
                spb(to, (unsigned)(7 * side + j), s);
                set_place(UNIT(s), side, pw(to, 0x0E));
                spb(from, (unsigned)(7 * side + i), 0xFF);
                break;
            }
    }
}

/* the unit of the cursor let go: its marks off; with 10h in `kind` the
 * move given up; a unit taken out of a building (the cursor's +19h 3)
 * goes back into its slot with 10h, and the cursor back into the
 * building's screen */
void unit_release(fptr cur, fptr map, int side, int kind)
{
    int unit = pw(cur, 0x1E);
    fptr u = UNIT(unit);
    int sq = (uint16_t)(place(u, side) - 1), sq2 = 0, m;

    if (pw(u, 4) & 0x40)
        sq2 = (uint16_t)(place(other_half(unit), side) - 1);
    m = side ? 2 : 1;
    undraw_marks(GWO(cursors, 0x31 * side + 2), side, m);
    clear_marks(m);
    draw_marks(GWO(cursors, 0x31 * side + 2), side);
    if (kind & 0x10) {
        spw(u, 4, pw(u, 4) & 0xFEFF);
        if (pw(u, 4) & 0x40) {
            fptr v = other_half(unit);

            spw(v, 4, pw(v, 4) & 0xFEFF);
        }
        clear_marks(side ? 0x80 : 0x40);
        spb(cur, 0x17, 0);
        spb(cur, 0x18, 0x19);
        spb(cur, 0x1B, 5);
        SW(game_flags2, GW(game_flags2) | 0x100);
        SW(number_asked, 3);
    }
    if (pb(cur, 0x19) == 3) {
        unsigned n = pb(cur, 0x1E);

        if (kind & 0x10) {
            spb(map, place(u, side), 0xF4);
            set_place(u, side, pw(pfp(cur, 0x24), 0x0E));
            if ((pw(cur, 0x24) | pw(cur, 0x26)) && (pw(pfp(cur, 0x24), 0x19) & 0x20))
                spb(map, place(u, side), pb(pfp(cur, 0x24), 0x1B));
        }
        t0d36_032c(cur, 1);
        if (kind & 0x10)
            spb(pfp(cur, 0x24), 7 * side + pw(cur, 0x0E), n);
        spb(cur, 0x17, 2);
        spb(cur, 0x18, 9);
        spb(cur, 0x19, 0);
        spb(cur, 0x1B, 5);
        spw(cur, 0x0C, pw(cur, 4));
        spw(cur, 0x0E, pw(cur, 6));
    }
    redraw_square(side, sq);
    if (pw(u, 4) & 0x40)
        redraw_square(side, sq2);
}

/* fire on a square with a unit chosen: 1 (with 8 for a unit of two
 * squares) and the way marked when the unit can go and stop there, else
 * 12h, with a message when stop_check or find_path says why not */
int move_aim(fptr cur, fptr map, int side)
{
    int r = 1, e, n, m;
    fptr u = UNIT(pw(cur, 0x1E)), list;

    if (!(GBO(marks, mark_at(pw(cur, 0))) & (side ? 0xAA : 0x55) & 3))
        r = 0x12;
    if (place(u, side) == (uint16_t)(pw(cur, 0) + 1))
        r = 0x12;
    if ((pw(u, 4) & 0x10) && (GWO(ground, 6 * pb(map, pw(cur, 0))) & 0x4000))
        r = 0x12;
    SB(move_record, pb(cur, 0x1E));
    if (r & 1) {
        e = stop_check(pw(cur, 0), side, map);
        if (e) {
            message_to(e, side);
            r = 0x12;
        }
    }
    if (pw(u, 4) & 0x40)
        r |= 8;
    if (!(r & 1))
        return r;
    if (find_path(pfp(PLAYER(side), 0x0D), pw(cur, 0x1E), (uint16_t)(place(u, side) - 1), pw(cur, 0), side, map)) {
        clear_marks(side ? 0x80 : 0x40);
        r = 0x12;
        message_to(0x16, side);
        return r;
    }
    n = (int16_t)GW(path_count);
    list = pfp(PLAYER(side), 0x0D);
    list = MKFP(FSEG(list), FOFF(list) + 0x1F40);
    m = side ? 2 : 1;
    undraw_marks(GWO(cursors, 0x31 * side + 2), side, m);
    clear_marks(m);
    while (n-- > 0) {
        unsigned at = mark_at(pw(list, (unsigned)(2 * n)));

        SBO(marks, at, GBO(marks, at) | m);
    }
    spw(u, 0x11, pw(cur, 0));
    draw_marks(GWO(cursors, 0x31 * side + 2), side);
    return r;
}

/* T122D:05B8: fire again on the square the way leads to: the move
 * begins (the unit into the player's list of moves, the timer of kind 4
 * for the steps), 1; else 12h; 8 with either for a unit of two squares */
int t122d_05b8(fptr cur, int side)
{
    int r = 1;
    fptr u = UNIT(pw(cur, 0x1E)), pl = PLAYER(side), rec = FP(move_record);

    if (pw(u, 0x11) != pw(cur, 0))
        r = 0x12;
    if (pw(u, 4) & 0x40)
        r |= 8;
    SB(move_record, pb(cur, 0x1E));
    if (!(r & 1))
        return r;
    spb(pfp(pl, 0x11), pb(pl, 0x15), pb(cur, 0x1E));
    spb(pl, 0x15, pb(pl, 0x15) + 1);
    spw(u, 0x11, place(u, side));
    SW(path_count, GW(path_count) - 2);
    spb(rec, 3, 0xFF);
    spb(rec, 2, 0xFF);
    if (pb(cur, 0x19) == 3)
        spb(rec, 2, pb(pfp(cur, 0x24), 0x1B));
    SW(game_flags2, GW(game_flags2) | 0x80);
    timer_set(4, 0, 0);
    SW(game_flags2, GW(game_flags2) | 0x100);
    SW(number_asked, 2);
    return r;
}

/* the direction (0 up, then clockwise to 5) from a square to the next */
int direction(int from, int to)
{
    int w = (int16_t)GW(map_width);
    int c1 = ((int16_t)from >> 1) % w, r1 = ((int16_t)from >> 1) / w;
    int c2 = ((int16_t)to >> 1) % w, r2 = ((int16_t)to >> 1) / w;

    if (c2 == c1)
        return r2 < r1 ? 0 : 3;
    if (c1 & 1) {
        if (c2 < c1)
            return r2 > r1 ? 4 : 5;
        return r2 > r1 ? 2 : 1;
    }
    if (c2 < c1)
        return r2 < r1 ? 5 : 4;
    return r2 < r1 ? 1 : 2;
}

/* a step of the move (the timer of kind 4): the unit a square on along
 * the way in its player's map, both halves of a unit of two squares;
 * after the last square move_arrive and its message */
void move_step(void)
{
    fptr rec = FP(move_record);
    int n = pb(rec, 0), side = pw(UNIT(n), 4) & 1, e;
    fptr list = pfp(PLAYER(side), 0x0D), u = UNIT(n), map;
    unsigned to;

    list = MKFP(FSEG(list), FOFF(list) + 0x1F40);
    to = pw(list, (unsigned)(2 * GW(path_count)));
    map = side ? GFP(map1) : GFP(map0);
    if (GW(path_count) == 0)
        stop_check((int)to, side, map);
    if (pw(u, 4) & 0x40) {
        fptr v = UNIT(n + 1);
        unsigned old = place(u, side);

        spb(map, place(v, side), pb(rec, 3));
        redraw_square(side, (uint16_t)(place(v, side) - 1));
        spb(rec, 3, pb(rec, 2));
        spb(rec, 2, pb(map, to + 1));
        spb(map, to + 1, n);
        spb(u, 0x0F + side, direction((uint16_t)(place(u, side) - 1), (int)to));
        set_place(u, side, to + 1);
        redraw_square(side, (uint16_t)(place(u, side) - 1));
        spb(map, old, n + 1);
        spb(v, 0x0F + side, pb(u, 0x0F + side));
        set_place(v, side, old);
        redraw_square(side, (uint16_t)(old - 1));
    } else {
        spb(map, place(u, side), pb(rec, 2));
        redraw_square(side, (uint16_t)(place(u, side) - 1));
        spb(rec, 2, pb(map, to + 1));
        spb(map, to + 1, n);
        spb(u, 0x0F + side, direction((uint16_t)(place(u, side) - 1), (int)to));
        set_place(u, side, to + 1);
        redraw_square(side, (uint16_t)(place(u, side) - 1));
    }
    if (pw(u, 4) & 0x1000)
        cargo_follow(n, side);
    if ((int16_t)GW(path_count) > 0) {
        timer_set(4, (int8_t)pb(TYPE(pb(u, 8)), 0x16), 0);
        SW(path_count, GW(path_count) - 1);
        return;
    }
    e = move_arrive(side, map);
    message_to(e, side);
    SW(game_flags2, GW(game_flags2) & 0xFF7F);
    spw(u, 4, pw(u, 4) & 0xFEFF);
    if (pw(u, 4) & 0x40) {
        fptr v = other_half(n);

        spw(v, 4, pw(v, 4) & 0xFEFF);
    }
}

/* the move's end by move_record's +0Ch; the message's number.  The
 * unit's +13h keeps what it was, +14h bit 1 that it moved, +15h, +16h,
 * +17h, +19h what move_undo needs. */
int move_arrive(int side, fptr map)
{
    fptr rec = FP(move_record), u = UNIT(pb(rec, 0)), r = pfp(rec, 5), h;
    int kind = pb(rec, 0x0C), ret = 0;

    switch (kind) {
    case 1:
        h = UNIT(pb(rec, 1));
        spb(r, (unsigned)(7 * side) + pb(rec, 4), pb(rec, 1));
        set_place(u, side, pw(rec, 0x0A) + 1);
        spb(map, place(u, side), pb(rec, 0));
        cargo_follow(pb(rec, 0), side);
        spw(u, 0x17, pw(h, 4));
        spw(h, 4, pw(h, 4) | 0x4200);
        spw(u, 4, (pw(u, 4) & 0xBEFF) | 0x200);
        ret = (pw(h, 6) & 0x20) ? 0x11 : 0x12;
        unit_flag(pb(rec, 0), 0x200, side, 1);
        spb(u, 0x13, kind);
        spb(u, 0x14, pb(u, 0x14) | 1);
        spb(u, 0x19, pb(rec, 4));
        spb(u, 0x16, pb(rec, 1));
        break;
    case 2:
        spb(r, (unsigned)(7 * side) + pb(rec, 4), pb(rec, 0));
        spb(map, place(u, side), pb(rec, 1));
        spw(u, 4, (pw(u, 4) & 0xFEFF) | 0x4200);
        if (pw(u, 4) & 0x1000)
            cargo_move(side, REC(cargo, (int8_t)pb(u, 0x0A)), pfp(rec, 5));
        redraw_square(side, (uint16_t)(place(u, side) - 1));
        spb(u, 0x13, kind);
        spb(u, 0x19, pb(rec, 4));
        spb(u, 0x15, pb(rec, 9));
        spb(u, 0x16, pb(rec, 1));
        spb(u, 0x14, pb(u, 0x14) | 1);
        ret = 6;
        break;
    case 3:
        ret = 6;
        spw(u, 4, (pw(u, 4) & 0xBEFF) | 0x200);
        unit_flag(pb(rec, 0), 0x200, side, 1);
        spb(u, 0x13, kind);
        if (pw(u, 4) & 0x1000)
            cargo_follow(pb(rec, 0), side);
        spb(u, 0x14, pb(u, 0x14) | 1);
        spb(u, 0x15, 0);
        spb(u, 0x19, 0);
        spw(u, 0x17, 0);
        spb(u, 0x16, 0);
        break;
    case 4:
        ret = 6;
        spw(u, 4, (pw(u, 4) & 0xBEFF) | 0x200);
        unit_flag(pb(rec, 0), 0x200, side, 1);
        spb(u, 0x13, kind);
        spb(u, 0x14, pb(u, 0x14) | 1);
        if (pw(u, 4) & 0x1000)
            cargo_follow(pb(rec, 0), side);
        spw(u, 0x17, pw(pfp(rec, 5), 0x19));
        spb(u, 0x15, pb(rec, 9));
        spb(u, 0x16, 0);
        break;
    case 5:
        spb(u, 0x13, kind);
        spb(u, 0x14, pb(u, 0x14) | 1);
        spb(u, 0x19, pb(rec, 4));
        spb(u, 0x15, pb(rec, 9));
        spw(u, 0x17, pw(r, 0x19));
        spb(r, (unsigned)(7 * side) + pb(rec, 4), pb(rec, 0));
        if (pw(u, 4) & 0x1000)
            cargo_move(side, REC(cargo, (int8_t)pb(u, 0x0A)), pfp(rec, 5));
        spb(map, place(u, side), 0xF4);
        set_place(u, side, pw(pfp(rec, 5), 0x0E));
        spw(u, 4, (pw(u, 4) & 0xFEFF) | 0x4200);
        /* the square + 1, as the original has it */
        redraw_square(side, place(u, side));
        ret = 6;
        break;
    }
    return ret;
}

/* may the unit of move_record stop on the square, in the map of `side`:
 * 0 and what arriving will be in move_record, or a message's number */
int stop_check(int off, int side, fptr map)
{
    fptr rec = FP(move_record), u = UNIT(pb(rec, 0)), h, r, r2;
    unsigned sq = (uint16_t)off, g;
    int e = 0, i, total, fits, found = 0, n = 0;

    spb(rec, 0x0C, 3);
    spw(rec, 0x0A, sq);
    if (pb(map, sq + 1) <= 0xF0) {
        /* a unit there */
        unsigned t = pb(map, sq + 1);

        h = UNIT(t);
        spb(rec, 1, t);
        if ((pw(h, 4) & 0x40) && !(pw(h, 4) & 0x80))
            h = UNIT(t - 1);
        if (((pw(h, 6) & 0x20) && (pw(u, 6) & 0x1000)) || ((pw(h, 6) & 4) && (pw(u, 6) & 0x2000))) {
            /* the mover takes it aboard */
            r = REC(cargo, (int8_t)pb(u, 0x0A));
            spfp(rec, 5, r);
            i = free_slot(r, side);
            if (i == 7)
                return 9;
            if (cargo_size(r, side) + pb(TYPE(pb(h, 8)), 0x40) > pb(TYPE(pb(u, 8)), 0x3F))
                return 9;
            spb(rec, 4, i);
            spb(rec, 0x0C, 1);
            return 0;
        }
        if (!same_side(pw(h, 4), side))
            return 5;
        if (!(pw(h, 4) & 0x1000))
            return 7;
        r = REC(cargo, (int8_t)pb(h, 0x0A));
        spfp(rec, 5, r);
        spb(rec, 9, pb(h, 0x0A));
        if (!(pw(u, 6) & pw(r, 0x19) & 0x0FC0) || (pw(h, 4) & 0x200))
            return 8;
        i = free_slot(r, side);
        if (i == 7)
            return 9;
        fits = 1;
        total = pb(TYPE(pb(u, 8)), 0x40);
        if (pw(u, 4) & 0x1000) {
            r2 = REC(cargo, (int8_t)pb(u, 0x0A));
            total += cargo_size(r2, side);
            if (count_slots(side, r, 0) < count_slots(side, r2, 1) + 1)
                fits = 0;
        }
        total += cargo_size(r, side);
        if (total > pb(TYPE(pb(h, 8)), 0x3F) || !fits)
            return 9;
        spb(rec, 4, i);
        spb(rec, 0x0C, 2);
        return 0;
    }
    g = GWO(ground, 6 * pb(map, sq));
    r = 0;
    if (g & 0x400) {
        n = find_building((int)sq, FP(factories));
        r = REC(factories, n);
        found = 1;
    } else if (g & 0x100) {
        n = find_building((int)sq, FP(depots));
        r = REC(depots, n);
        found = 1;
    } else if (g & 0x40) {
        n = (g & 1) ? 1 : 0;
        r = REC(hqs, n);
        found = 1;
    }
    if (found) {
        spb(rec, 9, n);
        i = free_slot(r, side);
        if (!same_side(pw(r, 0x19), side) || (pw(r, 0x19) & 2)) {
            spb(rec, 0x0C, 4);
            spfp(rec, 5, r);
            return 0;
        }
        if (pw(u, 4) & 0x1000) {
            r2 = REC(cargo, (int8_t)pb(u, 0x0A));
            if (count_slots(side, r, 0) < count_slots(side, r2, 1) + 1)
                i = 7;
        }
        if (i == 7)
            return 9;
        spb(rec, 4, i);
        spb(rec, 0x0C, 5);
        spfp(rec, 5, r);
        return 0;
    }
    if (!(pb(TYPE(pb(u, 8)), 4) & GBO(ground, 6 * pb(map, sq) + 2)))
        e = 0x0E;
    if ((pw(u, 4) & 0x10) && (g & 0x4000))
        e = 0x0E;
    return e;
}

/* a move's end taken back by the unit's +13h (the unit stays where it
 * is; what move_arrive put into slots and flags is undone) */
void move_undo(int unit, fptr map, int side)
{
    fptr u, r, h;

    unit &= 0xFF;
    u = UNIT(unit);
    switch (pb(u, 0x13)) {
    case 2:
        r = REC(cargo, pb(u, 0x15));
        spb(r, (unsigned)(7 * side) + pb(u, 0x19), 0xFF);
        break;
    case 3:
    case 4:
        spb(map, place(u, side), 0xFF);
        if (pw(u, 4) & 0x40) {
            h = UNIT(((pw(u, 4) & 0x80) ? unit + 1 : unit - 1) & 0xFF);
            spb(map, place(h, side), 0xFF);
            spb(h, 0x14, pb(h, 0x14) & 0xFE);
            spw(h, 4, pw(h, 4) & 0xBDFF);
        }
        break;
    case 1:
        h = UNIT(pb(u, 0x16));
        spb(map, place(u, side), pb(u, 0x16));
        set_place(h, side, place(u, side));
        spw(h, 4, pw(u, 0x17));
        spb(h, 0x14, 0);
        SBO(cargo, 0x1C * (int8_t)pb(u, 0x0A) + 7 * side + pb(u, 0x19), 0xFF);
        break;
    case 5:
        if (pw(u, 0x17) & 4)
            r = REC(hqs, pb(u, 0x15));
        else if (pw(u, 0x17) & 8)
            r = REC(factories, pb(u, 0x15));
        else
            r = REC(depots, pb(u, 0x15));
        spb(r, (unsigned)(7 * side) + pb(u, 0x19), 0xFF);
        break;
    default:
        spb(u, 0x13, 0);
        return;
    }
    spb(u, 0x14, pb(u, 0x14) & 0xFE);
    spw(u, 4, pw(u, 4) & 0xBDFF);
    spb(u, 0x13, 0);
}
