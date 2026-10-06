/* timer.c - BATTLE.EXE's T2354: its timers on the PIT's interrupt, the
 * keys, and what the keys held mean for each player.
 *
 * The timers are the original's tables in the program's memory (the
 * hints at timer_add have them in full); only the handlers are C, found
 * by the far pointer the program stores (timer_handler).
 */
#include "bi.h"

#define SLOTS 16

static struct { fptr handler; void (*fn)(void); } handlers[SLOTS];
static int nhandlers;
static int own_int09;                   /* T2354:0010 */

void timer_handler(fptr handler, void (*fn)(void))
{
    int i;

    for (i = 0; i < nhandlers; i++)
        if (handlers[i].handler == handler)
            return;
    if (nhandlers == SLOTS)
        bi_fatal("timer_handler: too many handlers");
    handlers[nhandlers].handler = handler;
    handlers[nhandlers].fn = fn;
    nhandlers++;
}

static void call_handler(fptr handler)
{
    int i;

    for (i = 0; i < nhandlers; i++)
        if (handlers[i].handler == handler) {
            handlers[i].fn();
            return;
        }
    /* every timer the port adds has its handler (timer_handler first) */
    bi_fatal("call_handler: a timer without a handler");
}

/* T2354:063C, timer 0: the INT 08h handler that was there before (the
 * BIOS's, which counts its ticks) */
static void old_timer(void)
{
}

/* the shortest period of the timers from `si` down, not above `least` */
static void shortest_period(unsigned si, uint32_t least)
{
    int i;

    for (i = (int)si; i >= 0; i -= 2) {
        uint32_t p = GWO(timers_period_low, i) | (uint32_t)GWO(timers_period_high, i) << 16;
        if (p <= least)
            least = p;
    }
    SW(pit_period_low, least);
    SW(pit_period_high, least >> 16);
}

unsigned timer_add(fptr handler, uint32_t period)
{
    unsigned si = GW(timers_last) + 2u, di;

    SWO(timers_period_low, si, period);
    SWO(timers_period_high, si, period >> 16);
    SWO(timers_left_low, si, period);
    SWO(timers_left_high, si, period >> 16);
    SWO(timers_handler, 2 * si, FOFF(handler));
    SWO(timers_handler, 2 * si + 2, FSEG(handler));
    if (period < (GW(pit_period_low) | (uint32_t)GW(pit_period_high) << 16)) {
        SW(pit_period_low, period);
        SW(pit_period_high, period >> 16);
    }
    for (di = si; !(GWO(timers_handle, di) & 0x8000); di -= 2)
        ;
    SWO(timers_place, si, di);
    SWO(timers_handle, di, si);
    SW(timers_last, si);
    return di;
}

void timer_remove(unsigned handle)
{
    unsigned si = GW(timers_last), bx;

    SW(timers_last, si - 2);
    if (si != GWO(timers_handle, handle)) {
        /* the last timer into the freed place */
        bx = GWO(timers_handle, handle);
        SWO(timers_period_low, bx, GWO(timers_period_low, si));
        SWO(timers_period_high, bx, GWO(timers_period_high, si));
        SWO(timers_left_low, bx, GWO(timers_left_low, si));
        SWO(timers_left_high, bx, GWO(timers_left_high, si));
        SWO(timers_handler, bx, GWO(timers_handler, si));       /* as the original: not 2 * */
        SWO(timers_handler, bx + 2, GWO(timers_handler, si + 2));
        SWO(timers_due, bx, GWO(timers_due, si));
        si = GWO(timers_place, si);
        SW(timers_handle, si);                                  /* as the original: handle 0's */
    }
    SWO(timers_handle, handle, 0x8000);
    shortest_period(GW(timers_last), 0x10000);
}

void timer_period(unsigned handle, uint32_t period)
{
    unsigned si = GWO(timers_handle, handle);

    SWO(timers_period_low, si, period);
    SWO(timers_period_high, si, period >> 16);
    shortest_period(GW(timers_last), period);
}

void int08(void)
{
    int si;
    uint32_t run = GW(pit_run_low) | (uint32_t)GW(pit_run_high) << 16;

    for (si = (int)GW(timers_last); si >= 0; si -= 2) {
        uint32_t left = GWO(timers_left_low, si) | (uint32_t)GWO(timers_left_high, si) << 16;

        left -= run;
        if ((left & 0x80000000u) || left == 0) {
            SWO(timers_due, si, 0x8000);
            left += GWO(timers_period_low, si) | (uint32_t)GWO(timers_period_high, si) << 16;
        }
        SWO(timers_left_low, si, left);
        SWO(timers_left_high, si, left >> 16);
    }
    SW(pit_run_low, GW(pit_period_low));
    SW(pit_run_high, GW(pit_period_high));
    for (si = (int)GW(timers_last); si >= 0; si -= 2)
        if (GWO(timers_due, si) & 0x8000) {
            SWO(timers_due, si, 0);
            call_handler(MKFP(GWO(timers_handler, 2 * si + 2), GWO(timers_handler, 2 * si)));
        }
    /* the game's handler writes the PIT only when the period changed
     * (pit_period_set), the intro's (T0559:0590) every time: the same to
     * clock_set_period */
    if (bi_prog == BI_INTRO) {
        clock_set_period(GW(pit_period_low));
        return;
    }
    if (GW(pit_period_set) != GW(pit_period_low))
        clock_set_period(GW(pit_period_low));
    SW(pit_period_set, GW(pit_period_low));
}

