/* sound.c - BATTLE.EXE's sound (CODE:0215..1B79).  Not translated yet:
 * the songs are loaded where the original loads them, nothing plays. */
#include "bi.h"

int sound_init(int speaker)
{
    return !speaker;
}

int load_song(fptr dest, fptr name, fptr work)
{
    return load_file(dest, name, work) != 0;
}

void play_song(int a, int b)
{
    (void)a;
    (void)b;
}

void stop_song(void)
{
}
