/* computer.c - BATTLE.EXE's T178C, T17C0, T1ABC, T1B01: the computer
 * player.  The map's loop calls computer_step once a pass for a player
 * who is the computer; it does one small step of a stage: the two sides
 * assessed and the aims listed (computer_assess), a task for every unit
 * (computer_plan, plan.c), the tasks handed out as commands
 * (computer_hand_out), a command carried out as keys (command.c).  The
 * computer moves and fires through the player's own cursor.
 */
#include "ai.h"

#define COMMAND FP(computer_command)

/* the message's number by the stage, shown by the map's loop */
static void message(int player, unsigned n) { SWO(computer_keys, 4 * player + 2, n); }

/* a pass's step for the player */
void computer_step(int player)
{
    fptr s = AI_STATE(player);

    message(player, 0x1C);
    if (pw(s, 0) & 0x20) {
        message(player, 0x2F);
        if (computer_assess(player)) {
            spw(s, 0, (pw(s, 0) & 0xFFDF) | 0x40);
            spb(AI_QUEUE(player), 0x46, 0);
        }
    } else if (pw(s, 0) & 0x40) {
        if (pb(s, 0x0B)) {
            message(player, 0x30);
            spb(s, 0x0B, 0);
        } else {
            message(player, 0x31);
            spb(s, 0x0B, 1);
        }
        if (computer_plan(player)) {
            spw(s, 0, (pw(s, 0) & 0xFFBF) | 8);
            spb(AI_QUEUE(player), 0x46, 0);
        }
    } else if (pw(s, 0) & 8) {
        message(player, 0x2E);
        if (computer_hand_out(player)) {
            spw(s, 0, (pw(s, 0) & 0xFFF7) | 2);
            spb(AI_QUEUE(player), 0x46, 0);
        }
    }
    if (pw(s, 0) & 2) {
        message(player, 0x1D);
        if (command_step(player))
            spw(s, 0, pw(s, 0) | 8);
        else {
            spw(s, 0, pw(s, 0) | 1);
            spb(AI_SCRIPT(player), 0x1E, 0);
        }
        spw(s, 0, pw(s, 0) & 0xFFFD);
    }
    if (pw(s, 0) & 1) {
        message(player, 0x1D);
        if (script_step(player))
            spw(s, 0, (pw(s, 0) & 0xFFFE) | 2);
    }
}

static void clear_record(fptr r)
{
    spb(r, 8, 0);
    spw(r, 4, 0xFFFF);
    spw(r, 2, 0);
    spw(r, 6, 0);
}

/* the computer's state for a player, at a map's start */
void computer_start(int player)
{
    fptr s = AI_STATE(player);
    unsigned n;

    SWO(computer_keys, 4 * player, 0);
    message(player, 0x1C);
    spw(s, 0, 0x20);
    spb(s, 3, 0), spb(s, 4, 0), spb(s, 6, 0), spb(s, 5, 0), spb(s, 7, 0), spb(s, 8, 0), spb(s, 2, 0);
    spw(s, 9, 0);
    spb(s, 0x0B, 0);
    for (n = 0; n <= 0xF0; n++) {
        clear_record(AI_ATTACK(n));
        clear_record(AI_MOVE(n));
    }
    spb(AI_SCRIPT(player), 0, 0xFF);
    spb(AI_SCRIPT(player), 0x1E, 0);
    spb(AI_SCRIPT(player), 0x1F, 0);
    spb(AI_QUEUE(player), 0, 0xFF);
    spb(AI_QUEUE(player), 0x46, 0);
    spb(AI_QUEUE(player), 0x47, 0);
    spb(AI_QUEUE(player), 0x48, 0);
}

/* a unit made in a factory: its two records empty (T0708, in the
 * building's loop) */
void computer_unit_new(int unit)
{
    fptr r;
    int i;

    for (i = 0; i < 2; i++) {
        r = i ? AI_ATTACK(unit) : AI_MOVE(unit);
        spw(r, 4, 0xFFFF);
        spb(r, 8, 0);
        spw(r, 0, 0);
        spw(r, 2, 0);
        spw(r, 6, 0);
    }
}

/* ---- T1ABC ---- */

/* a unit's strength, three numbers (against land, sea, air): the type's
 * hit value * (count / 2 + 1) * (experience + 1) * (armour / 8) / 32 */