/* T2354:05BE: the tables, timer 0 the old interrupt at the BIOS's rate,
 * and the interrupt */
static void t2354_05be(void)
{
    int si;

    SWO(timers_handler, 0, A_old_timer);
    SWO(timers_handler, 2, S_old_timer);
    SW(pit_run_low, 0);
    SW(pit_run_high, 1);
    SW(pit_period_low, 0);
    SW(pit_period_high, 1);
    SWO(timers_period_low, 0, 0);
    SWO(timers_period_high, 0, 1);
    SWO(timers_left_low, 0, 0);
    SWO(timers_left_high, 0, 1);
    for (si = 0x1E; si >= 0; si -= 2)
        SWO(timers_handle, si, 0x8000);
    SWO(timers_handle, 0, 0);
    SWO(timers_place, 0, 0);
    SW(timers_last, 0);
    timer_handler(FP(old_timer), old_timer);
    clock_set_period(0);
}

/* T2354:0268 on: the keys of a player's table that are down, then what
 * they come to.  now: left, right, up, down, fire (FFh); events: +0..+3
 * the directions last held, +4 fire held, +5 fire pressed twice quickly
 * (as read), +6..+8 its state; counts: five pairs of words */
static void input_events(unsigned now, unsigned before, unsigned counts, unsigned events,
                         unsigned table, int from_keys)
{
    unsigned i, si, bx = counts;

    if (from_keys) {
        for (si = table; rb((uint16_t)si); si += 7) {
            unsigned code = rb((uint16_t)si);
            if (!(rb((uint16_t)(A_keys_up + ((code & 0x7F) >> 3))) & (0x80 >> (code & 7))))
                for (i = 0; i < 5; i++)
                    wb((uint16_t)(now + i), rb((uint16_t)(now + i)) | rb((uint16_t)(si + 1 + i)));
        }
        if (bi_prog != BI_INTRO && GW(mouse_on) && GW(mouse_record) == events)
            mouse_input(now);
        if (rb((uint16_t)now))
            ww((uint16_t)(counts + 4), 0);
        if (rb((uint16_t)(now + 1)))
            ww((uint16_t)counts, 0);
        if (rb((uint16_t)(now + 2)))
            ww((uint16_t)(counts + 0x0C), 0);
        if (rb((uint16_t)(now + 3)))
            ww((uint16_t)(counts + 8), 0);
    }
    if (!rb((uint16_t)(now + 4)))
        wb((uint16_t)(events + 8), 0xFF);
    if (rw((uint16_t)now) | rw((uint16_t)(now + 2))) {
        ww((uint16_t)events, rw((uint16_t)now));
        ww((uint16_t)(events + 2), rw((uint16_t)(now + 2)));
    }
    if (rb((uint16_t)(now + 4))) {
        wb((uint16_t)(events + 4), 0xFF);
        if (rsw((uint16_t)(counts + 0x10)) < 0x19) {
            if (!rb((uint16_t)(events + 7))) {
                wb((uint16_t)(events + 7), 0xFF);
                wb((uint16_t)(events + 6), 0);
            }
            if (rb((uint16_t)(events + 6))) {
                wb((uint16_t)(events + 5), 0xFF);
                wb((uint16_t)(events + 7), 0);
                wb((uint16_t)(events + 6), 0);
            }
        } else {
            wb((uint16_t)(events + 7), 0);
            wb((uint16_t)(events + 6), 0);
        }
    } else if (rw((uint16_t)(counts + 0x10)))
        wb((uint16_t)(events + 6), 0xFF);
    else
        wb((uint16_t)(events + 7), 0);
    for (i = 0; i < 5; i++, bx += 4) {
        if (rb((uint16_t)(now + i)) && rb((uint16_t)(before + i))) {
            ww((uint16_t)bx, rw((uint16_t)bx) + 1);
            continue;
        }
        ww((uint16_t)(bx + 2), rw((uint16_t)(bx + 2)) + 1);
        if (rsw((uint16_t)(bx + 2)) > 0x0A) {
            ww((uint16_t)bx, 0);
            ww((uint16_t)(bx + 2), 0);
        }
    }
}

/* T2354:013F and 01D8: a player's input, every fourth tick.  The
 * joysticks' part (the axes against the middle's limits) is as read; the
 * port has none there (joy0_there, joy1_there stay 0). */
