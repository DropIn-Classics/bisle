/* prog.c - where the hints' names are in the program that is loaded: the
 * game (BATTLE.EXE, DESERT.EX2, MOON.EXE) or the intro (INTEGA/INTRO.EXE).  The
 * tables come from gen/names.h (symmap.py), one column a program. */
#include "bi.h"

#define X(seg, name, ...) uint16_t A_##name, S_##name;
BI_NAMES(X)
#undef X

#define X(seg, name, ...) static const uint16_t pa_##name[] = { __VA_ARGS__ };
BI_NAMES(X)
#undef X

#define X(seg, name, ...) static const uint16_t pf_##name[] = { __VA_ARGS__ };
BI_FRAMES(X)
#undef X

int bi_prog;

void bi_program(int prog)
{
    bi_prog = prog;
#define X(seg, name, ...) \
    A_##name = pa_##name[prog]; \
    S_##name = (uint16_t)(pf_##name[prog] + LOAD_SEG);
    BI_NAMES(X)
#undef X
}

uint16_t bi_data_seg(int prog)
{
    return (uint16_t)((prog == BI_INTRO ? INTEGA_DATA : prog == BI_MOON ? MOON_DATA : BATTLE_DATA) + LOAD_SEG);
}