void unit_strength(int unit, fptr out)
{
    fptr u = UNIT(unit & 0xFF), t = TYPE(pb(u, 8));
    int32_t a = (pb(u, (pw(t, 0x0E) & 4) ? 3 : 2) >> 1) + 1, e = pb(u, 1) + 1, arm = pb(t, 1) >> 3;
    static const uint8_t which[3] = {0x0A, 0x0B, 9};
    int i;

    for (i = 0; i < 3; i++) {
        int32_t v = (int32_t)((uint32_t)pb(t, which[i]) * (uint32_t)a * (uint32_t)e * (uint32_t)arm);

        spw(out, (unsigned)(2 * i), (uint16_t)(v >> 5));
    }
}

/* the first unit of the side whose type has one of the .COM flags; FFh */
int find_kind(int mask, int side)
{
    unsigned n;

    for (n = 0; n <= 0xF0; n++) {
        unsigned f = pw(UNIT(n), 4);

        if (same_side((int)f, side) && !(f & 0x8000) && (ai_com(n) & (unsigned)mask))
            return (int)n;
    }
    return 0xFF;
}

/* a path for the unit to the square over the whole map: every square
 * whose ground the type can be on and that holds no unit (with flag 1:
 * no unit of the other side), or an own unit that holds others.  The
 * path is left in the player's list, its length in path_count. */
int can_go(int square, int unit, int player, int flags)
{
    fptr map = ai_map(player);
    unsigned bit = player ? 2 : 1, s, mask = pb(TYPE(pb(UNIT(unit & 0xFF), 8)), 4);
    int found;

    unit &= 0xFF;
    for (s = 0; (int16_t)GW(map_bytes) > (int16_t)s; s += 2) {
        unsigned there = pb(map, s + 1);
        int ok = (GBO(ground, 6 * pb(map, s) + 2) & mask) != 0;

        /* MOON.EXE leaves a square whose ground the type cannot be on at
         * that, BATTLE.EXE lets a unit of the own side that holds others
         * make it free all the same */
        if (there <= 0xF0 && (ok || bi_prog != BI_MOON)) {
            unsigned f = pw(UNIT(there), 4);

            if ((flags & 1) && !same_side(player, (int)f))
                ok = 0;
            if (same_side((int)f, player)) {
                /* MOON.EXE, with flag 2: a unit of the own side on the square
                 * makes it no way unless it is the moving unit's own */
                if (bi_prog == BI_MOON && (flags & 2)) {
                    if (pw(UNIT(unit), 0x0B + 2 * player) != s)
                        ok = 0;
                } else if (f & 0x1000)
                    ok = 1;
            }
        }
        if (ok)
            SBO(marks, ai_mark(s), GBO(marks, ai_mark(s)) | bit);
    }
    find_path(ai_paths(player), unit, (int)ai_place((unsigned)unit, player), square, player, map);
    found = GW(path_count) != 0;
    clear_marks((int)bit);
    return found;
}

/* ---- T1B01 ---- */

static void command(int player, unsigned kind, unsigned unit)
{
    spb(COMMAND, 0, kind);
    spb(COMMAND, 1, unit);
    command_add(player, COMMAND);
}

static void command_to(int player, unsigned kind, unsigned unit, unsigned where)
{
    spb(COMMAND, 0, kind);
    spb(COMMAND, 1, unit);
    spw(COMMAND, 3, where);
    command_add(player, COMMAND);
}

static void command_at(int player, unsigned kind, unsigned unit, unsigned what)
{
    spb(COMMAND, 0, kind);
    spb(COMMAND, 1, unit);
    spb(COMMAND, 3, what);
    command_add(player, COMMAND);
}

static void reach_for(unsigned n, int player, unsigned flags)
{
    reach((int)ai_place(n, player), ai_paths(player), (int)n, pb(UNIT(n), 0), player, (int)flags, ai_map(player));
}

/* task 7, a unit to a unit: the first free square in reach around that
 * unit, tried from the way that unit faces: the command to move there,
 * and for the attack phase the task 2 with an enemy beside that square
 * the type can fire at; no square: the task 1 with the unit's square.
 * 1 when it is through. */
