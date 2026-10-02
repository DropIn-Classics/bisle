/* fight.c - BATTLE.EXE's T1F3C..T223C: the fight scene.  change_phase
 * calls fight_step once a picture for each attack order: the first call
 * reckons the fight (fight_reckon: the two units' counts after it), the
 * first two draw the ground (once a page), then the units come in along
 * their scripts, then the shots fly and explode; 1 when it is over.
 * tools/fight.py and tools/scene.py have the rules in full.
 *
 * The scene is in the attacker's half of the screen.  Its values are in
 * F2D37: a unit of the scene is 10h bytes (+0 the direction, +1 hit, +2
 * its place in the script, +6 how far to go, +8 x, +0Ah y, +0Ch, +0Eh
 * where drawn), a shot 1Ch (+0 x, +2 y, +4 flags: 1 flying, 2 exploding,
 * 8 over, 4 the attacker's, 10h hits, 20h not drawn, 40h its sound
 * begun; +5 its entry, +9 the explosion's step, +0Ah, +0Ch the way's
 * signs, +0Eh, +10h its widths, +12h, +14h what is left over, +16h the
 * steps, +18h those done, +1Ah the unit it flies to, +1Bh the calls it
 * waits).
 */
#include "bi.h"

#define REC GFP(scene_record)
#define UNIT_A pfp(REC, 0)
#define UNIT_B pfp(REC, 4)
#define TYPE_A pfp(REC, 0x0A)
#define TYPE_B pfp(REC, 0x0E)
#define SCENE_A(i) MKFP(S_scene_units_a, A_scene_units_a + 0x10 * (i))
#define SCENE_B(i) MKFP(S_scene_units_b, A_scene_units_b + 0x10 * (i))
#define SHOT_A(i) MKFP(S_scene_shots_a, A_scene_shots_a + 0x1C * (i))
#define SHOT_B(i) MKFP(S_scene_shots_b, A_scene_shots_b + 0x1C * (i))

/* the library's long division: a divisor of 0 ended the program */
static int32_t ldiv32(int32_t a, int32_t b)
{
    if (!b)
        bi_fatal("Divide error");
    return a / b;
}

static int32_t lmul32(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }

/* a sound asked for on a channel and started */
static void sound(unsigned ch, unsigned number, unsigned volume, unsigned how)
{
    fptr e = pfp(REC, 0x4A);

    spw(e, 6 * ch, number);
    spw(e, 6 * ch + 2, volume);
    spw(e, 6 * ch + 4, how);
    effects_start();
}

/* the scene's half: 160 for an attacker of player 1 */
static int half(void) { return (pw(UNIT_A, 4) & 1) ? 0xA0 : 0; }

/* ---- T2112: the ground and the frame ---- */

static void frame_above(void) { draw_entry(half(), 0, pfp(pfp(REC, 0x3E), 0), 0, 0, 0x40); }
static void frame_below(void) { draw_entry(half(), 0xA8, pfp(pfp(REC, 0x3E), 8), 0, 0, 0x40); }
static void frame_left(void) { draw_entry(half(), 0, pfp(pfp(REC, 0x3E), 0x0C), 0, 0, 0x40); }
static void frame_right(void) { draw_entry(half() + 0x90, 0, pfp(pfp(REC, 0x3E), 4), 0, 0, 0x40); }

static void frame_again(void)
{
    unsigned bits = GB(scene_frame_bits);

    if (bits & 1)
        frame_above();
    if (bits & 2)
        frame_below();
    if (bits & 4)
        frame_left();
    if (bits & 8)
        frame_right();
}

/* a ground's list of pieces by its record's +5 (1..7): a count, then
 * entry, x, y a piece */
static fptr pieces(unsigned kind)
{
    static const uint8_t at[7] = {0x06, 0x0D, 0x14, 0x1B, 0x22, 0x3B, 0x54};

    return kind >= 1 && kind <= 7 ? MKFP(S_scene_pieces, A_scene_pieces + at[kind - 1]) : 0;
}

static void piece(int x, int y, unsigned entry)
{
    draw_packed(x, y, pfp(pfp(REC, 0x42), 4 * entry), 0, 0, 0x5F, pfp(REC, 0x46));
}

