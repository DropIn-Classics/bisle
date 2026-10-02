/* plan.c - BATTLE.EXE's T1C04 and T1ED2: the computer player's stage
 * "plan": a task for every unit, in the move phase.  A step a call (the
 * record's +4), most in steps of their own (+7): the aims of the list,
 * the best first, kind after kind (1 a building to take, 2 a unit to go
 * with, 3 a square by the own headquarters, 4 a unit of nobody to
 * fetch); the weakened units to a building; what each enemy cannot fire
 * at; the units that fire from afar; units around the enemy that
 * threatens most; the units without a task.  tools/computer.py has the
 * rules in full.
 *
 * The tasks (a unit's record +8): 1 to a square, 2 fire at the unit on
 * a square, 3 to a square beside an enemy, 4 stay, 5 out of the
 * building, 6 a repair, 7 to a unit, 9 to a building for a repair, 0Ah
 * home with a unit of nobody, 0Bh to the headquarters, 0Ch to a unit
 * that helps, 0Dh fetch a unit, 0Eh fire from afar.
 */
#include "ai.h"

/* a unit that cannot go by itself: T1ED2:0005, in steps (the record's
 * +8).  A unit with 4 in its +6 is not moved itself.  Else its reach
 * (all ground allowed) and the squares it can stop on: one that holds an
 * own unit with the task: the unit goes there (task 0Ch) and that one's
 * score rises; one that holds an own unit without a task: the same, and
 * that one gets the task, the score and the square.  Else the first unit
 * that holds others, of a type with the .COM flags 80h..200h, with less
 * than the score, not the tasks 9, 0Ah, 0Bh, 0Dh, that may take the unit
 * in, gets the task 0Dh with the unit's square if it can go there.  1
 * when it is through. */
static int task_helper(int player, unsigned unit, unsigned score, unsigned task, unsigned square)
{
    fptr s = AI_STATE(player), table = ai_table(player), map = ai_map(player), list;
    unsigned bit = player ? 2 : 1, k, sq;
    int w = (int16_t)GW(map_width), i;

    unit &= 0xFF;
    task &= 0xFF;
    bi_seen("helper", pb(s, 8));
    switch (pb(s, 8)) {
    case 0:
        SW(helper_unit, 0);
        spb(s, 8, (pw(UNIT(unit), 6) & 4) ? 5 : pb(s, 8) + 1);
        break;
    case 1:
        reach((int)ai_place(unit, player), ai_paths(player), (int)unit, pb(UNIT(unit), 0), player, 0, map);
        spb(s, 8, pb(s, 8) + 1);
        break;
    case 2:
        list_reach(ai_paths(player), player, (int)unit);
        clear_marks((int)bit);
        spb(s, 8, pb(s, 8) + 1);
        break;
    case 3:
    case 4:
        list = ai_paths(player);
        for (i = 0; (int16_t)GW(path_count) > i; i++) {
            unsigned there;

            sq = pw(list, (unsigned)(2 * i));
            there = pb(map, sq + 1);
            if (there > 0xF0 || pb(ai_rec(table, there), 8) != (pb(s, 8) == 3 ? task : 0))
                continue;
            ai_give(table, unit, score, sq, 0x0C);
            if (pb(s, 8) == 3)
                spw(ai_rec(table, there), 4, pw(ai_rec(table, there), 4) + score);
            else
                ai_give(table, there, score, square, task);
            spb(s, 8, 0);
            return 1;
        }
        spb(s, 8, pb(s, 8) + 1);
        break;
    case 5:
        for (k = GB(helper_unit); k <= 0xF0; k++) {
            unsigned f = pw(UNIT(k), 4), t;
            int a, b;

            if ((f & 0xC000) || !(f & 0x1000) || (int16_t)pw(ai_rec(table, k), 4) >= (int16_t)score
                || !same_side(player, (int)f))
                continue;
            t = pb(ai_rec(table, k), 8);
            if (!(ai_com(k) & 0x380) || t == 9 || t == 0x0A || t == 0x0D || t == 0x0B)
                continue;
            if (!(pw(UNIT(unit), 6) & pw(AI_BUILDING(cargo, (int8_t)pb(UNIT(k), 0x0A)), 0x19) & 0x0FC0))
                continue;
            a = (int16_t)ai_place(unit, player) >> 1;
            b = (int16_t)ai_place(k, player) >> 1;
            if (square_distance(a % w, a / w, b % w, b / w) < 0x3E8) {
                SW(helper_unit, k);
                break;
            }
        }
        spb(s, 8, k <= 0xF0 ? pb(s, 8) + 1 : 7);
        break;
    case 6:
        sq = ai_place(unit, player);
        if (can_go((int)sq, GB(helper_unit), player, 0)) {
            ai_give(table, GW(helper_unit), score, sq, 0x0D);
            spb(s, 8, 7);
        } else {
            spb(s, 8, 5);
            SW(helper_unit, GW(helper_unit) + 1);
        }
        break;
    default:
        spb(s, 8, 0);
        return 1;
    }
    return 0;
}