int task_approach(int unit, int player)
{
    fptr s = AI_STATE(player), table = ai_table(player), map = ai_map(player), rec;
    unsigned n = ai_first_half((unsigned)unit & 0xFF), bit = player ? 2 : 1, i, d = 0, aim, pos, way, sq;

    rec = ai_rec(table, n);
    switch (pb(s, 7)) {
    case 0:
        if (!(GW(game_flags2) & 0x80))
            spb(s, 7, pb(s, 7) + 1);
        return 0;
    case 1:
        reach_for(n, player, (pw(UNIT(n), 4) & 1) ? 6 : 3);
        spb(s, 7, pb(s, 7) + 1);
        return 0;
    case 2:
        break;
    default:
        return 0;
    }
    aim = pw(rec, 0);
    pos = (uint16_t)(pw(UNIT(aim), 0x0B + 2 * player) - 1);
    way = pb(UNIT(aim), 0x0F + player);
    neighbours((int)pos);
    for (i = 0; i < 6; i++) {
        int free;

        d = ai_mod6(way + GBO(approach_turns, i));
        sq = GWO(around, 2 * d);
        if (sq == 0xFFFF || !(GBO(marks, ai_mark(sq)) & bit))
            continue;
        free = pb(map, sq + 1) > 0xF0;
        if ((pw(UNIT(n), 4) & 0x10) && (GWO(ground, 6 * pb(map, sq)) & 0x4000))
            free = 0;
        if (free)
            break;
    }
    if (i < 6) {
        unsigned targets;

        command_to(player, 1, n, GWO(around, 2 * d));
        spb(rec, 8, 0);
        spw(rec, 4, 0xFFFF);
        spw(rec, 2, 1);
        neighbours(GWO(around, 2 * d));
        for (i = 0; i < 6; i++) {
            unsigned there, f;

            d = ai_mod6(way + GBO(approach_turns, i));
            sq = GWO(around, 2 * d);
            if (sq == 0xFFFF)
                continue;
            there = pb(map, sq + 1);
            if (there > 0xF0)
                continue;
            f = pw(UNIT(there), 4);
            targets = pw(TYPE(pb(UNIT(n), 8)), 5) & 0x3C;
            if (same_side(player, (int)f) || !(targets & f))
                continue;
            spb(AI_ATTACK(n), 8, 2);
            spw(AI_ATTACK(n), 0, sq);
            spw(AI_ATTACK(n), 4, pw(ai_rec(table, there), 6));
        }
    } else {
        spb(rec, 8, 1);
        spw(rec, 0, pos);
    }
    clear_marks((int)bit);
    spb(s, 7, 0);
    return 1;
}

/* T1B01:04C6, in the move phase: the units in the side's buildings get
 * the task 5 (out of the building), score 7 */
static void tasks_out(fptr table, int player, int count)
{
    int k, i;

    for (k = 0; k < count; k++) {
        fptr r = MKFP(FSEG(table), FOFF(table) + 0x1C * k);

        if ((pw(r, 0x19) & 0x8002) || !same_side(pw(r, 0x19), player))
            continue;
        for (i = 0; i < 7; i++) {
            unsigned v = pb(r, (unsigned)(7 * player + i));

            if (v > 0xF0 || (pw(UNIT(v), 6) & 0x24))
                continue;
            spb(AI_MOVE(v), 8, 5);
            spw(AI_MOVE(v), 4, 7);
        }
    }
}

/* T1B01:05B9, in the attack phase: the units that are not whole, in a
 * building of the side with more than 2 of energy, get the task 6 (a
 * repair), score 7 */
static void tasks_repair(fptr table, int player, int count)
{
    int k, i;

    for (k = 0; k < count; k++) {
        fptr r = MKFP(FSEG(table), FOFF(table) + 0x1C * k);

        if ((pw(r, 0x19) & 0x8002) || !same_side(pw(r, 0x19), player) || pb(r, 0x16 + player) <= 2)
            continue;
        for (i = 0; i < 7; i++) {
            unsigned v = pb(r, (unsigned)(7 * player + i));

            if (v > 0xF0 || (pw(UNIT(v), 6) & 0x24))
                continue;
            if (pb(UNIT(v), 2) >= pb(TYPE(pb(UNIT(v), 8)), 2))
                continue;
            spb(AI_ATTACK(v), 8, 6);
            spw(AI_ATTACK(v), 4, 7);
        }
    }
}

