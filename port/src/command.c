/* command.c - BATTLE.EXE's T1938: the computer player's commands carried
 * out as keys.  A command is 7 bytes (+0 its kind, +1 a unit, +3 a
 * square or a unit or a building); a player's queue holds ten of them,
 * +46h which one runs (while handing out: how many there are), +47h the
 * command's step, +48h the step of the cursor's way to a square.  A step
 * puts bytes into the player's script of keys (1 up, 2 down, 3 left, 4
 * right, 5 fire; 80h..83h fire with up, down, left, right; FFh ends it),
 * which script_step plays, a byte a pass, into the computer's keys.
 */
#include "ai.h"

/* bytes onto the script and FFh behind them */
static void put(int player, unsigned key)
{
    fptr s = AI_SCRIPT(player);

    spb(s, pb(s, 0x1E), key);
    spb(s, 0x1E, pb(s, 0x1E) + 1);
    spb(s, pb(s, 0x1E), 0xFF);
}

static void give_up(int player) { spw(AI_PLAYER(player), 0, pw(AI_PLAYER(player), 0) | 8); }

/* the next byte of the script into the player's directions and fire; 1
 * at the script's end */
int script_step(int player)
{
    fptr s = AI_SCRIPT(player);
    unsigned k = pb(s, pb(s, 0x1E)), v;
    int end = 0;

    SWO(computer_keys, 4 * player, 0);
    if (k != 0xFF) {
        if (k & 0x80) {
            v = GBO(computer_key_sequences, 3 * (k & 0x7F) + pb(s, 0x1F));
            if (v != 0xFF) {
                SWO(computer_keys, 4 * player, v);
                spb(s, 0x1F, pb(s, 0x1F) + 1);
            } else {
                spb(s, 0x1F, 0);
                spb(s, 0x1E, pb(s, 0x1E) + 1);
            }
        } else {
            SWO(computer_keys, 4 * player, GBO(computer_key_bits, k));
            spb(s, 0x1E, pb(s, 0x1E) + 1);
        }
    }
    if (pb(s, pb(s, 0x1E)) == 0xFF) {
        spb(s, 0, 0xFF);
        spb(s, 0x1E, 0);
        spb(s, 0x1F, 0);
        end = 1;
    }
    return end;
}

/* a command onto the player's queue, FFh behind it; ten at most */
void command_add(int player, fptr command)
{
    fptr q = AI_QUEUE(player);
    unsigned slot, end;

    if (pb(q, 0x46) >= 0x0A)
        return;
    if (!pb(q, 0x46)) {
        slot = 0;
        spb(q, 0x46, 1);
        end = 1;
    } else {
        slot = pb(q, 0x46);
        spb(q, 0x46, slot + 1);
        end = pb(q, 0x46);
    }
    spb(q, 7 * slot, pb(command, 0));
    spw(q, 7 * slot + 1, pw(command, 1));
    spw(q, 7 * slot + 3, pw(command, 3));
    spw(q, 7 * slot + 5, pw(command, 5));
    spb(q, 7 * end, 0xFF);
}

/* the directions from x, y to ax, ay into the script; 1 when there */
static int toward(int player, int x, int y, int ax, int ay)
{
    fptr s = AI_SCRIPT(player);

    if (x == ax && y == ay)
        return 1;
    if (x != ax) {
        spb(s, pb(s, 0x1E), x > ax ? 3 : 4);
        spb(s, 0x1E, pb(s, 0x1E) + 1);
    }
    if (y != ay) {
        spb(s, pb(s, 0x1E), y > ay ? 1 : 2);
        spb(s, 0x1E, pb(s, 0x1E) + 1);
    }
    spb(s, pb(s, 0x1E), 0xFF);
    return 0;
}

/* a step of the cursor towards the square; 1 when it is there (or the
 * square is none of the map's, or in its last column or row) */
