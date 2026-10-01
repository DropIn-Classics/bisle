/* cursor.c - BATTLE.EXE's T0D36: the players' cursors on the map: the
 * directions and fire of a pass, what fire offers on a square, and what
 * comes of the choice (BATTLE.hints at "What fire on a square offers").
 *
 * A cursor's record (31h bytes, one a player): +0 its square's offset in
 * the map, +2 the window's first square, +0Ch, +0Eh column and row in
 * the window, +10h, +12h where it is drawn, +14h, +15h the window's
 * first column and row by two, +16h the phase (1 move, 2 attack), +17h
 * the state (0 the map; 2 a building, 3 the overview, 4 the unit's and
 * the status screen, 5 the change asked for, 6 a building's screen
 * opens), +18h the step within it, +1Bh the cursor's picture (and what
 * fire with a direction chooses), +1Ch what fire offers here, +1Eh the
 * unit chosen, +28h..+2Ah the passes between two steps of the cursor,
 * +2Bh..+2Eh how long a direction is held, +30h fire seen let go.
 */
#include "bi.h"

/* T0D36:0044: where the cursor is drawn and its square, from its column
 * and row in the window */
void t0d36_0044(fptr cur, int player)
{
    unsigned row = pw(cur, 0x0E), col = pw(cur, 0x0C);

    if (col & 1)
        spw(cur, 0x12, GWO(window_y_odd, 2 * row));
    else
        spw(cur, 0x12, GWO(window_y_even, 2 * row));
    spw(cur, 0x10, GWO(window_x, 2 * col));
    if (player)
        spw(cur, 0x10, pw(cur, 0x10) + 0xA0);
    spw(cur, 0, pw(cur, 2) + (col << 1) + row * (GW(map_width) << 1));
}

/* whether a unit's word +4 (or a player's number) `a` is of the side of
 * `b`: bit 0 player 1, bit 1 nobody's, else player 0 */
int same_side(int a, int b)
{
    if (a & 1)
        return (b & 1) != 0;
    if (a & 2)
        return (b & 2) != 0;
    return !(b & 1);
}

/* a timer of the map's loop: `kind` after `delay` passes with `arg`;
 * the kinds 1 and 2 (a player's message line) take the place of one of
 * their kind */
void timer_set(int kind, int delay, int arg)
{
    int i, j;

    for (i = 0; i < 0x0F; i++) {
        if (GBO(timer_kinds, i))
            continue;
        if (kind == 1 || kind == 2)
            for (j = 0; j < 0x0F; j++)
                if (GBO(timer_kinds, j) == kind)
                    SBO(timer_kinds, j, 0);
        SBO(timer_kinds, i, kind);
        SWO(timer_args, 2 * i, arg);
        spd(FP(timer_dues), 4 * i, GD(passes) + (uint32_t)(int32_t)(int16_t)delay);
        return;
    }
}

/* a timer has run: 1, 2 a player's message line cleared; 3, 4 flags for
 * the loop; 5 a square drawn again; 6, 7 flags cleared */
void timer_due(int kind, int arg)
{
    switch (kind) {
    case 1:
        show_message(-1, 0);
        SW(game_flags, GW(game_flags) & ~4);
        break;
    case 2:
        show_message(-1, 1);
        SW(game_flags, GW(game_flags) & ~8);
        break;
    case 3:
        SW(game_flags2, GW(game_flags2) | 0x20);
        break;
    case 4:
        SW(game_flags2, GW(game_flags2) | 0x40);
        break;
    case 5:
        redraw_square((GBO(cursors, 0x31 + 0x16) & 2) ? 1 : 0, GWO(timer_args, 2 * arg));
        break;
    case 6:
        SW(game_flags2, GW(game_flags2) & ~0x200);
        break;
    case 7:
        SW(game_flags2, GW(game_flags2) & ~0x400);
        break;
    }
}

/* T0D36:032C: a cursor's record (its first 28h bytes) kept in
 * cursor_kept, or put back from there */