/* a unit towards its aim: a path over the whole map (first without the
 * squares of enemy units), the unit's reach and the squares it can stop
 * on; the square of the path nearest to the aim that is one of those
 * (no building's, no unit that holds others, unless it is the aim): the
 * command to move there.  1 when it is through. */
int task_move(int unit, int player)
{
    fptr s = AI_STATE(player), table = ai_table(player), map = ai_map(player), rec, path, stops, u;
    unsigned n = ai_first_half((unsigned)unit & 0xFF), bit = player ? 2 : 1;
    int i, d, stage;

    rec = ai_rec(table, n);
    u = UNIT(n);
    /* MOON.EXE has a stage more (2, a path with flag 2 after the one with 3):
     * its numbers are used here, BATTLE.EXE's from its stage 2 on are one less */
    stage = pb(s, 7);
    if (bi_prog != BI_MOON && stage >= 2)
        stage++;
    switch (stage) {
    case 0:
        if (!(GW(game_flags2) & 0x80))
            spb(s, 7, pb(s, 7) + 1);
        break;
    case 1:
        if (can_go(pw(rec, 0), (int)n, player, bi_prog == BI_MOON ? 3 : 1))
            spb(s, 7, bi_prog == BI_MOON ? 4 : 3);
        else
            spb(s, 7, pb(s, 7) + 1);
        break;
    case 2:
        spb(s, 7, can_go(pw(rec, 0), (int)n, player, 2) ? 4 : 3);
        break;
    case 3:
        can_go(pw(rec, 0), (int)n, player, 0);
        spb(s, 7, pb(s, 7) + 1);
        break;
    case 4:
        path = ai_aims(player);
        stops = MKFP(FSEG(ai_paths(player)), FOFF(ai_paths(player)) + 0x1F40);
        SW(task_path_count, GW(path_count));
        for (i = 0; (int16_t)GW(path_count) > i; i++)
            spw(path, (unsigned)(2 * i), pw(stops, (unsigned)(2 * i)));
        spb(s, 7, pb(s, 7) + 1);
        break;
    case 5:
        reach_for(n, player, (pw(u, 6) & 1) ? 0xFFFF : (pw(u, 4) & 1) ? 6 : 3);
        spb(s, 7, pb(s, 7) + 1);
        break;
    case 6:
        list_reach(ai_paths(player), player, (int)n);
        clear_marks((int)bit);
        spb(s, 7, pb(s, 7) + 1);
        break;
    case 7:
        path = ai_aims(player);
        stops = ai_paths(player);
        spb(s, 7, 0);
        for (d = 0; d < (int16_t)GW(task_path_count); d++)
            for (i = 0; (int16_t)GW(path_count) > i; i++) {
                unsigned sq = pw(stops, (unsigned)(2 * i));
                int ok = 1;

                if (sq != pw(path, (unsigned)(2 * d)))
                    continue;
                if (pw(rec, 0) != sq) {
                    unsigned there = pb(map, sq + 1);

                    if (GWO(ground, 6 * pb(map, sq)) & 0x540)
                        ok = 0;
                    if (there <= 0xF0 && (pw(UNIT(there), 4) & 0x1000))
                        ok = 0;
                }
                if (ok) {
                    command_to(player, 1, n, sq);
                    spb(s, 7, 0);
                    return 1;
                }
            }
        return 1;
    }
    return 0;
}

/* the squares of the side's buildings with more than 2 of energy, onto
 * the list of aims (counted in path_count) */
void list_buildings(fptr table, int player, int count)
{
    fptr list = ai_aims(player);
    int k;

    for (k = 0; k < count; k++) {
        fptr r = MKFP(FSEG(table), FOFF(table) + 0x1C * k);

        if ((pw(r, 0x19) & 0x8002) || !same_side(pw(r, 0x19), player) || pb(r, 0x16 + player) <= 2)
            continue;
        spw(list, 2 * GW(path_count), pw(r, 0x0E));
        SW(path_count, GW(path_count) + 1);
    }
}

/* 1 when more than three of the six squares around hold units of the
 * other side, or two opposite ones do (`around` is left as it was) */