static int cursor_steer(int player, int cur, int aim)
{
    int w = (int16_t)GW(map_width), h = (int16_t)GW(map_height), ax, ay;

    cur = (int16_t)cur, aim = (int16_t)aim;
    if (aim < 0 || (int16_t)GW(map_bytes) < aim)
        return 1;
    cur >>= 1;
    aim >>= 1;
    ax = aim % w;
    ay = aim / w;
    if (w - 1 <= ax || h - 1 <= ay)
        return 1;
    return toward(player, cur % w, cur / w, ax, ay);
}

/* T1938:140D: a step of the overview's window towards the one with the
 * square x, y in its middle; 1 when it is there */
static int overview_steer(int player, int x, int y)
{
    int w = (int16_t)GW(map_width), h = (int16_t)GW(map_height);

    if (w <= x || h <= y)
        return 1;
    x -= 5;
    if (x < 0)
        x = 0;
    else if (w - 10 < x)
        x = w - 10;
    y -= 4;
    if (y < 0)
        y = 0;
    else if (h - 8 < y)
        y = h - 8;
    if (x & 1)
        x--;
    if (y & 1)
        y--;
    return toward(player, (int16_t)pw(CURSOR(player), 0x0C), (int16_t)pw(CURSOR(player), 0x0E), x, y);
}

/* T1938:1245: the cursor to a square, in steps (the queue's +48h): a
 * square five or more away by the overview (fire with right, its window
 * moved, fire), then step by step; 1 when it is there */
static int go_to(int player, int cur, int aim, fptr q)
{
    int w = (int16_t)GW(map_width), c, a, dx, dy;

    cur = (int16_t)cur, aim = (int16_t)aim;
    if (aim < 0 || (int16_t)GW(map_bytes) < aim)
        return 1;
    c = cur >> 1;
    a = aim >> 1;
    switch (pb(q, 0x48)) {
    case 0:
        dx = c % w - a % w;
        dy = c / w - a / w;
        if (dx < 0)
            dx = -dx;
        if (dy < 0)
            dy = -dy;
        if (pb(CURSOR(player), 0x18) || (dx < 5 && dy < 5))
            spb(q, 0x48, 4);
        else
            spb(q, 0x48, pb(q, 0x48) + 1);
        break;
    case 1:
        put(player, 0x83);
        spb(q, 0x48, pb(q, 0x48) + 1);
        break;
    case 2:
        if (overview_steer(player, a % w, a / w))
            spb(q, 0x48, pb(q, 0x48) + 1);
        break;
    case 3:
        put(player, 5);
        spb(q, 0x48, pb(q, 0x48) + 1);
        break;
    case 4:
        if (cursor_steer(player, cur, aim))
            spb(q, 0x48, pb(q, 0x48) + 1);
        break;
    default:
        spb(q, 0x48, 0);
        return 1;
    }
    return 0;
}

static int find_in_slots(int player, unsigned unit, fptr table, int count)
{
    int k, i;

    for (k = 0; k < count; k++) {
        fptr r = MKFP(FSEG(table), FOFF(table) + 0x1C * k);

        if (pw(r, 0x19) & 0x8002)
            continue;
        for (i = 0; i < 7; i++)
            if (pb(r, (unsigned)(7 * player + i)) == unit)
                return k;
    }
    return -1;
}

/* the record of the building or unit that holds the unit in one of its
 * slots: the factories, the depots, the units that hold others, the
 * headquarters; FFFF:FFFF when none does */
static fptr find_holder(int player, unsigned unit)
{
    int k;

    if ((k = find_in_slots(player, unit, FP(factories), 0x0A)) >= 0)
        return AI_BUILDING(factories, k);
    if ((k = find_in_slots(player, unit, FP(depots), 0x0A)) >= 0)
        return AI_BUILDING(depots, k);
    if ((k = find_in_slots(player, unit, FP(cargo), 0x46)) >= 0)
        return AI_BUILDING(cargo, k);
    if ((k = find_in_slots(player, unit, FP(hqs), 2)) >= 0)
        return AI_BUILDING(hqs, k);
    return 0xFFFFFFFFu;
}

/* up or down in a building's screen to the last slot that holds `what`;
 * 1 when the cursor is on it */
