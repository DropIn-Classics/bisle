/* orders.c - BATTLE.EXE's T0B70 and T11FD: an order given in the attack
 * phase, and the line below a player's window (a message, a unit).
 */
#include <string.h>
#include "bi.h"

/* message `number` of the table (18h bytes each) on the line below the
 * window of `side`, centred, and the player's bit 4 or 8 of game_flags;
 * -1 only clears the line */
void show_message(int number, int side)
{
    int x0 = side ? 0xA0 : 0;
    fptr text;

    SW(draw_x1, x0 + 5);
    SW(draw_y1, 0xBD);
    SW(draw_x2, x0 + 0x94);
    SW(draw_y2, 0xC4);
    SW(draw_colour, GBO(amok, 0x0F));
    fill_rect();
    if (number == -1)
        return;
    text = MKFP(S_messages, A_messages + 0x18 * number);
    SW(draw_colour, side ? 0x12 : 3);
    draw_chars(x0 + 0x4C - 3 * (int)strlen(fstr(text)), 0xBD, text);
    SW(game_flags, GW(game_flags) | (side ? 8 : 4));
}

/* the line of a unit: its count, EXP.LIB's entry for its experience, its
 * serial with st, nd, rd or th (not for a unit of nobody), its type's
 * second name */
void unit_line(int unit, int side)
{
    fptr u = UNIT(unit & 0xFF), type = TYPE(pb(u, 8));
    int x0 = side ? 0xA5 : 5, x;
    unsigned n;

    show_message(-1, side);
    SW(draw_colour, side ? 0x12 : 3);
    draw_number((pw(type, 0x0E) & 4) ? pb(u, 3) : pb(u, 2), x0 + 6, 0xBD);
    n = pb(u, 1);
    if (n > 6)
        n = 6;
    if (n > 0) {
        SW(clip_y2, 0xC8);
        draw_entry(x0 + 0x0E, 0xBD, pfp(pfp(FP(lib_exp), 0x0E), 4 * (n - 1)), 0, 0, 0);
        SW(clip_y2, 0xB3);
    }
    if (!(pw(u, 4) & 2)) {
        n = pb(u, 9);
        x = x0 + 0x20;
        if (n / 10 > 0)
            x -= 6;
        if (n / 100 > 0)
            x -= 6;
        draw_number((int)n, x, 0xBD);
        n = n > 3 ? 3 : (n - 1) & 0xFF;
        draw_chars(x0 + 0x26, 0xBD, MKFP(S_ordinals, A_ordinals + 3 * n));
    }
    draw_chars(x0 + 0x34, 0xBD, MKFP(FSEG(type), FOFF(type) + 0x2B));
}

/* Fire on a marked square with a unit chosen to fire: the order to
 * attack the unit there (the unit's +11h the square, +13h 9, +14h 2, the
 * unit into the player's list of orders), or for the pioneers (4000h in
 * their +6) the order to build: 0Bh a building site, 0Dh a depot on a
 * site.  A message says what came of it; 1 when the order is given. */
int give_order(fptr cur, fptr map, int side)
{
    fptr u = UNIT(pw(cur, 0x1E)), pl = MKFP(S_players, A_players + 0x17 * side);
    int kind = 0, message = 0x13, sound = 0xFF, given = 0;
    int w = (int16_t)GW(map_width), sq = (int16_t)pw(cur, 0) >> 1;

    if (pw(u, 6) & 0x4000)
        kind = 0x0C;
    else if (pb(map, pw(cur, 0) + 1) <= 0xF0)
        kind = 9;
    if (!(GBO(marks, (uint16_t)(sq % w + ((sq / w) << 6))) & (side ? 0xAA : 0x55) & 0x0C)) {
        kind = 0;
        message = 0x13;
    }
    if (kind == 0x0C) {
        if (pb(map, pw(cur, 0)) != GBO(amok, 0x18) || GB(depots_made) < 0x0A) {
            unit_flag(pb(cur, 0x1E), 0x200, side, 1);
            spw(u, 0x11, pw(cur, 0));
            spb(u, 0x13, pb(map, pw(cur, 0)) == GBO(amok, 0x18) ? 0x0D : 0x0B);
            message = 0x14;
            sound = 2;
            given = 1;
        } else
            message = 0x15;
    } else if (kind == 9) {
        unit_flag(pb(cur, 0x1E), 0x200, side, 1);
        spw(u, 0x11, pw(cur, 0));
        spb(u, 0x14, 2);
        spb(u, 0x13, 9);
        spb(pfp(pl, 0x11), pb(pl, 0x15), pb(cur, 0x1E));
        spb(pl, 0x15, pb(pl, 0x15) + 1);
        message = 0x17;
        sound = 2;
        given = 1;
    }
    show_message(message, side);
    SW(game_flags2, GW(game_flags2) | (side ? 0x10 : 8));
    if (sound != 0xFF) {
        SW(game_flags2, GW(game_flags2) | 0x100);
        SW(number_asked, sound);
    }
    return given;
}

/* the targets' marks taken off the screen and the unit chosen let go */
void order_release(fptr cur, int side)
{
    int bit = side ? 8 : 4;

    undraw_marks(pw(cur, 2), side, bit);
    clear_marks(bit);
    draw_marks(pw(cur, 2), side);
    unit_flag(pb(cur, 0x1E), 0x100, side, 0);
}