static void draw_pieces(fptr list, int x0, int dy)
{
    unsigned n, i;

    if (!list)
        return;
    n = pb(list, 0);
    for (i = 0; i < n; i++)
        piece(x0 + pb(list, 2 + 3 * i), pb(list, 3 + 3 * i) + dy, pb(list, 1 + 3 * i));
}

/* the target's ground above, the attacker's below; between them a black
 * ragged band for a fight over a distance, else a piece where the two
 * grounds differ */
static void scene_ground(int side, int far_apart)
{
    int x0 = side ? 0xA0 : 0, x;
    unsigned ka = pb(pfp(REC, 0x12), 5), kb = pb(pfp(REC, 0x16), 5);

    draw_pieces(pieces(kb), x0, 0);
    draw_pieces(pieces(ka), x0, 0x5A);
    if (far_apart) {
        bi_srand(pw(REC, 0));
        for (x = 0; x < 0x98; x += 2) {
            int h = bi_random(8, 0x0E);

            SW(draw_x1, x0 + x);
            SW(draw_y1, 0x5A - (h >> 1));
            SW(draw_y2, GW(draw_y1) + h);
            SW(draw_colour, 0);
            draw_column();
            SW(draw_x1, x0 + x + 1);
            draw_column();
        }
        return;
    }
    if (kb != 2 && kb != 5 && (ka == 2 || ka == 5)) {
        piece(x0, 0x58, 3);
        piece(x0 + 0x4C, 0x58, 3);
    }
    if (ka != 2 && ka != 5 && (kb == 2 || kb == 5)) {
        piece(x0, 0x50, 4);
        piece(x0 + 0x4C, 0x50, 4);
    }
    if (kb == 7 && ka != 7) {
        piece(x0, 0x50, 7);
        piece(x0 + 0x4C, 0x50, 7);
    }
    if (ka == 7 && kb != 7) {
        piece(x0, 0x58, 8);
        piece(x0 + 0x4C, 0x58, 8);
    }
}

/* ---- T2190: the reckoning ---- */

static int32_t count_of(fptr unit, fptr type)
{
    int32_t n = pb(unit, (pw(type, 0x0E) & 4) ? 3 : 2);

    return n == 0 ? 1 : n > 6 ? 6 : n;
}

static int32_t hit_of(fptr type, unsigned other_flags)
{
    return pb(type, (other_flags & 0x10) ? 9 : (other_flags & 8) ? 0x0B : 0x0A);
}

/* v and pct hundredths of it, in 16.16, the whole part kept */
static int32_t boosted(int32_t v, unsigned pct)
{
    int32_t x = (int32_t)((uint32_t)v << 16);

    return (int16_t)((uint32_t)(x + lmul32(ldiv32(x, 100), (int32_t)pct)) >> 16);
}

static int32_t strength(int32_t n, int32_t v, int32_t e, int32_t g, int32_t d)
{
    return lmul32(n, v) + ldiv32(lmul32(n, lmul32(v, ldiv32((int32_t)((uint32_t)e << 8), 6) + 1)), d)
           + lmul32(g, ldiv32(v, 2));
}