static int to_slot(int player, fptr rec, unsigned what)
{
    unsigned slot = 0, i;
    int at = (int16_t)pw(CURSOR(player), 0x0E);

    for (i = 0; i < 7; i++)
        if (pb(rec, (unsigned)(7 * player) + i) == what)
            slot = i;
    if (at < (int)slot)
        put(player, 2);
    else if (at > (int)slot)
        put(player, 1);
    else
        return 1;
    return 0;
}

#define NEXT spb(q, 0x47, pb(q, 0x47) + 1)
#define OVER return spb(q, 0x47, 0), 1

/* 1: a unit to a square */
static int command_move(int player, int cur, unsigned unit, unsigned aim, fptr q)
{
    switch (pb(q, 0x47)) {
    case 0:
        if (!(GW(game_flags2) & 0x80))
            NEXT;
        spb(q, 0x48, 0);
        break;
    case 1:
        if (go_to(player, cur, (int)ai_place(unit, player), q))
            NEXT;
        break;
    case 2:
        put(player, 0x80);
        NEXT;
        break;
    case 3:
        if (cursor_steer(player, cur, (int)aim))
            NEXT;
        break;
    case 4:
        /* the choice is given up when the square is not in reach or the
         * unit cannot stop there */
        SB(move_record, unit);
        if (!(GBO(marks, ai_mark((unsigned)cur)) & (player ? 2 : 1)))
            give_up(player);
        else if (stop_check(cur, player, ai_map(player)))
            give_up(player);
        put(player, 5);
        NEXT;
        break;
    case 5:
        NEXT;
        break;
    case 6:
        put(player, 5);
        NEXT;
        break;
    default:
        OVER;
    }
    return 0;
}

/* 2: a unit fires at a unit */
static int command_fire(int player, int cur, unsigned unit, unsigned target, fptr q)
{
    switch (pb(q, 0x47)) {
    case 0:
        if (go_to(player, cur, (int)ai_place(unit, player), q))
            NEXT;
        break;
    case 1:
        put(player, 0x80);
        NEXT;
        break;
    case 2:
        if (cursor_steer(player, cur, (int)ai_place(target, player)))
            NEXT;
        break;
    case 3:
        put(player, 5);
        if (GBO(marks, ai_mark(ai_place(target, player))) & (player ? 8 : 4))
            spb(q, 0x47, 5);
        else {
            NEXT;
            give_up(player);
        }
        break;
    case 4:
        put(player, 5);
        NEXT;
        break;
    default:
        OVER;
    }
    return 0;
}

/* 3: the change of phase asked for, a row below the own headquarters */
static int command_change(int player, int cur, unsigned hq, fptr q)
{
    switch (pb(q, 0x47)) {
    case 0:
        if (go_to(player, cur, (int)hq, q))
            NEXT;
        break;
    case 1:
        if (cursor_steer(player, cur, (int)(uint16_t)(hq + (GW(map_width) << 1))))
            NEXT;
        break;
    case 2:
        if (!(GW(game_flags2) & 0x80)) {
            put(player, 0x82);
            NEXT;
        }
        break;
    case 3:
        if (pw(AI_STATE(player), 0) & 4) {
            spw(AI_STATE(player), 0, pw(AI_STATE(player), 0) & 0xFFFB);
            NEXT;
        }
        break;
    default:
        OVER;
    }
    return 0;
}

/* 4: a unit out of its building */
static int command_out(int player, int cur, unsigned unit, fptr q)
{
    fptr list;

    switch (pb(q, 0x47)) {
    case 0:
        SFP(computer_holder, find_holder(player, unit));
        if (GFP(computer_holder) == 0xFFFFFFFFu)
            OVER;
        NEXT;
        break;
    case 1:
        if (go_to(player, cur, pw(GFP(computer_holder), 0x0E), q))
            NEXT;
        break;
    case 2:
        put(player, 0x82);
        NEXT;
        break;
    case 3:
        if (to_slot(player, GFP(computer_holder), unit))
            NEXT;
        break;
    case 4:
        if (!(GW(game_flags2) & 0x80))
            NEXT;
        break;
    case 5:
        put(player, 0x80);
        NEXT;
        break;
    case 6:
        NEXT;
        break;
    case 7:
        /* the first square the unit may stop on; none: back in */
        list = ai_aims(player);
        list_reach(list, player, (int)unit);
        if (!GW(path_count)) {
            SW(computer_out_square, pw(GFP(computer_holder), 0x0E));
            give_up(player);
        } else
            SW(computer_out_square, pw(list, 0));
        SW(path_count, 0);
        NEXT;
        break;
    case 8:
        if (cursor_steer(player, cur, GW(computer_out_square)))
            NEXT;
        break;
    case 9:
    case 10:
        put(player, 5);
        NEXT;
        break;
    case 11:
        put(player, 0x83);
        OVER;
    default:
        OVER;
    }
    return 0;
}

