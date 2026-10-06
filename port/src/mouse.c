/* mouse.c - the mouse: BATTLE.EXE's T2683, T263D, T267C and T268A and the
 * two routines of T2354 that turn it into a player's directions and fire
 * (the hints at mouse_start have them in full), and the driver they ask
 * (INT 33h), which is the port's own.
 *
 * The original takes the mouse as a joystick: the driver's place against
 * 160, 100 gives the directions, the place is set back there after each,
 * the left button is fire.
 *
 * The driver here answers as doskit's runner's does (tools/run/mouse.c),
 * so that a run of the port can be compared with one of the original: a
 * place within 0..639 and 0..199, three buttons, the sensitivity 50, 50
 * and 64; after a reset the place is the game's middle, 160, 100, not
 * the runner's half of the limits (drv_reset says why).  It is there only when asked
 * for: BI_MOUSE=1, or places scripted with BI_MOUSEAT (dos.c, bi_at).
 * Without that the port is a PC without a mouse, as before; the window's
 * mouse is not connected (platform.h gives the buttons pressed, not
 * held, and a place on the picture, not a movement).
 */
#include <stdlib.h>
#include "bi.h"

/* the mouse's variables without a name of their own, by their place
 * after mouse_on (DATA:040A in BATTLE.EXE; MOON.EXE's are the same) */
#define M_DRAWN 0x08            /* a pointer was drawn */
#define M_HELD 0x0A             /* the held rates are set */
#define M_LAST_X 0x14
#define M_LAST_Y 0x16
#define M_RATE_X 0x18
#define M_RATE_Y 0x1A
#define M_HELD_RATE_X 0x1C
#define M_HELD_RATE_Y 0x1E
#define M_THRESHOLD 0x20
#define M_HELD_THRESHOLD 0x22
#define M_DOUBLE 0x24
#define M_BUTTONS 0x2E
#define M_MIDDLE 0x32
#define M_SENS 0x3C             /* three words kept for mouse_stop */
#define M_COUNT 0x42

/* ---- the driver (the port's own) ---- */

static struct {
    int asked, there;
    unsigned x, y, buttons;
    unsigned rate_x, rate_y, sens_x, sens_y, threshold;
} drv;

static int drv_there(void)
{
    if (!drv.asked) {
        const char *e = getenv("BI_MOUSE");

        drv.asked = 1;
        drv.there = (e && *e && *e != '0') || getenv("BI_MOUSEAT");
    }
    return drv.there;
}

/* a place and the buttons from outside (bit 0 left, 1 right, 2 middle) */
void bi_mouse_place(int x, int y, int buttons)
{
    if (!drv_there())
        return;
    drv.x = x < 0 ? 0 : x > 639 ? 639 : (unsigned)x;
    drv.y = y < 0 ? 0 : y > 199 ? 199 : (unsigned)y;
    drv.buttons = (unsigned)buttons & 7;
}

/* AX=0: FFFFh and the buttons' number, or 0 as without a driver */
static unsigned drv_reset(unsigned *buttons)
{
    if (!drv_there())
        return 0;
    /* Not the runner's driver here: that one puts the place at half its
     * limits (319, 99), which the game reads as "right" once and then
     * sets to its own middle.  In the original that reading falls into
     * the menu or the loading before a map and nothing shows; the port's
     * clock stands while files load, so it would fall into the map's
     * first passes and move the cursor.  The place starts at the game's
     * middle instead: from the first reading on both are at 160, 100. */
    drv.x = 0xA0;
    drv.y = 0x64;
    drv.buttons = 0;
    drv.rate_x = 8;
    drv.rate_y = 16;
    drv.sens_x = drv.sens_y = 50;
    drv.threshold = 64;
    *buttons = 3;
    return 0xFFFF;
}

/* AX=4 */
static void drv_set_place(unsigned x, unsigned y)
{
    drv.x = x > 639 ? 639 : x;
    drv.y = y > 199 ? 199 : y;
}

/* AX=0Fh: the mickeys for 8 pixels; 0 leaves one as it is */
static void drv_set_rates(unsigned x, unsigned y)
{
    if (x)
        drv.rate_x = x;
    if (y)
        drv.rate_y = y;
}

/* ---- the original's routines ---- */

/* T263D:000A: the place and the buttons from the driver */
int mouse_read(void)
{
    if (!GW(mouse_on))
        return -1;
    SW(mouse_x, drv.x);
    SW(mouse_y, drv.y);
    SW(mouse_left, (drv.buttons & 1) ? 0xFFFF : 0);
    SW(mouse_right, (drv.buttons & 2) ? 0xFFFF : 0);
    SWO(mouse_on, M_MIDDLE, (drv.buttons & 4) ? 0xFFFF : 0);
    /* with mouse_shown the original exchanges the pages twice with nothing
     * between and notes a pointer as drawn; no caller asks for it */
    if (GW(mouse_shown)
        && (GWO(mouse_on, M_LAST_X) != GW(mouse_x) || GWO(mouse_on, M_LAST_Y) != GW(mouse_y))) {
        SWO(mouse_on, M_DRAWN, 0xFFFF);
        SWO(mouse_on, M_LAST_X, GW(mouse_x));
        SWO(mouse_on, M_LAST_Y, GW(mouse_y));
    }
    return 0;
}

/* T2683:000A: the mouse on for the player whose events record is at
 * `record`; 1 when it is on already, -1 without a driver */
