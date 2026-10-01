/* phase.c - BATTLE.EXE's T0408: the change of phase. */
#include "bi.h"

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