static void fight_reckon(void)
{
    fptr rec = REC, ua = pfp(rec, 0), ub = pfp(rec, 4), ta = pfp(rec, 0x0A), tb = pfp(rec, 0x0E);
    int silent = 0;
    int32_t na, nb, ea, eb, arma, armb, hita, hitb, P, Q, R, S, ca, cb, lossa, lossb, r1, r2;
    unsigned sa = pb(pfp(rec, 0x12), 5), sb = pb(pfp(rec, 0x16), 5);

    if (pw(tb, 5) & 0x80)
        silent = 1;
    if (!pw(rec, 0x22))
        silent = 1;
    if (!(pw(ta, 0x0C) & pw(tb, 5) & 0x3C))
        silent = 1;
    if (pw(rec, 0x22) && (pw(tb, 5) & 0x40))
        silent = 1;
    na = count_of(ua, ta);
    nb = count_of(ub, tb);
    eb = pb(ub, 1) + 1;
    armb = pb(tb, 1);
    hita = hit_of(ta, pw(ub, 4));
    ea = pb(ua, 1) + 1;
    arma = pb(ta, 1);
    hitb = hit_of(tb, pw(ua, 4));
    hita = boosted(hita, pb(rec, 0x24));
    armb = boosted(armb, pb(rec, 0x25));
    bi_srand((uint16_t)((pd(rec, 0) + pd(rec, 4)) ^ pd(rec, 0x2E)));
    P = strength(na, hita, ea, (int8_t)GBO(scene_ground_bonus, 2 * sa), 0x100);
    Q = strength(na, arma, ea, (int8_t)GBO(scene_ground_bonus, 2 * sa + 1), 0x100);
    R = strength(nb, hitb, eb, (int8_t)GBO(scene_ground_bonus, 2 * sb), 0x100);
    S = strength(nb, armb, eb, (int8_t)GBO(scene_ground_bonus, 2 * sb + 1), 0x200);
    ca = ea < 1 ? 2 : ea > 7 ? 6 : ea;
    cb = eb <= 1 ? 2 : eb >= 7 ? 6 : eb;
    lossb = ldiv32(P - S, ldiv32(lmul32(armb, 8 - ca), 2));
    lossa = ldiv32(R - Q, ldiv32(lmul32(arma, 8 - cb), 2));
    if (lossb <= 0 && ldiv32(S, nb) + ldiv32(armb, 2) < P)
        lossb = 1;
    if (lossa <= 0 && ldiv32(Q, na) + ldiv32(arma, 2) < R)
        lossa = 1;
    if (lossb > 6 && lossb < 0x0C)
        lossb = 6;
    if (lossa > 6 && lossa < 0x0C)
        lossa = 6;
    if (2 * na < lossb)
        lossb = 2 * na;
    if (2 * nb < lossa)
        lossa = 2 * nb;
    r1 = (int16_t)bi_random(0, 0x200);
    r2 = (int16_t)bi_random(0, 0x200);
    if (0x34 - lmul32(6, ea) > r1)
        lossb--;
    if (0x1DB - lmul32(0x14, ea) < r1)
        lossb++;
    if (0x34 - lmul32(6, eb) > r2)
        lossa--;
    if (0x1DB - lmul32(0x14, eb) < r2)
        lossa++;
    lossb = lossb < 0 ? 0 : lossb > nb ? nb : lossb;
    lossa = lossa < 0 ? 0 : lossa > na ? na : lossa;
    /* the target cannot answer: the attacker's count stays */
    if (pw(ta, 0x0E) & 4) {
        if (!silent) {
            spb(ua, 3, (unsigned)(na - lossa));
            if (!pb(ua, 3))
                spb(ua, 2, 0);
        }
    } else if (!silent)
        spb(ua, 2, (unsigned)(na - lossa));
    if (pw(tb, 0x0E) & 4) {
        spb(ub, 3, (unsigned)(nb - lossb));
        if (!pb(ub, 3))
            spb(ub, 2, 0);
    } else
        spb(ub, 2, (unsigned)(nb - lossb));
}

/* ---- T223C, T1F5A: the units and the shots ---- */

/* a unit's script: for one with 4 or 20h in its +6, in the air, on the
 * water, else by its ground's +5 */
static fptr unit_script(fptr unit, fptr ground)
{
    static const uint16_t by_ground[7] = {0x72, 0xA6, 0xA6, 0xC8, 0x0E, 0xFE, 0xFE};
    unsigned off, k = pb(ground, 5);

    /* MOON.EXE: its table of scripts begins 10 bytes earlier and a type
     * with 8 in its +0Eh has the longest script too */
    if (pw(unit, 6) & 0x24 || (bi_prog == BI_MOON && (pw(TYPE(pb(unit, 8)), 0x0E) & 8)))
        off = 0x128;
    else if (pw(unit, 4) & 0x10)
        off = 0x0E;
    else if (pw(unit, 4) & 8)
        off = 0x40;
    else
        off = k >= 1 && k <= 7 ? by_ground[k - 1] : 0x0E;
    return MKFP(S_scene_scripts, A_scene_scripts + off - (bi_prog == BI_MOON ? 10 : 0));
}

static unsigned clamp_count(unsigned n) { return n == 0 ? 1 : n > 6 ? 6 : n; }

/* the script's pointer a byte on: MOON.EXE keeps it normalized (a huge
 * pointer's add, L1E36) where BATTLE.EXE only counts the offset up */