int square_crowded(int square, int player)
{
    fptr map = ai_map(player);
    int other = player ? 0 : 1, result = 0, held = 0;
    unsigned kept[6], i;

    for (i = 0; i < 6; i++)
        kept[i] = GWO(around, 2 * i);
    neighbours(square);
    for (i = 0; i < 6; i++) {
        unsigned sq = GWO(around, 2 * i), there;

        SWO(around, 2 * i, 0);
        if (sq == 0xFFFF)
            continue;
        there = pb(map, sq + 1);
        if (there <= 0xF0 && same_side(other, pw(UNIT(there), 4))) {
            SWO(around, 2 * i, 1);
            held++;
        }
    }
    if (held > 3)
        result = 1;
    else
        for (i = 0; i < 6; i++)
            if (GWO(around, 2 * i) && GWO(around, 2 * ai_mod6(i + 3)))
                result = 1;
    for (i = 0; i < 6; i++)
        SWO(around, 2 * i, kept[i]);
    return result;
}

/* T1B01:0DE3: is the unit one of the six chosen (the paths' list +28h) */
int t1b01_0de3(int unit, int player)
{
    fptr p = ai_paths(player);
    unsigned i;

    for (i = 0; i < 6; i++)
        if (pb(p, 0x28 + i) == (unsigned)(unit & 0xFF))
            return 1;
    return 0;
}

/* task 0Ah for a unit that holds a unit of nobody: the aim the nearest
 * own factory it can reach, or the own headquarters */
void task_home(int player, int unit)
{
    fptr table = ai_table(player);
    int best = 0x3E8, which = -1, w = (int16_t)GW(map_width), k;
    unsigned sq;

    unit &= 0xFF;
    for (k = 0; k < 10; k++) {
        unsigned f = pw(AI_BUILDING(factories, k), 0x19);
        int p, d;

        if ((f & 0x8000) || !same_side((int)f, player))
            continue;
        sq = pw(AI_BUILDING(factories, k), 0x0E);
        if (!can_go((int)sq, unit, player, 1))
            continue;
        /* the offsets as they are, not halved */
        p = (int16_t)ai_place((unsigned)unit, player);
        d = square_distance((int16_t)sq % w, (int16_t)sq / w, p % w, p / w);
        if (d < best) {
            best = d;
            which = k;
        }
    }
    sq = which >= 0 ? pw(AI_BUILDING(factories, which), 0x0E) : pw(AI_BUILDING(hqs, player), 0x0E);
    spb(ai_rec(table, (unsigned)unit), 8, 0x0A);
    spw(ai_rec(table, (unsigned)unit), 4, 0x190);
    spw(ai_rec(table, (unsigned)unit), 0, sq);
    spb(AI_ATTACK(unit), 8, 0);
    spw(AI_ATTACK(unit), 4, 0xFFFF);
}

/* ---- T17C0 ---- */