void t0d36_032c(fptr cur, int back)
{
    fptr from = back ? FP(cursor_kept) : cur, to = back ? cur : FP(cursor_kept);
    unsigned i;

    for (i = 0; i < 0x14; i += 2)
        spw(to, i, pw(from, i));
    for (i = 0x14; i < 0x1C; i++)
        spb(to, i, pb(from, i));
    for (i = 0x1C; i < 0x28; i += 2)
        spw(to, i, pw(from, i));
}

/* T0D36:04D8: a player's input of this pass as bits (1 up, 2 down, 4
 * left, 8 right, 10h fire) and the input taken.  A direction held comes
 * only every few passes, fewer the longer it is held (the cursor's
 * +28h..+2Ah, by +2Bh); fire comes always. */
int t0d36_04d8(fptr in, fptr cur)
{
    unsigned fire = 0, dirs = 0;
    int now = 0;

    if (pb(in, 4))
        fire = 0x10;
    if (pb(in, 0))
        dirs |= 4;
    else if (pb(in, 1))
        dirs |= 8;
    if (pb(in, 2))
        dirs |= 1;
    else if (pb(in, 3))
        dirs |= 2;
    spb(in, 4, 0), spb(in, 3, 0), spb(in, 2, 0), spb(in, 1, 0), spb(in, 0, 0);
    if (pb(cur, 0x2C)) {
        spb(cur, 0x2C, pb(cur, 0x2C) - 1);
        if (pw(cur, 0x2E) != dirs)
            now = 1;
    } else
        now = 1;
    if (now) {
        if (pw(cur, 0x2E) != dirs) {
            spb(cur, 0x2D, 0);
            spb(cur, 0x2B, 0);
        }
        fire |= dirs;
        spb(cur, 0x2C, pb(cur, 0x28 + pb(cur, 0x2B)));
        spw(cur, 0x2E, fire);
        if (pb(cur, 0x2D))
            spb(cur, 0x2D, pb(cur, 0x2D) - 1);
        else {
            spb(cur, 0x2B, pb(cur, 0x2B) + 1);
            if (pb(cur, 0x2B) > 2)
                spb(cur, 0x2B, 2);
        }
    }
    return (int)fire;
}

/* the library's srand and rand (CODE:2627, CODE:2640) */
void bi_srand(unsigned seed)
{
    SD(rand_seed, seed & 0xFFFF);
}

int bi_rand(void)
{
    uint32_t s = GD(rand_seed) * 0x015A4E35u + 1;

    SD(rand_seed, s);
    return (int)(s >> 16 & 0x7FFF);
}

/* a random number from lo to hi */
int bi_random(int lo, int hi)
{
    if (hi > 0x7FFF)
        hi = 0x7FFF;
    return bi_rand() % (hi - lo + 1) + lo;
}

static void message_flag(int side)
{
    SW(game_flags2, GW(game_flags2) | (side ? 0x10 : 8));
}

static void ask_sound(int number)
{
    SW(game_flags2, GW(game_flags2) | 0x100);
    SW(number_asked, number);
}

/* T0D36:0642: a pass of a cursor that has a unit chosen: the steps 16h
 * (a unit to move: fire on a square is move_aim), 17h (the move's path
 * shown), 3 (fire again starts the move), 18h (the move under way), 13h
 * (a unit to fire: fire on a square gives the order), 1Ah, 19h; else
 * fire on the map begins a choice (state 1).  `in` the player's input
 * record (+5 fire twice, +8 fire let go), `bits` this pass' input.
 * Returns 0 when fire was taken, else 1. */