static fptr next_byte(fptr s)
{
    return bi_prog == BI_MOON ? hadd(s, 1) : MKFP(FSEG(s), FOFF(s) + 1);
}

/* T1F5A:0001: both sides' counts within 1..6, each unit at its FEh of
 * the script */
static void place_units(void)
{
    fptr rec = REC, s;
    unsigned i;

    SB(scene_count_a, clamp_count(GB(scene_count_a)));
    SB(scene_count_b, clamp_count(GB(scene_count_b)));
    s = unit_script(pfp(rec, 0), pfp(rec, 0x12));
    for (i = 0; i < GB(scene_count_a); i++) {
        while (pb(s, 0) != 0xFE)
            s = next_byte(s);
        spfp(SCENE_A(i), 2, s);
        s = next_byte(s);
        spw(SCENE_A(i), 6, 0);
        spb(SCENE_A(i), 1, 0);
    }
    s = unit_script(pfp(rec, 4), pfp(rec, 0x16));
    for (i = 0; i < GB(scene_count_b); i++) {
        while (pb(s, 0) != 0xFE)
            s = next_byte(s);
        spfp(SCENE_B(i), 2, s);
        s = next_byte(s);
        spw(SCENE_B(i), 6, 0);
        spb(SCENE_B(i), 1, 0);
    }
}

/* the frame's pieces a sprite at x, y reaches into */
static unsigned frame_touched(int x, int y)
{
    int left = half() ? 0xA8 : 8, right = half() ? 0x118 : 0x78;
    unsigned bits = 0;

    x = (int16_t)x, y = (int16_t)y;
    if (x < left)
        bits |= 4;
    if (x > right)
        bits |= 8;
    if (y < 0x0C)
        bits |= 1;
    if (y > 0x90)
        bits |= 2;
    return bits;
}

static void touch(int x, int y) { SB(scene_frame_bits, GB(scene_frame_bits) | frame_touched(x, y)); }

/* the way of a shot, one of eight: 0 up, 1 right up, 2 right .. 7 left
 * up; upright or level when the other width is less than half */
static int shot_way(int x1, int y1, int x2, int y2)
{
    int dx = (int16_t)(x2 - x1), dy = (int16_t)(y1 - y2);
    int ax = (int16_t)(dx < 0 ? -dx : dx), ay = (int16_t)(dy < 0 ? -dy : dy);

    if (dx == 0)
        return dy < 0 ? 4 : 0;
    if (dy == 0)
        return dx < 0 ? 6 : 2;
    if ((int16_t)(ax << 1) < ay)
        return dy > 0 ? 0 : 4;
    if ((int16_t)(ay << 1) < ax)
        return dx > 0 ? 2 : 6;
    if (dx > 0)
        return dy > 0 ? 1 : 3;
    return dy > 0 ? 7 : 5;
}

/* a unit of the scene drawn (not one that was hit): the attacker's at
 * x, 156 - y, the target's at 128 - x, y and turned by three directions;
 * the second half of a unit of two squares beside it, a unit in the air
 * with its shadow */
static void scene_draw_unit(fptr r, int side)
{
    fptr rec = REC, unit = side ? UNIT_B : UNIT_A, lib = pfp(rec, 0x32), e1, e2 = 0, t;
    unsigned two = pb(side ? TYPE_B : TYPE_A, 0x0C) & 0x40, d;
    int ybase = side ? 0 : 0x9C, xbase, x, y, base;

    if (pb(r, 1))
        return;
    if (pw(UNIT_A, 4) & 1) {
        xbase = side ? 0x120 : 0xA0;
        SW(scene_base_a, 0x30);
        SW(scene_base_b, 0x20);
    } else {
        xbase = side ? 0x80 : 0;
        SW(scene_base_a, 0x20);
        SW(scene_base_b, 0x30);
    }
    d = pb(r, 0);
    if (side) {
        d += 3;
        if (d > 5)
            d -= 6;
    }
    e1 = pfp(lib, pb(unit, 8) * 0x18 + d * 4);
    if (two)
        e2 = pfp(lib, (pb(unit, 8) + 1) * 0x18 + d * 4);
    x = (int16_t)(side ? xbase - pw(r, 8) : xbase + pw(r, 8));
    y = (int16_t)(side ? ybase + pw(r, 0x0A) : ybase - pw(r, 0x0A));
    base = side ? GW(scene_base_b) : GW(scene_base_a);
    if (two) {
        int dx = 0, dy = 0;

        switch (pb(r, 0)) {
        case 0:
        case 3:
            dy = 0x18;
            break;
        case 1:
        case 4:
            dy = 0x0C, dx = -0x10;
            break;
        case 2:
        case 5:
            dy = 0x0C, dx = 0x10;
            break;
        }
        if (side)
            dx = -dx, dy = -dy;
        if (pb(r, 0) == 3 || pb(r, 0) == 2 || pb(r, 0) == 4) {
            t = e1;
            e1 = e2;
            e2 = t;
        }
        touch(x + dx, y + dy);
        draw_entry(x + dx, y + dy, e2, 1, 0, base);
    }
    if (pw(unit, 4) & 0x10) {
        /* the shadow: looked for 2 above, drawn 2 below */
        touch(x - 3, y - 2);
        draw_entry(x - 3, y + 2, e1, 1, 0, 0x50);
    }
    touch(x, y);
    draw_entry(x, y, e1, 1, 0, base);
    spw(r, 0x0C, x);
    spw(r, 0x0E, y);
}