/* the stage "assess", a step a call (the record's +5); 1 when through */
int computer_assess(int player)
{
    fptr s = AI_STATE(player), table = ai_table(player), map = ai_map(player), list;
    int other = player ? 0 : 1, w = (int16_t)GW(map_width);
    unsigned n, i, k;

    bi_seen("assess", pb(s, 5));
    switch (pb(s, 5)) {
    case 0: {
        int32_t own = 0, enemy = 0;

        spb(s, 7, 0);
        if (pb(CURSOR(player), 0x16) == 2) {
            spb(s, 5, 5);
            return 0;
        }
        for (n = 0; n <= 0xF0; n++) {
            unsigned f = pw(UNIT(n), 4);
            fptr r = ai_rec(table, n);
            int32_t total;

            if (f & 0x8002)
                continue;
            unit_strength((int)n, FP(strength_values));
            total = (int16_t)GWO(strength_values, 4) + (int16_t)GWO(strength_values, 0)
                    + (int16_t)GWO(strength_values, 2);
            if (!same_side(player, (int)f)) {
                spw(r, 6, 0xFFFF);
                spw(r, 2, 0xFFFF);
                enemy += total;
                continue;
            }
            own += total;
            if (pb(r, 8) == 0x0B)
                continue;
            if (pb(r, 8) == 9) {
                /* on its way to a building for a repair: stays so while
                 * one can be reached */
                int found = 0, left;

                list = ai_aims(player);
                SW(path_count, 0);
                list_buildings(FP(hqs), player, 2);
                list_buildings(FP(factories), player, 10);
                list_buildings(FP(depots), player, 10);
                for (left = (int16_t)GW(path_count); left > 0 && !found;) {
                    left--;
                    found = can_go(pw(list, (unsigned)(2 * left)), (int)n, player, 0);
                }
                if (found)
                    continue;
            }
            if (f & 0x1000) {
                fptr c = AI_BUILDING(cargo, (int8_t)pb(UNIT(n), 0x0A));
                int holds = 0;

                for (i = 0; i < 7; i++) {
                    unsigned v = pb(c, (unsigned)(7 * player) + i);

                    if (v <= 0xF0 && (pw(UNIT(v), 6) & 4))
                        holds = 1;
                }
                if (holds) {
                    task_home(player, (int)n);
                    continue;
                }
            }
            spw(r, 4, 0xFFFF);
            spb(r, 8, 0);
            spw(r, 0, 0);
            spw(AI_ATTACK(n), 4, 0xFFFF);
            spb(AI_ATTACK(n), 8, 0);
            spw(AI_ATTACK(n), 0, 0);
        }
        spw(s, 9, 2 * own <= enemy ? 3 : own < enemy ? 2 : 1);
        break;
    }
    case 1: {
        /* every enemy unit's threat */
        int hq = (int16_t)pw(AI_BUILDING(hqs, player), 0x0E) >> 1, hx = hq % w, hy = hq / w;

        for (n = 0; n <= 0xF0; n++) {
            fptr u = UNIT(n), t;
            unsigned f = pw(u, 4);
            int v;

            if (((f & 0x40) && !(f & 0x80)) || !same_side(other, (int)f) || (f & 0x8000))
                continue;
            t = TYPE(pb(u, 8));
            v = (int8_t)pb(AI_COM(pb(u, 8)), 0);
            if (pw(t, 5) & 0xC0)
                v += 0x32;
            /* MOON (T18B8:0FF9): not for a type with 20h in its +10h;
             * one that holds others adds the held units' values, from
             * the slots of the assessing player (as the original) */
            if (bi_prog != BI_MOON || !(pw(t, 0x10) & 0x20)) {
                if (!pb(t, 9))
                    v += 0x19;
                if (!pb(t, 0x0B))
                    v += 0x19;
                if (!pb(t, 0x0A))
                    v += 0x19;
            }
            if (bi_prog == BI_MOON && (f & 0x1000)) {
                fptr c = AI_BUILDING(cargo, (int8_t)pb(u, 0x0A));

                for (i = 0; i < 7; i++) {
                    unsigned held = pb(c, (unsigned)(7 * player) + i);

                    if (held <= 0xF0)
                        v += (int8_t)pb(AI_COM(pb(UNIT(held), 8)), 0);
                }
            }
            v += (6 - pb(u, 1)) * 10;
            if (ai_com(n) & 1) {
                /* one that takes buildings, nearer to the headquarters
                 * than its move */
                int p = (int16_t)ai_place(n, player) >> 1, move;

                if (f & 0x4000)
                    move = pb(TYPE(pb(UNIT(pb(AI_BUILDING(cargo, (int8_t)pb(u, 0x0A)), 0x1B)), 8)), 0);
                else
                    move = pb(t, 0);
                if (square_distance(hx, hy, p % w, p / w) < move) {
                    v += 0x12C;
                    spw(s, 9, pw(s, 9) + 2);
                    /* the square's number, not its offset, as the original */
                    neighbours(p);
                    for (i = 0; i < 6; i++) {
                        unsigned sq = GWO(around, 2 * i), there;

                        if ((int16_t)sq < 0)
                            continue;
                        there = pb(map, sq + 1);
                        if (there <= 0xF0 && same_side(other, pw(UNIT(there), 4)))
                            spw(ai_rec(table, there), 6, pw(ai_rec(table, there), 6) + 0x12C);
                    }
                }
            }
            spw(ai_rec(table, n), 6, pw(ai_rec(table, n), 6) + (unsigned)v);
        }
        break;
    }
    case 2:
        /* the buildings to take */
        if (find_kind(1, player) > 0xF0)
            break;
        ai_add_aim(player, pb(AI_BUILDING(hqs, other), 0x18) * 10u, pw(AI_BUILDING(hqs, other), 0x0E), 1);
        for (k = 0; k < 20; k++) {
            fptr r = k < 10 ? AI_BUILDING(factories, k) : AI_BUILDING(depots, k - 10);
            unsigned v;

            if ((pw(r, 0x19) & 0x8000) || same_side(pw(r, 0x19), player))
                continue;
            v = pb(r, 0x18) * 10u;
            for (i = 0; i < 7; i++)
                if (pb(r, (unsigned)(7 * player) + i) <= 0xF0)
                    v += 0x14;
            if (k < 10 && pb(r, 0x16 + player) >= 0x0A)
                v += (pb(r, 0x16 + player) / 10u) * 0x14;
            ai_add_aim(player, v, pw(r, 0x0E), 1);
        }
        break;
    case 3:
        /* squares around the own headquarters to hold */
        if (find_kind(1, other) > 0xF0)
            break;
        neighbours(pw(AI_BUILDING(hqs, player), 0x0E));
        if ((int16_t)pw(s, 9) > 6)
            spw(s, 9, 6);
        if (pb(AI_PLAYER(player), 2) < 0x0A && (int16_t)pw(s, 9) > 1)
            spw(s, 9, 1);
        for (k = pb(s, 9), i = 0; k > 0; k--, i++)
            ai_add_aim(player, 0x78 + 0x1E * k, GWO(around, 2 * (int8_t)GBO(computer_hq_order, i)), 3);
        break;
    case 4:
        /* the units of nobody to fetch */
        if (find_kind(0x280, player) > 0xF0)
            break;
        for (n = 0; n <= 0xF0; n++)
            if (!(pw(UNIT(n), 4) & 0x8000) && (pw(UNIT(n), 6) & 4) && (pw(UNIT(n), 4) & 2))
                ai_add_aim(player, 0xC8, n, 4);
        break;
    default:
        spb(s, 5, 0);
        return 1;
    }
    spb(s, 5, pb(s, 5) + 1);
    return 0;
}