int t0d36_0642(fptr cur, fptr in, int bits, fptr map, int side)
{
    int ret = (bits & 0x10) ? 0 : 1;
    fptr pl = MKFP(S_players, A_players + 0x17 * side);
    int moving = 0;

    switch (pb(cur, 0x18)) {
    case 0x16:
        if (pb(in, 8) && !pb(cur, 0x30)) {
            spb(cur, 0x30, 1);
            break;
        }
        if (!(bits & 0x10) || !pb(cur, 0x30))
            break;
        spb(cur, 0x30, move_aim(cur, map, side));
        if (pb(cur, 0x30) & 0x10) {
            SW(game_flags2, GW(game_flags2) | 0x400);
            timer_set(7, 4, 0);
            spb(in, 5, 0);
            spb(cur, 0x18, 0x17);
        } else {
            spb(cur, 0x18, 3);
            spb(cur, 0x30, 0);
        }
        spb(in, 8, 0);
        break;
    case 0x18:
        moving = 1;
        /* fall through */
    case 0x17:
        if (GW(game_flags2) & 0x400) {
            if (pb(in, 5) || (pw(pl, 0) & 8)) {
                spw(pl, 0, pw(pl, 0) & ~8);
                spb(cur, 0x30, 0x10);
                unit_release(cur, map, side, 0x10);
                spb(cur, 0x30, 0);
                spb(in, 5, 0);
            }
        } else if (moving) {
            /* the reach again, with the arguments reach kept */
            fptr a = FP(reach_args);

            reach((int16_t)pw(a, 0), pfp(a, 2), (int16_t)pw(a, 6), (int16_t)pw(a, 8), (int16_t)pw(a, 0x0A),
                  (int16_t)pw(a, 0x0C), pfp(a, 0x0E));
            draw_marks(GWO(cursors, 0x31 * side + 2), side);
            ask_sound(2);
            spb(cur, 0x30, 0);
            spb(cur, 0x18, 0x16);
        } else
            spb(cur, 0x18, 0x16);
        spb(in, 8, 0);
        break;
    case 3:
        if (pb(in, 8) && !pb(cur, 0x30)) {
            spb(cur, 0x30, 1);
            break;
        }
        if (!(bits & 0x10) || !pb(cur, 0x30))
            break;
        spb(cur, 0x30, t122d_05b8(cur, side));
        if (pb(cur, 0x30) & 0x10) {
            SW(game_flags2, GW(game_flags2) | 0x400);
            timer_set(7, 4, 0);
            spb(in, 5, 0);
            spb(cur, 0x18, 0x18);
        } else {
            spb(cur, 0x17, 0);
            spb(cur, 0x18, 0x19);
            spb(cur, 0x1B, 5);
            unit_release(cur, map, side, (int8_t)pb(cur, 0x30));
            spb(cur, 0x30, 0);
        }
        spb(in, 8, 0);
        break;
    case 0x13:
        if (pb(in, 8) && !pb(cur, 0x30)) {
            spb(cur, 0x30, 1);
            break;
        }
        if (!(bits & 0x10) || !pb(cur, 0x30))
            break;
        spb(cur, 0x30, give_order(cur, map, side));
        if (pb(cur, 0x30)) {
            order_release(cur, side);
            spb(cur, 0x17, 0);
            spb(cur, 0x18, 0x19);
            spb(cur, 0x1B, 5);
        } else {
            spb(cur, 0x18, 0x1A);
            spb(in, 5, 0);
            SW(game_flags2, GW(game_flags2) | 0x400);
            timer_set(7, 4, 0);
        }
        spb(cur, 0x30, 0);
        spb(in, 8, 0);
        break;
    case 0x1A:
        if (GW(game_flags2) & 0x400) {
            if (pb(in, 5) || (pw(pl, 0) & 8)) {
                spw(pl, 0, pw(pl, 0) & ~8);
                order_release(cur, side);
                spb(cur, 0x17, 0);
                spb(cur, 0x18, 0x19);
                spb(cur, 0x1B, 5);
                ask_sound(3);
                spb(cur, 0x30, 0);
                spb(in, 5, 0);
            }
        } else
            spb(cur, 0x18, 0x13);
        spb(in, 8, 0);
        break;
    case 0x19:
        if (pb(in, 8)) {
            spb(in, 8, 0);
            spb(cur, 0x18, 0);
        }
        break;
    default:
        if (bits & 0x10) {
            show_message(-1, side);
            SW(game_flags, GW(game_flags) & (side ? ~8 : ~4));
            spb(cur, 0x17, 1);
            spb(cur, 0x18, 1);
        }
        break;
    }
    return ret;
}