/* a unit a step on along its script and drawn; 1 when it has arrived */
static int scene_step_unit(fptr r, int side)
{
    if (!pw(r, 6)) {
        fptr s = pfp(r, 2);
        unsigned c = pb(s, 0);

        if (c == 0xFF) {
            spw(r, 6, 0);
            scene_draw_unit(r, side);
            return 1;
        }
        if (c == 0xFE) {
            spw(r, 8, pb(s, 1));
            spw(r, 0x0A, pb(s, 2));
            spb(r, 0, pb(s, 3));
            spw(r, 2, pw(r, 2) + 4);
            spw(r, 6, 0);
        } else {
            spb(r, 0, c);
            spw(r, 6, pb(s, 1));
            spw(r, 2, pw(r, 2) + 2);
        }
    } else {
        unsigned step = GBO(scene_pace, pb(side ? UNIT_B : UNIT_A, 8));
        int dx = 0, dy = 0;

        if ((int16_t)pw(r, 6) < (int)step)
            step = pb(r, 6);
        switch (pb(r, 0)) {
        case 0:
            dy = 1;
            break;
        case 1:
            dx = 1, dy = 1;
            break;
        case 2:
            dx = 1, dy = -1;
            break;
        case 3:
            dy = -1;
            break;
        case 4:
            dx = -1, dy = -1;
            break;
        case 5:
            dx = -1, dy = 1;
            break;
        }
        spw(r, 8, pw(r, 8) + dx * (int)step);
        spw(r, 0x0A, pw(r, 0x0A) + dy * (int)step);
        spw(r, 6, pw(r, 6) - step);
    }
    scene_draw_unit(r, side);
    return 0;
}

/* T1F5A:093C: the units come in; 1 when all have arrived.  Each side's
 * sound (its type's +41h) begins with the first call and ends when its
 * units have arrived, unless the type's +43h has bit 1. */
static int units_come(void)
{
    fptr rec = REC;
    unsigned arrived_a = 0, arrived_b = 0, snd_a = pb(TYPE_A, 0x41), snd_b = pb(TYPE_B, 0x41), i;
    unsigned ch_a = (pw(UNIT_A, 4) & 1) ? 2 : 0, ch_b = (pw(UNIT_A, 4) & 1) ? 0 : 2;

    SD(scene_calls_units, GD(scene_calls_units) + 1);
    SB(scene_frame_bits, 0);
    if (GD(scene_calls_units) == 1) {
        if (pw(rec, 0x2C) & 1) {
            fptr e = pfp(rec, 0x4A);

            if (snd_a != 0xFF) {
                spw(e, 6 * ch_a, snd_a);
                spw(e, 6 * ch_a + 2, GB(scene_count_a) * 10 + 0x40);
                spw(e, 6 * ch_a + 4, 0xC8);
            }
            if (snd_b != 0xFF) {
                spw(e, 6 * ch_b, snd_b);
                spw(e, 6 * ch_b + 2, GB(scene_count_b) * 10 + 0x40);
                spw(e, 6 * ch_b + 4, 0xC8);
            }
            effects_start();
        }
        place_units();
    }
    for (i = 0; i < GB(scene_count_a); i++)
        if (scene_step_unit(SCENE_A(i), 0))
            arrived_a++;
    for (i = 0; i < GB(scene_count_b); i++)
        if (scene_step_unit(SCENE_B(i), 1))
            arrived_b++;
    if (pw(rec, 0x2C) & 1) {
        if (snd_a != 0xFF && arrived_a == GB(scene_count_a) && !(pb(TYPE_A, 0x43) & 1))
            sound(ch_a, snd_a, 0, 0xFFFF);
        if (snd_b != 0xFF && arrived_b == GB(scene_count_b) && !(pb(TYPE_B, 0x43) & 1))
            sound(ch_b, snd_b, 0, 0xFFFF);
    }
    frame_again();
    if (arrived_a != GB(scene_count_a) || arrived_b != GB(scene_count_b))
        return 0;
    SD(scene_calls_units, 0);
    return 1;
}