static int own(unsigned n, int player)
{
    unsigned f = pw(UNIT(n), 4);

    return !(f & 0x8000) && same_side((int)f, player);
}

static void done(fptr rec, int keep_score)
{
    spb(rec, 8, 0);
    if (!keep_score)
        spw(rec, 4, 0xFFFF);
    spw(rec, 2, 1);
}

/* the unit fires at what stands on its aim's square */
static int hand_fire(int player, fptr table, unsigned n, unsigned back)
{
    unsigned there = pb(ai_map(player), pw(ai_rec(table, n), 0) + 1);

    done(ai_rec(table, n), 0);
    spb(AI_STATE(player), 3, back);
    if ((pw(UNIT(there), 4) & 0x8000) || there > 0xF0)
        return 0;
    command_at(player, 2, n, there);
    return 1;
}

/* the stage "hand out", a step a call (the record's +3): 1 after each
 * command it puts into the queue */
int computer_hand_out(int player)
{
    fptr s = AI_STATE(player), table = ai_table(player), rec;
    unsigned n, k, i, task;
    int mode = pb(CURSOR(player), 0x16), best;

    switch (pb(s, 3)) {
    case 0:
        for (n = 0; n <= 0xF0; n++)
            if (own(n, player))
                spw(ai_rec(table, n), 2, 0xFFFF);
        spb(s, 7, 0);
        spw(s, 9, 0);
        spb(s, 3, pb(s, 3) + 1);
        break;
    case 1:
        if (mode == 1) {
            tasks_out(FP(depots), player, 10);
            tasks_out(FP(factories), player, 10);
            tasks_out(FP(hqs), player, 2);
        } else {
            tasks_repair(FP(depots), player, 10);
            tasks_repair(FP(factories), player, 10);
            tasks_repair(FP(hqs), player, 2);
        }
        spb(s, 3, pb(s, 3) + 1);
        break;
    case 2:
        /* the next unit with a task, in the units' order */
        for (n = pb(s, 9); n <= 0xF0; n++)
            if (own(n, player) && pb(ai_rec(table, n), 8)) {
                spw(s, 9, n);
                break;
            }
        spb(s, 3, n <= 0xF0 ? pb(s, 3) + 1 : 4);
        break;
    case 3:
        n = pw(s, 9);
        rec = ai_rec(table, n);
        bi_seen("task", pb(rec, 8));
        switch (pb(rec, 8)) {
        case 3:
            command_to(player, 1, n, pw(rec, 0));
            done(rec, 1);
            spb(s, 3, 2);
            return 1;
        case 4:
            done(rec, 1);
            spb(s, 3, 2);
            break;
        case 0x0C:
            if (task_move((int)n, player)) {
                done(rec, 0);
                spb(s, 3, 2);
                return 1;
            }
            break;
        case 0x0E:
            return hand_fire(player, table, n, 2);
        default:
            spb(s, 3, 2);
            spw(s, 9, pw(s, 9) + 1);
            break;
        }
        break;
    case 4:
        /* then the unit not handled with the highest score */
        best = -1;
        spw(s, 9, 0xFFFF);
        for (n = 0; n <= 0xF0; n++) {
            rec = ai_rec(table, n);
            if ((int16_t)pw(rec, 4) > best && !(pw(UNIT(n), 4) & 0x8200) && same_side(pw(UNIT(n), 4), player)
                && pb(rec, 8) && (int16_t)pw(rec, 2) < 0) {
                best = (int16_t)pw(rec, 4);
                spw(s, 9, n);
            }
        }
        spb(s, 3, (int16_t)pw(s, 9) >= 0 ? pb(s, 3) + 1 : 6);
        break;
    case 5:
        n = pw(s, 9);
        rec = ai_rec(table, n);
        task = pb(rec, 8);
        bi_seen("task", (int)task);
        switch (task) {
        case 1:
        case 0x0D:
            if (task_move((int)n, player)) {
                done(rec, 0);
                spb(s, 3, 4);
                return 1;
            }
            break;
        case 2:
            return hand_fire(player, table, n, 4);
        case 5:
        case 6:
            command(player, task == 5 ? 4 : 6, n);
            done(rec, 0);
            spb(s, 3, 4);
            return 1;
        case 7:
            if (task_approach((int)n, player)) {
                spb(s, 3, 4);
                return 1;
            }
            break;
        case 9:
        case 0x0A:
        case 0x0B:
            if (task_move((int)n, player)) {
                spw(rec, 2, 1);
                spb(s, 3, 4);
                return 1;
            }
            break;
        default:
            done(rec, 0);
            spb(s, 3, 4);
            break;
        }
        break;
    case 6: {
        /* in the attack phase: units made in the own factories */
        unsigned made = 0;

        if (mode == 1) {
            spb(s, 3, pb(s, 3) + 1);
            break;
        }
        for (k = 0; k < 10; k++) {
            fptr r = AI_BUILDING(factories, k);
            int energy = pb(r, 0x16 + player);

            if ((pw(r, 0x19) & 0x8002) || !same_side(pw(r, 0x19), player) || energy <= 9)
                continue;
            for (i = 0; i < 7; i++) {
                unsigned pick = 0xFF, j;

                if (pb(r, (unsigned)(7 * player) + i) <= 0xF0)
                    continue;
                if (energy - 9 <= 0)
                    break;
                list_makeable((energy - 9) & 0xFF);
                if (!GB(types_listed) || made >= 10)
                    break;
                for (j = 0; GB(types_listed) > j; j++)
                    if ((pw(AI_COM(GBO(types_list, j)), 2) & 0x400) && bi_random(0, 0x3E8) > 0x1F4) {
                        pick = j;
                        break;
                    }
                if (pick == 0xFF)
                    continue;
                /* the cost of the type numbered as the list's place, as
                 * the original has it */
                energy -= pb(TYPE(pick), 0x3E);
                made++;
                command_at(player, 7, pick, k);
            }
        }
        spb(s, 3, pb(s, 3) + 1);
        break;
    }
    case 7:
        spb(COMMAND, 0, 3);
        command_add(player, COMMAND);
        spb(s, 3, pb(s, 3) + 1);
        return 1;
    default:
        spw(s, 0, (pw(s, 0) & 0xFFF7) | 0x20);
        spb(s, 3, 0);
        break;
    }
    return 0;
}