/* T0D36:0BB6: a cursor's record at a map's start */
void t0d36_0bb6(fptr cur)
{
    spb(cur, 0x17, 0), spb(cur, 0x18, 0), spb(cur, 0x19, 0), spb(cur, 0x1A, 0);
    spb(cur, 0x1B, 5);
    spw(cur, 0x22, 0), spw(cur, 0x20, 0), spw(cur, 0x1E, 0), spw(cur, 0x1C, 0);
    spw(cur, 6, 0), spw(cur, 4, 0), spw(cur, 0x0A, 0), spw(cur, 8, 0);
    spw(cur, 0x26, 0), spw(cur, 0x24, 0);
    spb(cur, 0x30, 0);
    spw(cur, 0x2E, 0);
    spb(cur, 0x2C, 0), spb(cur, 0x2D, 0), spb(cur, 0x2B, 0);
    spb(cur, 0x28, 1), spb(cur, 0x29, 2), spb(cur, 0x2A, 1);
}

/* T0D36:0D82: fire on the map.  At its first pass (step 1) what the
 * square offers goes into the cursor's +1Ch: 2, 4, 8 always (down, the
 * status or the unit's screen; right, the overview), 200h the change of
 * phase, 1 an attack or 10h a move of a unit of one's own there (by the
 * phase), 400h a building or a unit that holds others.  While fire is
 * held a direction chooses the cursor's picture; fire let go does what
 * the picture stands for. */