static int sign(int v) { return v > 0 ? 1 : v < 0 ? 0xFFFF : 0; }

/* what the shots' other side loses: its count before less after */
static unsigned loss_of(int answer)
{
    if (answer)
        return (GB(scene_count_a) - GB(scene_a_after)) & 0xFF;
    return (GB(scene_count_b) - GB(scene_b_after)) & 0xFF;
}

/* a side's shots made: shot j of n from its unit j to the other side's
 * unit n - 1 - j; those below the other's loss hit, the others aim up to
 * 32 off; more losses than shots make shots that are not drawn */
static void make_side(int answer)
{
    unsigned n, count_from, count_to, loss, j, k, mask = answer ? 1 : 5;
    fptr rec = REC;

    if (answer) {
        SB(scene_shots_b_count, GB(scene_count_b));
        n = GB(scene_shots_b_count);
        count_from = GB(scene_count_b), count_to = GB(scene_count_a);
    } else {
        SB(scene_shots_a_count, GB(scene_count_a));
        n = GB(scene_shots_a_count);
        count_from = GB(scene_count_a), count_to = GB(scene_count_b);
    }
#define FROM(i) (answer ? SCENE_B(i) : SCENE_A(i))
#define TO(i) (answer ? SCENE_A(i) : SCENE_B(i))
#define SHOT(i) (answer ? SHOT_B(i) : SHOT_A(i))
    for (j = 0, k = (n - 1) & 0xFF; j < n; j++, k = (k - 1) & 0xFF) {
        fptr s = SHOT(j);
        int sx, sy, tx, ty, dx, dy, ax, ay;
        unsigned t = n > count_to ? k % count_to : k, f, kind;

        tx = (int16_t)(pw(TO(t), 0x0C) + 0x0C);
        ty = (int16_t)(pw(TO(t), 0x0E) + 0x0C);
        spb(s, 0x1A, t);
        /* the attacker's by the shot's number, the answer's by j */
        f = k < count_from ? j : (answer ? j : k) % count_from;
        sx = (int16_t)(pw(FROM(f), 0x0C) + 6);
        sy = (int16_t)pw(FROM(f), 0x0E);
        if (k >= loss_of(answer))
            ty = (int16_t)(ty + bi_random(-0x20, 0x20));
        dx = (int16_t)(tx - sx);
        dy = (int16_t)(ty - sy);
        spw(s, 0x0A, sign(dx));
        spw(s, 0x0C, sign(dy));
        kind = GBO(scene_shot_types, 2 * pb(answer ? UNIT_B : UNIT_A, 8));
        spfp(s, 5, pfp(pfp(rec, 0x3A), kind * 32 + 4 * (unsigned)shot_way(sx, sy, tx, ty) + 0x18));
        ax = (int16_t)(dx < 0 ? -dx : dx);
        ay = (int16_t)(dy < 0 ? -dy : dy);
        spw(s, 0x16, ax > ay ? ax : ay);
        spw(s, 0x18, 0);
        spw(s, 0, sx);
        spw(s, 2, sy);
        spw(s, 0x14, 0);
        spw(s, 0x12, 0);
        spb(s, 4, mask | (k < loss_of(answer) ? 0x10 : 0));
        spb(s, 9, 5);
        spw(s, 0x0E, ax);
        spw(s, 0x10, ay);
        spb(s, 0x1B, bi_random(0, 0x2710) & 0x0C);
    }
    loss = loss_of(answer);
    if (loss <= n)
        return;
    for (j = n; j < loss; j++) {
        fptr s = SHOT(j);

        spb(s, 0x1A, j);
        spb(s, 4, mask | 0x30);
        spw(s, 0x18, 0);
        spb(s, 9, 5);
        spw(s, 0x16, pw(SHOT(j % n), 0x16));
        spw(s, 0, pw(TO(j), 0x0C));
        spw(s, 2, pw(TO(j), 0x0E));
        spb(s, 0x1B, 0);
    }
    if (answer)
        SB(scene_shots_b_count, loss);
    else
        SB(scene_shots_a_count, loss);
#undef FROM
#undef TO
#undef SHOT
}

