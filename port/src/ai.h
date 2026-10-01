/* ai.h - what the computer player's modules share (computer.c, plan.c,
 * command.c).  Its state is in F2C0A: a record of 0Ch bytes a player at
 * computer_state (+0 the stage: 20h assess, 40h plan, 8 hand out, 2 a
 * command runs, 1 its keys are played, 4 the change of phase is over;
 * +3, +4, +5 the steps of handing out, planning and assessing, +6 the
 * number of aims, +7 a step within a step, +8 the helper's step, +9 a
 * word, +0Bh which of two messages), a record of 9 bytes a unit for the
 * move phase and one for the attack phase (+0 its aim, a square or a
 * unit; +2 a distance, or FFFFh "not handled yet"; +4 the task's score;
 * +6 how much the unit threatens; +8 the task), the commands' queues and
 * the keys' scripts.  The aims (6 bytes each: the score, +2 where, +4
 * and +5 the kind) are in the player's list at his record's +9, the
 * paths in the one at +0Dh.  tools/computer.py has the rules in full.
 */
#ifndef AI_H
#define AI_H

#include "bi.h"

#define AI_PLAYER(p) MKFP(S_players, A_players + 0x17 * (p))
#define AI_STATE(p) MKFP(S_computer_state, A_computer_state + 0x0C * (p))
#define AI_MOVE(n) MKFP(S_computer_move_units, A_computer_move_units + 9 * (n))
#define AI_ATTACK(n) MKFP(S_computer_attack_units, A_computer_attack_units + 9 * (n))
#define AI_COM(type) MKFP(S_computer_types, A_computer_types + 6 * (type))
#define AI_SCRIPT(p) MKFP(S_computer_scripts, A_computer_scripts + 0x20 * (p))
#define AI_QUEUE(p) MKFP(S_computer_queues, A_computer_queues + 0x49 * (p))
#define AI_BUILDING(table, n) MKFP(S_##table, A_##table + 0x1C * (n))

/* the units' records of the phase the player is in */
static inline fptr ai_table(int player)
{
    return pb(CURSOR(player), 0x16) == 2 ? FP(computer_attack_units) : FP(computer_move_units);
}

static inline fptr ai_rec(fptr table, unsigned n) { return MKFP(FSEG(table), FOFF(table) + 9 * n); }
static inline fptr ai_aims(int player) { return pfp(AI_PLAYER(player), 9); }
static inline fptr ai_paths(int player) { return pfp(AI_PLAYER(player), 0x0D); }
static inline fptr ai_map(int player) { return player ? GFP(map1) : GFP(map0); }
static inline unsigned ai_place(unsigned n, int player) { return (uint16_t)(pw(UNIT(n), 0x0B + 2 * player) - 1); }
static inline unsigned ai_com(unsigned n) { return pw(AI_COM(pb(UNIT(n), 8)), 2); }

static inline unsigned ai_mark(unsigned off)
{
    int s = (int16_t)off >> 1, w = (int16_t)GW(map_width);

    return (uint16_t)(s % w + ((s / w) << 6));
}

static inline unsigned ai_mod6(unsigned n)
{
    n &= 0xFF;
    while (n > 5)
        n = (n - 6) & 0xFF;
    return n;
}

/* the first half of a unit of two squares */
static inline unsigned ai_first_half(unsigned n)
{
    unsigned f = pw(UNIT(n), 4);

    return (f & 0x40) && !(f & 0x80) ? (n - 1) & 0xFF : n;
}

/* a unit's record of the phase gets a task, its attack record none */
static inline void ai_give(fptr table, unsigned n, unsigned score, unsigned aim, unsigned task)
{
    spw(ai_rec(table, n), 4, score);
    spw(ai_rec(table, n), 0, aim);
    spb(ai_rec(table, n), 8, task);
    spb(AI_ATTACK(n), 8, 0);
    spw(AI_ATTACK(n), 4, 0xFFFF);
}

/* an aim onto the player's list */
static inline void ai_add_aim(int player, unsigned score, unsigned where, unsigned kind)
{
    fptr s = AI_STATE(player), e = ai_aims(player);

    e = MKFP(FSEG(e), FOFF(e) + 6 * pb(s, 6));
    spw(e, 0, score);
    spb(e, 4, kind);
    spb(e, 5, kind);
    spw(e, 2, where);
    spb(s, 6, pb(s, 6) + 1);
}

/* computer.c */
void unit_strength(int unit, fptr out);
int find_kind(int mask, int side);
int can_go(int square, int unit, int player, int flags);
int task_approach(int unit, int player);
int task_move(int unit, int player);
void list_buildings(fptr table, int player, int count);
int square_crowded(int square, int player);
int t1b01_0de3(int unit, int player);
void task_home(int player, int unit);
int computer_assess(int player);
int computer_hand_out(int player);

/* plan.c */
int computer_plan(int player);

/* command.c */
void command_add(int player, fptr command);
int command_step(int player);
int script_step(int player);

#endif