void t0d36_0d82(fptr cur, int side, int bits)
{
    fptr map = side ? GFP(map1) : GFP(map0), u, pl = MKFP(S_players, A_players + 0x17 * side);
    unsigned g = pb(map, pw(cur, 0)), ub = pb(map, pw(cur, 0) + 1), f;
    int sound, n, i;

    if (pb(cur, 0x18) == 1) {
        spw(cur, 0x1C, 0x0E);
        if (!(GW(game_flags2) & 0x80))
            spw(cur, 0x1C, pw(cur, 0x1C) | 0x200);
        if (ub <= 0xF0) {
            f = pw(UNIT(ub), 4);
            if (same_side(side, (int)f) && !(f & 2) && !(f & 0x200)) {
                if (pb(cur, 0x16) & 1) {
                    if (!(GW(game_flags2) & 0x80))
                        spw(cur, 0x1C, pw(cur, 0x1C) | 0x10);
                } else
                    spw(cur, 0x1C, pw(cur, 0x1C) | 1);
            }
            if (f & (0x1000 | 0x2000)) {
                if (!(GW(menu_flags) & 8) || (same_side((int)f, side) && !(f & 2)))
                    spw(cur, 0x1C, pw(cur, 0x1C) | 0x400);
            }
        } else {
            f = pw(GROUND(g), 0);
            if (f & (0x80 | 0x200 | 0x40)) {
                if (!(GW(menu_flags) & 8) || ((side & 1) ? (f & 1) : (f & 4)))
                    spw(cur, 0x1C, pw(cur, 0x1C) | 0x400);
            }
        }
        spb(cur, 0x18, 2);
    }
    if (bits & 0x10) {
        /* fire held: the direction chooses */
        if (bits & 1) {
            if (pw(cur, 0x1C) & 0x10)
                spb(cur, 0x1B, 4);
            else if (pw(cur, 0x1C) & 1)
                spb(cur, 0x1B, 0);
            if (pb(pl, 0x15) >= pb(pl, 0x16) && !(pw(pl, 0) & 2)) {
                spb(cur, 0x1B, 5);
                show_message(0x21, side);
                message_flag(side);
            }
        } else if ((bits & 2) && (pw(cur, 0x1C) & 4))
            spb(cur, 0x1B, 2);
        else if (bits & 4) {
            if (pw(cur, 0x1C) & 0x400)
                spb(cur, 0x1B, 0x0A);
            else if (pw(cur, 0x1C) & 0x200)
                spb(cur, 0x1B, 9);
        } else if ((bits & 8) && (pw(cur, 0x1C) & 8))
            spb(cur, 0x1B, 3);
        else
            spb(cur, 0x1B, 1);
        return;
    }
    switch (pb(cur, 0x1B)) {
    case 4:                             /* a unit to move: its reach */
        n = (int)ub;
        u = UNIT(n);
        if ((pw(u, 4) & 0x40) && !(pw(u, 4) & 0x80))
            n--;
        u = UNIT(n);
        f = (pw(u, 6) & 1) ? 0xFFFF : (pw(u, 4) & 1) ? 6 : 3;
        if (reach((int16_t)(pw(u, 0x0B + 2 * side) - 1), pfp(pl, 0x0D), n, pb(u, 0), side, (int)f, map) > 1) {
            draw_marks(pw(cur, 2), side);
            unit_flag((int)ub, 0x100, side, 1);
            sound = 2;
            spb(cur, 0x17, 0);
            spb(cur, 0x18, 0x16);
            spw(cur, 0x1E, n);
        } else {
            undraw_marks(pw(cur, 2), side, side ? 2 : 1);
            clear_marks(side ? 2 : 1);
            message_flag(side);
            show_message(0x22, side);
            sound = 3;
            spb(cur, 0x17, 0);
            spb(cur, 0x18, 0);
            spb(cur, 0x1B, 5);
        }
        ask_sound(sound);
        break;
    case 2:
        spb(cur, 0x17, 4);
        spb(cur, 0x18, 0);
        spb(cur, 0x1B, 5);
        break;
    case 0:                             /* a unit to fire, or to build */
        u = UNIT(ub);
        sound = 3;
        spb(cur, 0x17, 0);
        spb(cur, 0x18, 0);
        spb(cur, 0x1B, 5);
        if (pw(u, 6) & 0x4000) {
            if ((int8_t)pb(u, 0x0A) <= 0) {
                message_flag(side);
                show_message(0x1A, side);
            } else if (!t0e9b_1704((int16_t)(pw(u, 0x0B + 2 * side) - 1), map)) {
                message_flag(side);
                show_message(0x19, side);
            } else {
                for (i = 0; i < 0x0A; i++) {
                    int a = (int16_t)GWO(around, 2 * i), w = (int16_t)GW(map_width);

                    if (a < 0)
                        continue;
                    a >>= 1;
                    a = a % w + ((a / w) << 6);
                    SBO(marks, (uint16_t)a, GBO(marks, (uint16_t)a) | (side ? 8 : 4));
                }
                goto chosen;
            }
        } else {
            fptr type = TYPE(pb(u, 8));
            unsigned total = 0, range;

            range = pb(type, 8);
            if (range > 1)
                total += fire_reach(pfp(pl, 0x0D), pw(cur, 0), (int)range, side, (int)ub, pw(type, 5) & 0xFFEF, map);
            range = pb(type, 7);
            if (range > 1)
                total += fire_reach(pfp(pl, 0x0D), pw(cur, 0), (int)range, side, (int)ub, pw(type, 5) & 0xFFD3, map);
            if (!(total & 0xFF)) {
                message_flag(side);
                show_message(0x1B, side);
            } else {
chosen:
                spb(cur, 0x17, 0);
                spb(cur, 0x18, 0x13);
                spw(cur, 0x1E, ub);
                spb(cur, 0x1B, 0);
                draw_marks(pw(cur, 2), side);
                unit_flag((int)ub, 0x100, side, 1);
                sound = 2;
            }
        }
        ask_sound(sound);
        break;
    case 0x0A:
        spb(cur, 0x17, 6);
        spb(cur, 0x18, 0);
        break;
    case 9:
        spb(cur, 0x17, 5);
        spb(cur, 0x18, 0);
        break;
    case 3:
        spb(cur, 0x17, 3);
        spb(cur, 0x18, 4);
        spb(cur, 0x1B, 6);
        break;
    default:
        spb(cur, 0x17, 0);
        spb(cur, 0x18, 0);
        spb(cur, 0x1B, 5);
        break;
    }
}

/* T0D36:15E9: `count` bytes copied */
void t0d36_15e9(fptr from, fptr to, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
        mem[(flin(to) + i) & (MEM_SIZE - 1)] = mem[(flin(from) + i) & (MEM_SIZE - 1)];
}