/* the target's shots, unless it cannot answer: fight_reckon's rules, and
 * here also squares further apart than its range (its type's +7 against
 * a unit in the air, else +8, less 1) */
static void make_answer(void)
{
    fptr rec = REC;
    int dx = (int16_t)(pw(rec, 0x1E) - pw(rec, 0x1A)), dy = (int16_t)(pw(rec, 0x20) - pw(rec, 0x1C)), range;

    SB(scene_shots_b_count, GB(scene_count_b));
    SB(scene_b_silent, 0);
    if (pw(TYPE_B, 5) & 0x80)
        SB(scene_b_silent, 1);
    if (!pw(rec, 0x22))
        SB(scene_b_silent, 1);
    if (!(pw(TYPE_A, 0x0C) & pw(TYPE_B, 5) & 0x3C))
        SB(scene_b_silent, 1);
    if (pw(rec, 0x22) && (pw(TYPE_B, 5) & 0x40))
        SB(scene_b_silent, 1);
    dx = (int16_t)(dx < 0 ? -dx : dx);
    dy = (int16_t)(dy < 0 ? -dy : dy);
    range = pb(TYPE_B, (pw(UNIT_A, 4) & 0x10) ? 7 : 8) - 1;
    if ((dx > dy ? dx : dy) > range)
        SB(scene_b_silent, 1);
    if (!GB(scene_b_silent))
        make_side(1);
}

/* a shot a call on; 1 when it is over */
static int step_shot(fptr s)
{
    fptr rec = REC;
    unsigned speed, i;

    if (pb(s, 4) & 8)
        return 1;
    if (pb(s, 0x1B)) {
        spb(s, 0x1B, pb(s, 0x1B) - 1);
        return 0;
    }
    if (!(pb(s, 4) & 0x60)) {
        /* its sound: the firing type's +42h, quieter with 2 in its +43h */
        if (pw(rec, 0x2C) & 1) {
            fptr t = (pb(s, 4) & 4) ? TYPE_A : TYPE_B;

            sound((pw(UNIT_A, 4) & 1) ? 1 : 3, pb(t, 0x42), (pb(t, 0x43) & 2) ? 0x5F : 0x7F, 1);
        }
        spb(s, 4, pb(s, 4) | 0x40);
    }
    if (pb(s, 4) & 1) {
        if (!(pb(s, 4) & 0x20)) {
            draw_entry((int16_t)pw(s, 0), (int16_t)pw(s, 2), pfp(s, 5), 1, 0, 0);
            touch(pw(s, 0), pw(s, 2));
        }
        speed = GBO(scene_shot_types, 2 * pb((pb(s, 4) & 4) ? UNIT_A : UNIT_B, 8) + 1);
        for (i = 0; i < speed && (int16_t)pw(s, 0x18) < (int16_t)pw(s, 0x16); i++) {
            if (!(pb(s, 4) & 0x20)) {
                spw(s, 0x12, pw(s, 0x12) + pw(s, 0x0E));
                spw(s, 0x14, pw(s, 0x14) + pw(s, 0x10));
                if ((int16_t)pw(s, 0x12) > (int16_t)pw(s, 0x16)) {
                    spw(s, 0x12, pw(s, 0x12) - pw(s, 0x16));
                    spw(s, 0, pw(s, 0) + pw(s, 0x0A));
                }
                if ((int16_t)pw(s, 0x14) > (int16_t)pw(s, 0x16)) {
                    spw(s, 0x14, pw(s, 0x14) - pw(s, 0x16));
                    spw(s, 2, pw(s, 2) + pw(s, 0x0C));
                }
            }
            spw(s, 0x18, pw(s, 0x18) + 1);
        }
        if (pw(s, 0x18) == pw(s, 0x16)) {
            unsigned volume = 0x5A;

            /* there: it explodes, and the unit it hits is gone */
            spb(s, 4, (pb(s, 4) & 0xFE) | 2);
            if (pb(s, 4) & 0x10) {
                volume = 0x7F;
                spb((pb(s, 4) & 4) ? SCENE_B(pb(s, 0x1A)) : SCENE_A(pb(s, 0x1A)), 1, 1);
            }
            if (pw(rec, 0x2C) & 1)
                sound((pw((pb(s, 4) & 4) ? UNIT_A : UNIT_B, 4) & 1) ? 1 : 3, 3, volume, 1);
        }
        return 0;
    }
    if (pb(s, 4) & 2) {
        if ((int8_t)pb(s, 9) < 0) {
            spb(s, 4, pb(s, 4) | 8);
            return 0;
        }
        draw_entry((int16_t)(pw(s, 0) - 0x0C), (int16_t)(pw(s, 2) - 0x0C),
                   pfp(pfp(rec, 0x3A), (unsigned)(4 * (int8_t)pb(s, 9))), 1, 0, (pb(s, 4) & 0x10) ? 0x30 : 0x20);
        touch(pw(s, 0) - 0x0C, pw(s, 2) - 0x0C);
        spb(s, 9, pb(s, 9) - 1);
    }
    return 0;
}