/* 6: a unit repaired in its building */
static int command_repair(int player, int cur, unsigned unit, fptr q)
{
    switch (pb(q, 0x47)) {
    case 0:
        SFP(computer_repair_holder, find_holder(player, unit));
        if (GFP(computer_repair_holder) == 0xFFFFFFFFu)
            OVER;
        NEXT;
        break;
    case 1:
        if (go_to(player, cur, pw(GFP(computer_repair_holder), 0x0E), q))
            NEXT;
        break;
    case 2:
        put(player, 0x82);
        NEXT;
        break;
    case 3:
        if (to_slot(player, GFP(computer_repair_holder), unit))
            NEXT;
        break;
    case 4:
        put(player, 0x81);
        NEXT;
        break;
    case 5:
        put(player, 0x83);
        OVER;
    default:
        OVER;
    }
    return 0;
}

/* 7: a type made in a factory */
static int command_make(int player, int cur, unsigned type, unsigned factory, fptr q)
{
    unsigned line = 0, i;

    switch (pb(q, 0x47)) {
    case 0:
        SFP(computer_factory, AI_BUILDING(factories, factory));
        if (go_to(player, cur, pw(GFP(computer_factory), 0x0E), q))
            NEXT;
        break;
    case 1:
    case 3:
    case 5:
        put(player, 0x82);
        NEXT;
        break;
    case 2:
        if (to_slot(player, GFP(computer_factory), 0xFF))
            NEXT;
        break;
    case 4:
        /* down to the type's line of the list */
        for (i = 0; GB(types_listed) > i; i++)
            if (GBO(types_list, i) == type)
                line = i;
        if ((int16_t)(pw(CURSOR(player), 0x0E) + pw(CURSOR(player), 0x20)) < (int)line)
            put(player, 2);
        else
            NEXT;
        break;
    case 6:
        put(player, 0x83);
        NEXT;
        break;
    default:
        OVER;
    }
    return 0;
}

/* a step of the command that runs; FFFFh when the queue is at its end */
int command_step(int player)
{
    fptr q = AI_QUEUE(player), c = MKFP(FSEG(q), FOFF(q) + 7 * pb(q, 0x46));
    int cur = pw(CURSOR(player), 0), over = 0;

    bi_seen("command", pb(c, 0));
    switch (pb(c, 0)) {
    case 0xFF:
        spb(q, 0, 0xFF);
        spb(q, 0x46, 0);
        return 0xFFFF;
    case 1:
        over = command_move(player, cur, pb(c, 1), pw(c, 3), q);
        break;
    case 2:
        over = command_fire(player, cur, pb(c, 1), pb(c, 3), q);
        break;
    case 3:
        over = command_change(player, cur, pw(AI_BUILDING(hqs, player), 0x0E), q);
        break;
    case 4:
        over = command_out(player, cur, pb(c, 1), q);
        break;
    case 5:
        over = go_to(player, cur, pw(c, 1), q);
        break;
    case 6:
        over = command_repair(player, cur, pb(c, 1), q);
        break;
    case 7:
        over = command_make(player, cur, pb(c, 1), pb(c, 3), q);
        break;
    default:
        spb(q, 0, 0xFF);
        spb(q, 0x46, 0);
        return 0;
    }
    if (over)
        spb(q, 0x46, pb(q, 0x46) + 1);
    return 0;
}