static int half(unsigned n) { return (pw(UNIT(n), 4) & 0x40) && !(pw(UNIT(n), 4) & 0x80); }

/* MOON.EXE's T1BCB:0001 (a unit, the enemy): a unit that has more than 2
 * fewer than the enemy (the count a type with 4 in its +0Eh keeps in +3,
 * else in +2) and less than 3 in its +1 is left out of the units that go
 * for the enemy */
static int much_weaker(unsigned a, unsigned b)
{
    fptr ua = UNIT(a), ub = UNIT(b);
    int ca = pb(ua, (pw(TYPE(pb(ua, 8)), 0x0E) & 4) ? 3 : 2);
    int cb = pb(ub, (pw(TYPE(pb(ub, 8)), 0x0E) & 4) ? 3 : 2);

    return ca - cb < -2 && pb(ua, 1) < 3;
}

/* the distance of two squares given as offsets */
static int distance(unsigned a, unsigned b)
{
    int w = (int16_t)GW(map_width), p = (int16_t)a >> 1, q = (int16_t)b >> 1;

    return square_distance(p % w, p / w, q % w, q / w);
}

/* the stage "plan"; 1 when it is through */
int computer_plan(int player)
{
    fptr s = AI_STATE(player), list = ai_aims(player), table = ai_table(player), map = ai_map(player), aim, p;
    int other = player ? 0 : 1, w = (int16_t)GW(map_width), best, who, i, x, y;
    unsigned sub = pb(s, 7), n, u, k, j, sq, f, bit, foe;

#define T(n) ai_rec(table, n)
#define SCORE(n) ((int16_t)pw(T(n), 4))
#define FLAGS(n) pw(UNIT(n), 4)
#define SUB(v) spb(s, 7, v)
#define STEP(v) spb(s, 4, v)
#define NEXT_SUB spb(s, 7, pb(s, 7) + 1)
#define NEXT_STEP (spb(s, 7, 0), spb(s, 4, pb(s, 4) + 1))
#define CUR GB(plan_unit)
#define NEXT_UNIT SB(plan_unit, GB(plan_unit) + 1)
/* an aim is done: its kind 0, back to the aims */
#define AIM_DONE (spb(s, 7, 0), spb(s, 4, 1), spb(aim, 5, 0))

    aim = MKFP(FSEG(list), FOFF(list) + 6 * GW(plan_aim));
    bi_seen("plan", pb(s, 4) << 4 | (sub & 0x0F));
    switch (pb(s, 4)) {
    case 0:
        if (pb(CURSOR(player), 0x16) == 2)
            STEP(0x0C);
        else {
            STEP(pb(s, 4) + 1);
            SW(plan_kind, 1);
        }
        break;
    case 1:
        /* the best aim of the kind; none: the next kind */
        if (!pb(s, 6) || (int16_t)GW(plan_kind) >= 6) {
            STEP(7);
            break;
        }
        best = 0;
        SW(plan_aim, 0xFFFF);
        for (i = 0; pb(s, 6) > i; i++)
            if ((int16_t)pw(list, (unsigned)(6 * i)) > best && pb(list, (unsigned)(6 * i + 5)) == GW(plan_kind)) {
                best = (int16_t)pw(list, (unsigned)(6 * i));
                SW(plan_aim, i);
            }
        if ((int16_t)GW(plan_aim) < 0) {
            SW(plan_kind, GW(plan_kind) + 1);
            break;
        }
        k = pb(list, 6 * GW(plan_aim) + 5);
        STEP(k == 1 ? 3 : k == 2 ? 4 : k == 3 ? 5 : k == 4 ? 6 : 2);
        break;
    case 2:
        STEP(1);
        spb(aim, 5, 0);
        break;
    case 3:
        /* a building to take: of the own units that take buildings the
         * one with the shortest path, if the aim's score less the path
         * is more than it has */
        switch (sub) {
        case 0:
            SB(plan_unit, 0);
            NEXT_SUB;
            break;
        case 1:
            for (; CUR <= 0xF0; NEXT_UNIT) {
                n = CUR;
                if ((ai_com(n) & 1) && !(FLAGS(n) & 0xC040) && same_side(FLAGS(n), player)
                    && SCORE(n) < (int16_t)pw(aim, 0))
                    break;
            }
            SUB(CUR <= 0xF0 ? sub + 1 : 3);
            break;
        case 2:
            spw(T(CUR), 2, can_go(pw(aim, 2), CUR, player, 0) ? GW(path_count) : 0x3E8);
            SUB(1);
            NEXT_UNIT;
            break;
        case 3:
            best = 0x3E8;
            who = -1;
            for (n = 0; n <= 0xF0; n++)
                if ((ai_com(n) & 1) && !(FLAGS(n) & 0xC040) && same_side(FLAGS(n), player)
                    && SCORE(n) < (int16_t)pw(aim, 0) && (int16_t)pw(T(n), 2) < best
                    && (int16_t)(pw(aim, 0) - pw(T(n), 2)) > SCORE(n)) {
                    best = (int16_t)pw(T(n), 2);
                    who = (int)n;
                }
            if (who >= 0) {
                unsigned left = (uint16_t)(pw(aim, 0) - pw(T(who), 2));

                ai_give(table, (unsigned)who, left, pw(aim, 2), 1);
                /* one that others should go with */
                if (ai_com((unsigned)who) & 0x20)
                    ai_add_aim(player, left - 10, (unsigned)who, 2);
            }
            NEXT_SUB;
            break;
        default:
            AIM_DONE;
            break;
        }
        break;
    case 4:
        /* units to go with a unit: as many as its type's .COM +4 says,
         * of types whose flags meet the unit's class, the nearest first */
        switch (sub) {
        case 0:
            SB(plan_unit, 0);
            NEXT_SUB;
            break;
        case 1:
            for (; CUR <= 0xF0; NEXT_UNIT) {
                n = CUR;
                if (!half(n) && !(FLAGS(n) & 0xC000) && same_side(FLAGS(n), player)
                    && SCORE(n) < (int16_t)pw(aim, 0) && (ai_com(n) & FLAGS(pw(aim, 2)) & 0x1C))
                    break;
            }
            SUB(CUR <= 0xF0 ? sub + 1 : 3);
            break;
        case 2:
            spw(T(CUR), 2, can_go((int)ai_place(pw(aim, 2), player), CUR, player, 0) ? GW(path_count) : 0x3E8);
            SUB(1);
            NEXT_UNIT;
            break;
        case 3:
            u = pb(aim, 2);
            for (k = 0; k < pb(AI_COM(pb(UNIT(u), 8)), 4); k++) {
                best = 0x3E8;
                who = -1;
                for (n = 0; n <= 0xF0; n++)
                    if (!half(n) && !(FLAGS(n) & 0xC000) && same_side(FLAGS(n), player)
                        && SCORE(n) < (int16_t)pw(aim, 0) && (ai_com(n) & FLAGS(u) & 0x1C)
                        && (int16_t)pw(T(n), 2) < best) {
                        best = (int16_t)pw(T(n), 2);
                        who = (int)n;
                    }
                if (who >= 0) {
                    int now = (int16_t)pw(aim, 0);

                    ai_give(table, (unsigned)who, (unsigned)now, u, 7);
                    spw(aim, 0, now - now / 3);
                }
            }
            NEXT_SUB;
            break;
        default:
            AIM_DONE;
            break;
        }
        break;
    case 5:
        /* a square by the own headquarters: a unit on it that holds
         * ground stays, else the nearest such unit goes there */
        sq = pw(aim, 2);
        switch (sub) {
        case 0:
            SB(plan_unit, 0);
            NEXT_SUB;
            break;
        case 1:
            u = pb(map, sq + 1);
            if (u <= 0xF0 && same_side(player, FLAGS(u)) && SCORE(u) < (int16_t)pw(aim, 0) && (ai_com(u) & 0x40)) {
                ai_give(table, u, pw(aim, 0), sq, 4);
                SUB(5);
            } else
                NEXT_SUB;
            break;
        case 2:
            for (; CUR <= 0xF0; NEXT_UNIT) {
                n = CUR;
                if (!half(n) && !(FLAGS(n) & 0xC000) && same_side(FLAGS(n), player)
                    && SCORE(n) < (int16_t)pw(aim, 0) && (ai_com(n) & 0x40))
                    break;
            }
            SUB(CUR <= 0xF0 ? sub + 1 : 4);
            break;
        case 3:
            spw(T(CUR), 2, can_go((int)sq, CUR, player, 0) ? GW(path_count) : 0x3E8);
            SUB(2);
            NEXT_UNIT;
            break;
        case 4:
            best = 0x3E8;
            who = -1;
            for (n = 0; n <= 0xF0; n++)
                if (!half(n) && !(FLAGS(n) & 0xC000) && same_side(FLAGS(n), player)
                    && SCORE(n) < (int16_t)pw(aim, 0) && (ai_com(n) & 0x40) && (int16_t)pw(T(n), 2) < best) {
                    best = (int16_t)pw(T(n), 2);
                    who = (int)n;
                }
            if (who >= 0)
                ai_give(table, (unsigned)who, pw(aim, 0), sq, 1);
            NEXT_SUB;
            break;
        default:
            AIM_DONE;
            break;
        }
        break;
    case 6:
        /* a unit of nobody to fetch */
        u = pw(aim, 2);
        if (task_helper(player, u, pw(aim, 0), 0x0A, pw(UNIT(u), 0x0B + 2 * player))) {
            STEP(1);
            SUB(0);
            spb(aim, 5, 0);
        }
        break;
    case 7:
        /* the units that are not whole (no ships): to an own building
         * with energy, the last of the list */
        switch (sub) {
        case 0:
            SB(plan_unit, 0);
            NEXT_SUB;
            break;
        case 1:
            for (; CUR <= 0xF0; NEXT_UNIT) {
                n = CUR;
                if (!(FLAGS(n) & 0xC048) && same_side(FLAGS(n), player)
                    && pb(UNIT(n), 2) < pb(TYPE(pb(UNIT(n), 8)), 2))
                    break;
            }
            SUB(CUR <= 0xF0 ? sub + 1 : 5);
            break;
        case 2:
            SW(path_count, 0);
            list_buildings(FP(hqs), player, 2);
            list_buildings(FP(factories), player, 10);
            list_buildings(FP(depots), player, 10);
            if (!GW(path_count)) {
                SUB(1);
                NEXT_UNIT;
            } else {
                SW(plan_count, GW(path_count));
                NEXT_SUB;
            }
            break;
        case 3: {
            unsigned v;

            if ((int16_t)GW(plan_count) <= 0)
                break;
            SW(plan_count, GW(plan_count) - 1);
            sq = pw(list, 2 * GW(plan_count));
            n = CUR;
            v = pb(UNIT(n), 2) < 5 && pb(UNIT(n), 1) > 1 ? 0x46 + 10u * pb(UNIT(n), 1) : 0x0F;
            if (can_go((int)sq, (int)n, player, 1) && (int16_t)GW(path_count) <= pb(TYPE(pb(UNIT(n), 8)), 0)) {
                /* within a move */
                v <<= 1;
                if (SCORE(n) < (int16_t)v)
                    ai_give(table, n, v, sq, 1);
                SUB(1);
                NEXT_UNIT;
            } else if (SCORE(n) < (int16_t)v) {
                SW(plan_score, v);
                SW(plan_square, sq);
                NEXT_SUB;
            } else {
                SUB(1);
                NEXT_UNIT;
            }
            break;
        }
        case 4:
            if ((ai_com(CUR) & 0x380) || task_helper(player, CUR, GW(plan_score), 9, GW(plan_square))) {
                SUB(1);
                NEXT_UNIT;
            }
            break;
        default:
            NEXT_STEP;
            break;
        }
        break;
    case 8:
        /* what each enemy unit cannot fire at */
        for (n = 0; n <= 0xF0; n++) {
            fptr t = TYPE(pb(UNIT(n), 8));

            if (half(n) || !same_side(other, FLAGS(n)) || (FLAGS(n) & 0xC000))
                continue;
            spw(T(n), 2, ((pw(t, 5) & 0xC0) ? 1 : 0) | (pb(t, 9) ? 0 : 2) | (pb(t, 0x0B) ? 0 : 4)
                         | (pb(t, 0x0A) ? 0 : 8));
        }
        STEP(pb(s, 4) + 1);
        break;
    case 9:
        /* the units that fire from afar: with a target they stay, and
         * their attack record gets the target that threatens most */
        bit = player ? 8 : 4;
        switch (sub) {
        case 0:
            SB(plan_unit, 0);
            NEXT_SUB;
            break;
        case 1:
            for (n = CUR; n <= 0xF0; n++)
                if (!half(n) && same_side(player, FLAGS(n)) && !(FLAGS(n) & 0xC200) && (ai_com(n) & 2)) {
                    SB(plan_unit, n);
                    break;
                }
            SUB(n <= 0xF0 ? sub + 1 : 4);
            break;
        case 2: {
            fptr t = TYPE(pb(UNIT(CUR), 8));

            sq = ai_place(CUR, player);
            clear_marks((int)bit);
            if (pb(t, 8) > 1)
                fire_reach(ai_paths(player), (int)sq, pb(t, 8), player, CUR, pw(t, 5) & 0xFFEF, map);
            if (pb(t, 7) > 1)
                fire_reach(ai_paths(player), (int)sq, pb(t, 7), player, CUR, pw(t, 5) & 0xFFD3, map);
            NEXT_SUB;
            break;
        }
        case 3:
            best = -1;
            who = -1;
            for (x = 0; w > x; x++)
                for (y = 0; (int16_t)GW(map_height) > y; y++) {
                    if (!(GBO(marks, (unsigned)(x + (y << 6))) & bit))
                        continue;
                    sq = (uint16_t)((w * y + x) << 1);
                    u = pb(map, sq + 1);
                    if ((int16_t)pw(T(u), 6) > best) {
                        who = (int)sq;
                        best = (int16_t)pw(T(u), 6);
                    }
                }
            n = CUR;
            if (who >= 0) {
                spb(T(n), 8, 4);
                spw(T(n), 0, ai_place(n, player));
                spw(T(n), 4, 0x64);
                spb(AI_ATTACK(n), 8, 0x0E);
                spw(AI_ATTACK(n), 0, (unsigned)who);
                spw(AI_ATTACK(n), 4, 0x64);
            }
            clear_marks((int)bit);
            NEXT_UNIT;
            SUB(1);
            break;
        default:
            NEXT_STEP;
            break;
        }
        break;
    case 0x0A:
        /* units around the enemy that threatens most: the aims' list
         * holds for each of the six squares around it up to 20 units
         * that could go there (+78h their numbers), the paths' list each
         * square's state (0 free, 1 taken, FFFFh none), +14h the squares,
         * +28h the unit chosen for each */
        foe = GB(plan_foe);
        p = ai_paths(player);
        bit = player ? 2 : 1;
        switch (sub) {
        case 0:
            SB(plan_foe, 0);
            NEXT_SUB;
            break;
        case 1:
            best = -1;
            SB(plan_unit, 0);
            for (n = 0; n <= 0xF0; n++)
                if (!half(n) && same_side(other, FLAGS(n)) && !(FLAGS(n) & 0xC002)
                    && (int16_t)pw(T(n), 6) > best && (int16_t)pw(T(n), 2) >= 0) {
                    best = (int16_t)pw(T(n), 6);
                    SB(plan_foe, n);
                }
            if (best >= 0) {
                NEXT_SUB;
                for (i = 0; i < 6; i++)
                    spb(list, 0x78 + (unsigned)i, 0);
            } else
                SUB(0x0B);
            break;
        case 2:
            NEXT_SUB;
            break;
        case 3:
            /* the next own unit near enough that can fire at it */
            for (n = CUR; n <= 0xF0; n++) {
                fptr t = TYPE(pb(UNIT(n), 8));

                if (half(n) || !same_side(player, FLAGS(n)) || (FLAGS(n) & 0xC000))
                    continue;
                if (bi_prog == BI_MOON && much_weaker(n, foe))
                    continue;
                if (SCORE(n) >= (int16_t)pw(T(foe), 6))
                    continue;
                if (distance(ai_place(n, player), ai_place(foe, player)) >= (pb(t, 0) >> 1))
                    continue;
                if (!(pw(t, 5) & FLAGS(foe) & 0x3C) || (pw(t, 5) & 0x40))
                    continue;
                SB(plan_unit, n);
                break;
            }
            SUB(n <= 0xF0 ? sub + 1 : 6);
            break;
        case 4:
            n = CUR;
            reach((int)ai_place(n, player), ai_paths(player), (int)n, pb(UNIT(n), 0), player,
                  (pw(UNIT(n), 6) & 1) ? 0xFFFF : (FLAGS(n) & 1) ? 6 : 3, map);
            NEXT_SUB;
            break;
        case 5:
            n = CUR;
            neighbours((int)ai_place(foe, player));
            for (i = 0; i < 6; i++) {
                unsigned count = pb(list, 0x78 + (unsigned)i);

                sq = GWO(around, 2 * i);
                if (count >= 0x14 || (int16_t)sq < 0 || square_crowded((int)sq, player))
                    continue;
                if (ai_place(n, player) != sq) {
                    unsigned g = GWO(ground, 6 * pb(map, sq));

                    if (!(GBO(marks, ai_mark(sq)) & bit) || pb(map, sq + 1) <= 0xF0)
                        continue;
                    if ((g & 0x540) || ((FLAGS(n) & 0x10) && (g & 0x4000)))
                        continue;
                    /* nobody's aim yet */
                    for (k = 0; k <= 0xF0; k++)
                        if (pw(T(k), 0) == sq)
                            break;
                    if (k <= 0xF0)
                        continue;
                }
                spb(list, 0x14 * (unsigned)i + count, n);
                spb(list, 0x78 + (unsigned)i, count + 1);
            }
            clear_marks((int)bit);
            NEXT_UNIT;
            SUB(2);
            break;
        case 6:
            SW(plan_got, 6);
            neighbours((int)ai_place(foe, player));
            for (i = 0; i < 6; i++) {
                sq = GWO(around, 2 * i);
                spb(p, 0x28 + (unsigned)i, 0xFF);
                if ((int16_t)sq < 0) {
                    spw(p, (unsigned)(2 * i), 0xFFFF);
                    SW(plan_got, GW(plan_got) - 1);
                    continue;
                }
                spw(p, 0x14 + (unsigned)(2 * i), sq);
                spw(p, (unsigned)(2 * i), 0);
                if (square_crowded((int)sq, player)) {
                    spw(p, (unsigned)(2 * i), 0xFFFF);
                    SW(plan_got, GW(plan_got) - 1);
                    continue;
                }
                for (k = 0; k <= 0xF0; k++)
                    if (!(FLAGS(k) & 0xC000) && same_side(FLAGS(k), player) && pb(T(k), 8) && pw(T(k), 0) == sq) {
                        spw(p, (unsigned)(2 * i), 1);
                        SW(plan_got, GW(plan_got) - 1);
                        break;
                    }
            }
            if ((int16_t)GW(plan_got) > 0) {
                NEXT_SUB;
                SW(plan_got, 0);
            } else
                SUB(0x0A);
            break;
#define COUNT(i) pb(list, 0x78 + (unsigned)(i))
#define CAND(i, j) pb(list, 0x14 * (unsigned)(i) + (unsigned)(j))
#define STATE(i) pw(p, 2 * (unsigned)(i))
#define TAKE(i, u, by) (spb(p, 0x28 + (unsigned)(i), u), spw(p, 2 * (unsigned)(i), 1), \
                        spb(list, 0x78 + (unsigned)(i), 0), SW(plan_got, GW(plan_got) + (by)))
        case 7:
            /* first the units that can fire at what the enemy cannot
             * answer */
            for (i = 0; i < 6; i++) {
                if (STATE(i))
                    continue;
                for (j = 0; COUNT(i) > j && (int16_t)GW(plan_got) < 4; j++) {
                    unsigned weak, fu;
                    int chosen;

                    u = CAND(i, j);
                    chosen = t1b01_0de3((int)u, player);
                    if ((int16_t)pw(T(foe), 6) <= SCORE(u) || chosen)
                        continue;
                    weak = pw(T(foe), 2);
                    fu = FLAGS(u);
                    if ((weak & 1) || ((weak & 2) && (fu & 0x10)) || ((weak & 8) && (fu & 4))
                        || ((weak & 4) && (fu & 8)))
                        TAKE(i, u, 1);
                }
            }
            NEXT_SUB;
            break;
        case 8:
            /* one opposite a square that is taken; pairs on opposite
             * squares; one between two that are taken; any */
            for (i = 0; i < 6; i++) {
                unsigned o = ai_mod6((unsigned)i + 3);

                if ((int16_t)GW(plan_got) >= 4)
                    break;
                if (STATE(i) == 1 && STATE(o) == 0) {
                    for (j = 0; COUNT(o) > j; j++) {
                        u = CAND(o, j);
                        if (!t1b01_0de3((int)u, player)) {
                            j = COUNT(o);
                            TAKE(o, u, 1);
                        }
                    }
                } else if (STATE(i) == 0 && STATE(o) == 0 && COUNT(i) > 0 && COUNT(o) > 0) {
                    unsigned a = 0xFF, b = 0xFF;

                    for (j = 0; COUNT(i) > j; j++)
                        if (!t1b01_0de3(CAND(i, j), player))
                            a = CAND(i, j);
                    for (j = 0; COUNT(o) > j; j++)
                        if (!t1b01_0de3(CAND(o, j), player) && CAND(o, j) != a)
                            b = CAND(o, j);
                    if (a != 0xFF && b != 0xFF) {
                        spb(p, 0x28 + (unsigned)i, a);
                        spb(p, 0x28 + o, b);
                        spw(p, 2 * (unsigned)i, 1);
                        spw(p, 2 * o, 1);
                        spb(list, 0x78 + (unsigned)i, 0);
                        spb(list, 0x78 + o, 0);
                        SW(plan_got, GW(plan_got) + 2);
                    }
                }
            }
            for (i = 0; i < 6; i++) {
                if (STATE(i) != 0 || STATE(ai_mod6((unsigned)i + 1)) != 1 || STATE(ai_mod6((unsigned)i + 5)) != 1
                    || (int16_t)GW(plan_got) >= 4)
                    continue;
                for (j = 0; COUNT(i) > j; j++) {
                    u = CAND(i, j);
                    if (!t1b01_0de3((int)u, player))
                        TAKE(i, u, 1);
                }
            }
            for (i = 0; i < 6; i++) {
                if ((int16_t)GW(plan_got) >= 4)
                    continue;
                for (j = 0; COUNT(i) > j; j++) {
                    u = CAND(i, j);
                    if (!t1b01_0de3((int)u, player))
                        TAKE(i, u, 1);
                }
            }
            NEXT_SUB;
            break;
        case 9:
            /* the chosen: to their squares, and fire at the enemy */
            for (i = 0; i < 6; i++) {
                u = pb(p, 0x28 + (unsigned)i);
                if (u == 0xFF)
                    continue;
                sq = pw(p, 0x14 + (unsigned)(2 * i));
                spb(T(u), 8, ai_place(u, player) == sq ? 4 : 3);
                spw(T(u), 0, sq);
                spw(T(u), 4, pw(T(foe), 6));
                spb(AI_ATTACK(u), 8, 2);
                spw(AI_ATTACK(u), 0, ai_place(foe, player));
                spw(AI_ATTACK(u), 4, pw(T(foe), 6));
            }
            NEXT_SUB;
            break;
        case 0x0A:
            spw(T(foe), 2, 0xFFFF);
            SB(plan_foe, foe + 1);
            SUB(1);
            break;
        default:
            NEXT_STEP;
            break;
        }
        break;
    case 0x0B:
        /* the units without a task: to the enemy that threatens most
         * which the unit can fire at and reach, else to the aim of the
         * nearest own unit with a task, else towards the headquarters */
        switch (sub) {
        case 0:
            SB(plan_unit, 0);
            NEXT_SUB;
            for (n = 0; n <= 0xF0; n++)
                spw(T(n), 2, 0x3E8);
            break;
        case 1:
            for (; CUR <= 0xF0; NEXT_UNIT) {
                n = CUR;
                if (!half(n) && !(FLAGS(n) & 0xC000) && !(pw(UNIT(n), 6) & 0x20) && !(ai_com(n) & 0x380)
                    && same_side(FLAGS(n), player) && !pb(T(n), 8))
                    break;
            }
            SUB(CUR <= 0xF0 ? sub + 1 : 0x0A);
            break;
        case 2:
            /* the others' distances: of an own unit its aim's */
            for (u = 0; u <= 0xF0; u++) {
                if ((FLAGS(u) & 0xC002) || (pw(UNIT(u), 6) & 0x20) || u == CUR)
                    continue;
                spw(T(u), 2, (unsigned)distance(ai_place(CUR, player),
                                                 same_side(FLAGS(u), player) ? pw(T(u), 0) : ai_place(u, player)));
            }
            SUB(5);
            break;
        case 5:
            best = -1;
            f = pw(TYPE(pb(UNIT(CUR), 8)), 5) & 0x3C;
            SB(plan_other, 0xFF);
            for (u = 0; u <= 0xF0; u++)
                if (!(FLAGS(u) & 0xC000) && same_side(FLAGS(u), other) && (int16_t)pw(T(u), 6) > best
                    && (int16_t)pw(T(u), 2) < 0x3E8 && (f & FLAGS(u))) {
                    best = (int16_t)pw(T(u), 6);
                    SB(plan_other, u);
                }
            SUB(GB(plan_other) <= 0xF0 ? sub + 1 : 7);
            break;
        case 6:
        case 8:
            u = GB(plan_other);
            sq = sub == 6 ? ai_place(u, player) : pw(T(u), 0);
            if (can_go((int)sq, CUR, player, 0)) {
                ai_give(table, CUR, 0x0A, sq, 1);
                NEXT_UNIT;
                SUB(1);
            } else {
                spw(T(u), 2, 0x3E8);
                SUB(sub - 1);
            }
            break;
        case 7:
            best = 0x3E8;
            SB(plan_other, 0xFF);
            for (u = 0; u <= 0xF0; u++) {
                k = pb(T(u), 8);
                if ((FLAGS(u) & 0xC000) || (ai_com(u) & 0x380) || !same_side(FLAGS(u), player)
                    || SCORE(u) <= 0x0A || (int16_t)pw(T(u), 2) >= best || k == 0 || k == 8 || k == 0x0C || k == 6)
                    continue;
                best = (int16_t)pw(T(u), 2);
                SB(plan_other, u);
            }
            SUB(GB(plan_other) <= 0xF0 ? sub + 1 : 9);
            break;
        case 9:
            if ((ai_com(CUR) & 0x380)
                || task_helper(player, CUR, 0x46, 0x0B, pw(AI_BUILDING(hqs, player), 0x0E))) {
                SUB(1);
                NEXT_UNIT;
            }
            break;
        default:
            NEXT_STEP;
            break;
        }
        break;
    default:
        spb(s, 4, 0);
        spb(s, 6, 0);
        return 1;
    }
    return 0;
}