/* T1F5A:1A21: the shots; 1 when all are over */
static int scene_shots(void)
{
    unsigned i, done_a = 0, done_b = 0;

    SD(scene_calls_shots, GD(scene_calls_shots) + 1);
    SB(scene_frame_bits, 0);
    if (GD(scene_calls_shots) == 1) {
        make_side(0);
        make_answer();
    }
    for (i = 0; i < GB(scene_count_b); i++)
        scene_draw_unit(SCENE_B(i), 1);
    for (i = 0; i < GB(scene_count_a); i++)
        scene_draw_unit(SCENE_A(i), 0);
    for (i = 0; i < GB(scene_shots_a_count); i++)
        if (step_shot(SHOT_A(i)))
            done_a++;
    if (GB(scene_b_silent))
        done_b = GB(scene_shots_b_count);
    else
        for (i = 0; i < GB(scene_shots_b_count); i++)
            if (step_shot(SHOT_B(i)))
                done_b++;
    frame_again();
    if (done_a != GB(scene_shots_a_count) || done_b != GB(scene_shots_b_count))
        return 0;
    SD(scene_calls_shots, 0);
    return 1;
}

/* T1F3C:000A: a call of the scene; 1 when the fight is over */
int fight_step(fptr rec)
{
    fptr e;
    unsigned i;

    bi_at("fight_step");
    SFP(scene_record, rec);
    spd(rec, 0x26, pd(rec, 0x26) + 1);
    if (pd(rec, 0x26) <= 2) {
        if (pd(rec, 0x26) == 1) {
            SB(scene_shot, 0);
            SB(scene_arrived, 0);
            SB(scene_count_a, pb(UNIT_A, 2));
            SB(scene_count_b, pb(UNIT_B, 2));
            fight_reckon();
            SB(scene_a_after, pb(UNIT_A, 2));
            SB(scene_b_after, pb(UNIT_B, 2));
        }
        scene_ground(pw(UNIT_A, 4) & 1, pw(rec, 0x22) == 0);
        frame_below();
        frame_above();
        frame_left();
        frame_right();
    }
    if (!GB(scene_arrived)) {
        SB(scene_arrived, units_come());
        return 0;
    }
    if (!GB(scene_shot)) {
        SB(scene_shot, scene_shots());
        return 0;
    }
    e = GFP(effects_ptr);
    for (i = 0; i < 4; i++)
        spw(e, 6 * i + 4, 0xFFFF);
    effects_start();
    for (i = 0; i < 4; i++)
        effects_volume((int)i, 0x7F);
    spd(rec, 0x26, 0);
    return 1;
}