int mouse_start(int by_timer, unsigned record, int shown)
{
    unsigned buttons = 0;

    if (GW(mouse_on))
        return 1;
    if (drv_reset(&buttons) != 0xFFFF)
        return -1;
    SWO(mouse_on, M_BUTTONS, buttons);
    SW(mouse_on, 0xFFFF);
    SWO(mouse_on, M_LAST_X, 0xFFFF);
    SWO(mouse_on, M_LAST_Y, 0xFFFF);
    SW(mouse_by_timer, by_timer);
    SW(mouse_record, record);
    SW(mouse_shown, shown);
    SWO(mouse_on, M_SENS, drv.sens_x);          /* AX=1Bh */
    SWO(mouse_on, M_SENS + 2, drv.sens_y);
    SWO(mouse_on, M_SENS + 4, drv.threshold);
    return mouse_read();
}

/* T267C:000E: the mouse off */
int mouse_stop(void)
{
    if (!GW(mouse_on))
        return -1;
    if (GWO(mouse_on, M_DRAWN)) {
        unsigned shown = GW(page_shown);

        SW(page_shown, GW(page_drawn));
        SW(page_drawn, shown);
        t24d3_0000(GFP(pointer_store));
        shown = GW(page_shown);
        SW(page_shown, GW(page_drawn));
        SW(page_drawn, shown);
    }
    drv.sens_x = GWO(mouse_on, M_SENS);         /* AX=1Ah */
    drv.sens_y = GWO(mouse_on, M_SENS + 2);
    drv.threshold = GWO(mouse_on, M_SENS + 4);
    SW(mouse_on, 0);
    SWO(mouse_on, M_DRAWN, 0);
    return 0;
}

/* T268A:0006: the driver's rates, those for while the left button is
 * held, and the distance from the middle that is a direction */
int mouse_rates(int x, int y, int held_x, int held_y, int doubled, int threshold,
                int held_threshold)
{
    if (!GW(mouse_on))
        return -1;
    SWO(mouse_on, M_RATE_X, x);
    SWO(mouse_on, M_RATE_Y, y);
    drv_set_rates((unsigned)x, (unsigned)y);
    SWO(mouse_on, M_DOUBLE, doubled);
    drv.threshold = doubled ? (unsigned)doubled : 64;   /* AX=13h */
    SWO(mouse_on, M_HELD_RATE_X, held_x);
    SWO(mouse_on, M_HELD_RATE_Y, held_y);
    SWO(mouse_on, M_THRESHOLD, threshold);
    SW(mouse_threshold, threshold);
    SWO(mouse_on, M_HELD_THRESHOLD, held_threshold);
    return 0;
}

/* the rates by the menu's speed (0, 1, 2), as every caller but the
 * start's passes them */
void mouse_rates_by_speed(void)
{
    int speed = (int8_t)GB(mouse_speed), n = speed == 0 ? 0x32 : speed == 1 ? 0x0A : 3;

    mouse_rates(n, n, n, n, 0x0FA0, 0x28, 0x28);
}

/* T2354:04A0: the place back to the middle, with no button down */
static void mouse_centre(void)
{
    if (GW(mouse_right) || GW(mouse_left))
        return;
    SW(mouse_x, 0xA0);
    SW(mouse_y, 0x64);
    drv_set_place(0xA0, 0x64);
}

/* T2354:0397: the mouse as a player's directions and fire; `now` is the
 * pass's five bytes left, right, up, down, fire */
void mouse_input(unsigned now)
{
    int d;

    if (!GW(mouse_by_timer))
        mouse_read();
    if (GW(mouse_left)) {
        if (!GWO(mouse_on, M_HELD)) {
            drv_set_rates(GWO(mouse_on, M_HELD_RATE_X), GWO(mouse_on, M_HELD_RATE_Y));
            SWO(mouse_on, M_HELD, 0xFFFF);
            SW(mouse_threshold, GWO(mouse_on, M_HELD_THRESHOLD));
        }
    } else if (GWO(mouse_on, M_HELD)) {
        drv_set_rates(GWO(mouse_on, M_RATE_X), GWO(mouse_on, M_RATE_Y));
        SWO(mouse_on, M_HELD, 0);
        SW(mouse_threshold, GWO(mouse_on, M_THRESHOLD));
        mouse_centre();
        if (GW(mouse_right))
            SWO(mouse_on, M_COUNT, 0x0A);
        return;
    } else if (GWO(mouse_on, M_COUNT)) {
        SWO(mouse_on, M_COUNT, GWO(mouse_on, M_COUNT) - 1);
        if (GWO(mouse_on, M_COUNT) != 9 && GWO(mouse_on, M_COUNT))
            return;
    }
    if (GW(mouse_left) & GW(mouse_right))
        return;
    d = (int16_t)(GW(mouse_x) - 0xA0);
    if (d > 0) {
        if (d >= (int16_t)GW(mouse_threshold)) {
            wb((uint16_t)(now + 1), 0xFF);
            mouse_centre();
        }
    } else if (d < 0 && (int16_t)-d >= (int16_t)GW(mouse_threshold)) {
        wb((uint16_t)now, 0xFF);
        mouse_centre();
    }
    d = (int16_t)(GW(mouse_y) - 0x64);
    if (d > 0) {
        if (d >= (int16_t)GW(mouse_threshold)) {
            wb((uint16_t)(now + 3), 0xFF);
            mouse_centre();
        }
    } else if (d < 0 && (int16_t)-d >= (int16_t)GW(mouse_threshold)) {
        wb((uint16_t)(now + 2), 0xFF);
        mouse_centre();
    }
    if (GW(mouse_left))
        wb((uint16_t)(now + 4), 0xFF);
}