static void player_input(int player)
{
    unsigned now = player ? A_input1_now : A_input0_now;
    unsigned before = player ? A_input1_before : A_input0_before;
    unsigned counts = player ? A_input1_counts : A_input0_counts;
    unsigned events = player ? A_input1_events : A_input0_events;
    unsigned table = player ? GW(keys1_table) : GW(keys0_table);
    unsigned used = player ? GW(joy1_used) : GW(joy0_used);
    unsigned there = player ? GW(joy1_there) : GW(joy0_there);
    unsigned axes = A_joy_axes + (player ? 6u : 0u);        /* x, y, buttons */
    unsigned limits = A_joy_middle + (player ? 0x18u : 8u); /* left, right, up, down */

    ww((uint16_t)before, rw((uint16_t)now));
    ww((uint16_t)(before + 2), rw((uint16_t)(now + 2)));
    wb((uint16_t)(before + 4), rb((uint16_t)(now + 4)));
    ww((uint16_t)now, 0);
    ww((uint16_t)(now + 2), 0);
    ww((uint16_t)(now + 4), 0);
    if (used && there) {
        int16_t x = rsw((uint16_t)axes), y = rsw((uint16_t)(axes + 2));

        if (x <= rsw((uint16_t)limits)) {
            wb((uint16_t)now, 0xFF);
            ww((uint16_t)(counts + 4), 0);
        } else if (x >= rsw((uint16_t)(limits + 2))) {
            wb((uint16_t)(now + 1), 0xFF);
            ww((uint16_t)counts, 0);
        }
        if (y <= rsw((uint16_t)(limits + 4))) {
            wb((uint16_t)(now + 2), 0xFF);
            ww((uint16_t)(counts + 0x0C), 0);
        } else if (y >= rsw((uint16_t)(limits + 6))) {
            wb((uint16_t)(now + 3), 0xFF);
            ww((uint16_t)(counts + 8), 0);
        }
        wb((uint16_t)(now + 4), rb((uint16_t)(axes + 4)));
        if (rw((uint16_t)now) | rw((uint16_t)(now + 2)) | rb((uint16_t)(now + 4))) {
            input_events(now, before, counts, events, table, 0);
            return;
        }
    }
    input_events(now, before, counts, events, table, 1);
}

/* the timer's callback at 4000h counts, 72.8 a second: the count of
 * ticks, a key from the BIOS (or the last scancode of the program's own
 * INT 09h), and every fourth time the players' input */
static void timer_keys(void)
{
    unsigned k;

    SW(tick_count, GW(tick_count) + 1);
    if (bios_key_waits()) {
        k = bios_key();
        SBO(key_scan, 1, GB(key_scan));
        SBO(key_char, 1, GB(key_char));
        SB(key_scan, k >> 8);
        SB(key_char, k);
        SW(key_there, 0xFFFF);
    } else if (GB(last_scancode) && !(GB(last_scancode) & 0x80)) {
        SBO(key_scan, 1, GB(key_scan));
        SBO(key_char, 1, GB(key_char));
        SB(key_scan, GB(last_scancode));
        SB(key_char, 0);
        SW(key_there, 0xFFFF);
        SB(last_scancode, 0);
    }
    /* the mouse; never on in the intro, whose names for it are not checked */
    mouse_window(bi_prog != BI_INTRO && GW(mouse_on));
    if (bi_prog != BI_INTRO && GW(mouse_on) && GW(mouse_by_timer) == 1)
        mouse_read();
    SW(input_divider, GW(input_divider) + 1);
    if ((int16_t)GW(input_divider) < 4)
        return;
    /* the joysticks read (T2670:000A) when one is used: none */
    player_input(0);
    player_input(1);
    SW(input_divider, 0);
}

/* T265E:000E: looks for joysticks at port 201h; the port's answers as a
 * PC without one (FFh): two retraces' wait and nothing found */
void t265e_000e(void)
{
    SW(joy0_there, 0);
    SW(joy1_there, 0);
    SB(joy_bits, 0);
    SW(joy_wait, 0xFFFF);
    SWO(joy_middle, 0, 0);
    SWO(joy_middle, 2, 0);
    SWO(joy_middle, 0x10, 0);
    SWO(joy_middle, 0x12, 0);
    SWO(joy_middle, 4, 0);
    SWO(joy_middle, 6, 0);
    SWO(joy_middle, 0x14, 0);
    SWO(joy_middle, 0x16, 0);
    wait_retrace();
    wait_retrace();
    SW(joy_wait, 0);
}

void t2354_0011(void)
{
    t2354_05be();
    timer_handler(FP(timer_keys), timer_keys);
    timer_add(FP(timer_keys), 0x4000);
    t265e_000e();
    own_int09 = 0;
}

void t2354_005e(void)
{
    clock_set_period(0);
}

/* the program's INT 09h: the scancode kept, and a bit per key, set while
 * the key is up */
void int09(unsigned char scancode)
{
    unsigned at = A_keys_up + ((scancode & 0x7Fu) >> 3), bit = 0x80u >> (scancode & 7);

    SB(last_scancode, scancode);
    wb((uint16_t)at, (uint8_t)((rb((uint16_t)at) & ~bit) | ((scancode & 0x80) ? bit : 0)));
}

void t2354_0530(void)
{
    if (own_int09)
        return;
    own_int09 = 1;
    dos_keyboard_own(1);
}

void t2354_056f(void)
{
    if (!own_int09)
        return;
    own_int09 = 0;
    dos_keyboard_own(0);
    SB(last_scancode, 0);
}
