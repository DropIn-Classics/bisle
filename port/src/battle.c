/* battle.c - BATTLE.EXE's T0708: the program. */
#include <stdio.h>
#include <string.h>
#include "bi.h"

/* T0D36:1592: the keyboard and the timers put back, a message, the end */
void fatal_error(int number)
{
    char text[64];

    t2354_056f();
    t2354_005e();
    t261e_0008();
    snprintf(text, sizeof text, "ERROR: %s", fstr(FP(fatal_messages)) + 0x14 * (number - 1));
    bi_fatal(text);
}

/* main: the switches /s (the PC speaker's sound) and /m (another text
 * than "Color."; what else it is for is not known) */
void battle_main(int argc, char **argv)
{
    fptr work, p;
    int speaker = 0, colour = 1, i;

    t2354_0011();
    SW(game_flags, 0);
    SW(menu_flags, 0);
    SWO(disk_record, 0, 1);
    SWO(disk_record, 2, 0);
    t164d_0482(1);
    for (i = 1; i < argc; i++)
        if (argv[i][0] == '/') {
            if (argv[i][1] == 's' || argv[i][1] == 'S')
                speaker = 1;
            if (argv[i][1] == 'm' || argv[i][1] == 'M')
                colour = 0;
        }
    fwb(SEG(F2740), 0x02C8, colour ? 0 : 2);
    sound_init(speaker);
    t0d36_000f(0x32);
    work = t2619_0004(0x493E0);
    if (work == 0)
        fatal_error(1);
    p = work;
    if (t2485_0001(0x140, 0xC8, 4, 0x2518, 0, 1))
        fatal_error(3);
    SW(draw_colour, 0);
    clear_page();
    flip_page();
    clear_page();
    SFP(font_ptr, p);
    p = hadd(p, 0xBF4);
    work = hadd(work, 0xBF4);
    if (!load_file(GFP(font_ptr), make_path(1, -1, FP(name_char6), 5), work))
        fatal_error(2);
    if (title(work))
        fatal_error(2);
    SWO(players, 0x17, 0);
    SWO(players, 0, 0);
    SD(score_now, 0);
    SW(map_number, 0);
    stop_song();
    t2354_0530();
    SW(game_flags, GW(game_flags) | 1);
    SW(game_flags, GW(game_flags) & ~0x10);
    /* until fire is let go */
    do {
        SBO(input0_events, 4, 0);
        t0d36_000f(5);
    } while (GBO(input0_events, 4));
    t2354_056f();
    SW(one_page, 0);
    t2593_0006();
    flip_page();
    /* the mouse (T267C:000E, T2683:000A, T268A:0006): the port has none */
    bi_at("menu");
    bi_todo("the menu");
}
