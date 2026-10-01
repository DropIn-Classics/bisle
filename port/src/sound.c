/* sound.c - BATTLE.EXE's sound, CODE:0215..1B79: a driver for the
 * AdLib's registers (18 operators of 14 values, nine voices or six and
 * five drums), a player of MIDI files with the AdLib's own events in
 * them, and the effects (tools/sndfiles.py has the files' formats and
 * the rules; BATTLE.hints at sound_init).
 *
 * The values go to doskit's OPL (audio.c).  The PC speaker's sound (the
 * original's /s, and what it does when it finds no AdLib) is not in the
 * port: the port has an AdLib always.
 */
#include <string.h>
#include "bi.h"

/* an operator's 14 values in op_params: +0 the level's scaling, +1 the
 * frequency's multiple, +2 the feedback, +3 attack, +4 sustain, +5 the
 * sustained sound, +6 decay, +7 release, +8 the level, +9 tremolo, +10
 * vibrato, +11 the envelope's scaling, +12 not the frequency modulation,
 * +13 the wave form */
#define OP(op, i) GBO(op_params, 14 * (op) + (i))
/* a far pointer to a variable behind DATA as the original makes it: with
 * DS, the segment of DATA */
#define DP(n, i) MKFP(SEG(DATA), A_##n + ((S_##n - SEG(DATA)) << 4) + (i))
/* the port's own memory for the 28 words of a song's timbre (on the
 * stack in the original) */
#define TIMBRE_SCRATCH MKFP(0x0050, 0x0040)

static void opl_write(unsigned reg, unsigned value)
{
    adlib_out(reg & 0xFF, value & 0xFF);
}

/* the frequency of a voice: the note (a semitone), the bend (2000h none)
 * times the range, in sixteenths of a semitone; the key's bit with it.
 * Returns what register B0 got. */
static unsigned opl_frequency(unsigned voice, unsigned note, unsigned bend, unsigned key)
{
    int ax = (int16_t)(bend - 0x2000), octave;
    unsigned n, fnum, b0;

    if (ax)
        ax = (int16_t)((ax >> 5) * (int)GW(bend_range));
    ax = (int16_t)(ax + (int)(note << 8));
    ax = (int16_t)(ax + 8) >> 4;
    if (ax < 0)
        ax = 0;
    else if (ax >= 0x5FF)
        ax = 0x5FF;
    n = (unsigned)ax >> 4;
    fnum = GWO(fnums, 32 * GBO(notes, n) + (((unsigned)ax << 1) & 0x1F));
    octave = (int8_t)(GBO(octaves, n) - 1);
    if (fnum & 0x8000)
        octave = (int8_t)(octave + 1);
    if (octave < 0) {
        octave = (int8_t)(octave + 1);
        fnum = (uint16_t)((int16_t)fnum >> 1);
    }
    opl_write(0xA0 + voice, fnum);
    b0 = ((fnum >> 8 & 3) + ((unsigned)octave << 2) + key) & 0xFF;
    opl_write(0xB0 + voice, b0);
    return b0;
}

static void opl_write_frequency(unsigned voice)
{
    SBO(voice_b0, voice, opl_frequency(voice, (unsigned)(int8_t)GBO(voice_note, voice),
                                       GWO(voice_bend, 2 * voice), (unsigned)(int8_t)GBO(voice_key, voice)));
}

/* register BDh: the depths, the percussion mode, the drums' keys */
static void opl_write_bd(void)
{
    opl_write(0xBD, (GB(opl_am_deep) ? 0x80u : 0) | (GB(opl_vib_deep) ? 0x40u : 0)
                        | (GB(perc_mode) ? 0x20u : 0) | (unsigned)(int8_t)GB(perc_keys));
}

/* CODE:0897: register 8 */
static void opl_write_08(void)
{
    opl_write(8, GB(opl_note_sel) ? 0x40 : 0);
}

/* an operator's level, a carrier's (and a drum's) by its voice's volume */
static void opl_write_level(unsigned op)
{
    unsigned voice = (unsigned)(int8_t)(GB(perc_mode) ? GBO(op_voice_perc, op) : GBO(op_voice, op));
    unsigned level = 0x3F - (OP(op, 8) & 0x3F);

    if (GBO(op_is_carrier, op) || (GB(perc_mode) && voice > 6))
        level = (uint16_t)((int8_t)GBO(voice_volume, voice) * (int)level + 0x40) >> 7;
    level = (uint16_t)(0x3F - level) | (uint16_t)((int8_t)OP(op, 0) << 6);
    opl_write(0x40 + (unsigned)(int8_t)GBO(op_offsets, op), level);
}

/* CODE:07C6: all of an operator's registers */
static void opl_write_operator(unsigned op)
{
    unsigned at = (unsigned)(int8_t)GBO(op_offsets, op);

    opl_write_bd();
    opl_write_08();
    opl_write_level(op);
    if (!GBO(op_is_carrier, op))
        opl_write(0xC0 + (unsigned)(int8_t)GBO(op_voice, op),
                  (uint16_t)((int8_t)OP(op, 2) << 1) | (OP(op, 12) ? 0 : 1u));
    opl_write(0x60 + at, (uint16_t)((int8_t)OP(op, 3) << 4) | (OP(op, 6) & 0x0F));
    opl_write(0x80 + at, (uint16_t)((int8_t)OP(op, 4) << 4) | (OP(op, 7) & 0x0F));
    opl_write(0x20 + at, (OP(op, 9) ? 0x80u : 0) + (OP(op, 10) ? 0x40u : 0) + (OP(op, 5) ? 0x20u : 0)
                             + (OP(op, 11) ? 0x10u : 0) + (OP(op, 1) & 0x0F));
    opl_write(0xE0 + at, GW(opl_wave_on) ? OP(op, 13) & 3u : 0);
}

/* an operator's values from 13 words at `from` and its wave form */
static void opl_set_operator(unsigned op, fptr from, unsigned wave)
{
    unsigned i;

    for (i = 0; i < 13; i++)
        SBO(op_params, 14 * op + i, pb(from, 2 * i));
    SBO(op_params, 14 * op + 13, wave & 3);
    opl_write_operator(op);
}

/* CODE:071E: the same from 13 bytes */
static void opl_set_operator_bytes(unsigned op, fptr from, unsigned wave)
{
    unsigned i;

    for (i = 0; i < 13; i++)
        SWO(op_scratch, 2 * i, pb(from, i));
    opl_set_operator(op, DP(op_scratch, 0), wave);
}

/* CODE:05FC: every operator the driver's own modulator or carrier, and in
 * the percussion mode the drums' */
static void opl_default_operators(void)
{
    static const uint8_t drum_ops[6] = { 12, 15, 16, 14, 17, 13 };
    unsigned op;

    for (op = 0; op < 0x12; op++)
        opl_set_operator_bytes(op, GBO(op_is_carrier, op) ? FP(op_carrier) : FP(op_modulator), 0);
    if (GB(perc_mode))
        for (op = 0; op < 6; op++)
            opl_set_operator_bytes(drum_ops[op], MKFP(S_op_drums, A_op_drums + 14 * op), 0);
}

static void opl_set_mode(unsigned percussion)
{
    if (percussion) {
        SBO(voice_note, 8, 0x18);
        SWO(voice_bend, 16, 0x2000);
        opl_write_frequency(8);
        SBO(voice_note, 7, 0x1F);
        SWO(voice_bend, 14, 0x2000);
        opl_write_frequency(7);
    }
    SB(perc_mode, percussion);
    SW(voice_count, percussion ? 0x0B : 9);
    SB(perc_keys, 0);
    opl_default_operators();
    opl_write_bd();
}

static void opl_set_wave(unsigned on)
{
    unsigned i;

    SW(opl_wave_on, on ? 0x20 : 0);
    for (i = 0; i < 0x12; i++)
        opl_write(0xE0 + (unsigned)(int8_t)GBO(op_offsets, i), 0);
    opl_write(1, GW(opl_wave_on));
}

static void opl_set_range(unsigned semitones)
{
    if (semitones > 0x0C)
        semitones = 0x0C;
    if (semitones < 1)
        semitones = 1;
    SW(bend_range, semitones);
}

static void opl_set_depths(unsigned am, unsigned vib, unsigned note_sel)
{
    SB(opl_am_deep, am);
    SB(opl_vib_deep, vib);
    SB(opl_note_sel, note_sel);
    opl_write_bd();
    opl_write_08();
}

/* a voice's two operators (one for a drum) */
static fptr voice_slots(unsigned voice)
{
    return GB(perc_mode) ? MKFP(S_slots_perc, A_slots_perc + 2 * voice) : MKFP(S_slots, A_slots + 2 * voice);
}

/* a voice's timbre: 13 words for each of its two operators, then their
 * two wave forms */
static void opl_set_timbre(unsigned voice, fptr from)
{
    fptr slot;

    if (!GW(adlib_found) || voice >= GW(voice_count))
        return;
    slot = voice_slots(voice);
    opl_set_operator(pb(slot, 0), from, pw(from, 0x34));
    if (pb(slot, 1) != 0xFF)
        opl_set_operator(pb(slot, 1), MKFP(FSEG(from), FOFF(from) + 0x1A), pw(from, 0x36));
}

static void opl_set_volume(unsigned voice, unsigned volume)
{
    fptr slot;

    if (voice >= GW(voice_count))
        return;
    if (volume > 0x7F)
        volume = 0x7F;
    volume = (uint16_t)(GWO(voice_scale, 2 * voice) * volume) / 0x7F;
    SBO(voice_volume, voice, volume);
    slot = voice_slots(voice);
    opl_write_level(pb(slot, 0));
    if (pb(slot, 1) != 0xFF)
        opl_write_level(pb(slot, 1));
}

/* 1 for a voice with a pitch of its own: all nine, or in the percussion
 * mode those below `drums` */
static int melodic(unsigned voice, unsigned drums)
{
    return GB(perc_mode) ? voice < drums : voice < 9;
}

static void opl_set_bend(unsigned voice, unsigned bend)
{
    if (GB(perc_mode) ? voice > 6 : voice >= 9)
        return;
    if (bend > 0x3FFF)
        bend = 0x3FFF;
    SWO(voice_bend, 2 * voice, bend);
    opl_write_frequency(voice);
}

static void note_on(unsigned voice, int note)
{
    note -= 0x0C;
    if (note < 0)
        note = 0;
    if (melodic(voice, 6)) {
        SBO(voice_note, voice, note);
        SBO(voice_key, voice, 0x20);
        opl_write_frequency(voice);
        return;
    }
    if (!GB(perc_mode) || voice > 0x0A)
        return;
    if (voice == 6) {
        SBO(voice_note, 6, note);
        opl_write_frequency(6);
    } else if (voice == 8 && (int8_t)GBO(voice_note, 8) != note) {
        SBO(voice_note, 8, note);
        SBO(voice_note, 7, note + 7);
        opl_write_frequency(8);
        opl_write_frequency(7);
    }
    SB(perc_keys, GB(perc_keys) | GBO(perc_bits, voice - 6));
    opl_write_bd();
}

static void note_off(unsigned voice)
{
    if (melodic(voice, 6)) {
        SBO(voice_key, voice, 0);
        SBO(voice_b0, voice, GBO(voice_b0, voice) & 0xDF);
        opl_write(0xB0 + voice, GBO(voice_b0, voice));
        return;
    }
    if (!GB(perc_mode) || voice > 0x0A)
        return;
    SB(perc_keys, GB(perc_keys) & ~GBO(perc_bits, voice - 6));
    opl_write_bd();
}

static void opl_reset(void)
{
    unsigned i;

    for (i = 1; i <= 0xF5; i++)
        opl_write(i, 0);
    opl_write(4, 6);
    for (i = 0; i < 9; i++) {
        SWO(voice_bend, 2 * i, 0x2000);
        SBO(voice_key, i, 0);
        SBO(voice_note, i, 0);
    }
    for (i = 0; i < 0x0B; i++)
        SBO(voice_volume, i, 0x7F);
    opl_set_mode(0);
    opl_set_depths(0, 0, 0);
    opl_set_range(1);
    opl_set_wave(1);
}

/* the AdLib looked for by its timers (the status before and after timer
 * 1 has run); the port's card is there */
static int adlib_probe(void)
{
    opl_write(4, 0x60);
    opl_write(4, 0x80);
    opl_write(2, 0xFF);
    opl_write(4, 0x21);
    opl_write(4, 0x60);
    opl_write(4, 0x80);
    return 1;
}

static int opl_init(unsigned port)
{
    int found;

    SW(opl_port, port);
    found = adlib_probe();
    opl_reset();
    return found;
}

/* ---- the songs ---- */

static void song_timer(void);

static unsigned be16(fptr p) { return pb(p, 0) << 8 | pb(p, 1); }
static uint32_t be32(fptr p)
{
    return (uint32_t)pb(p, 0) << 24 | (uint32_t)pb(p, 1) << 16 | (uint32_t)pb(p, 2) << 8 | pb(p, 3);
}
static fptr fadd(fptr p, unsigned n) { return MKFP(FSEG(p), FOFF(p) + n); }

/* CODE:14F2: the song's timer gets the period (0: 10000h counts) */
static void song_period(unsigned counts)
{
    if (GW(song_timer_handle) == 0xFFFF)
        return;
    timer_period(GW(song_timer_handle), counts ? counts : 0x10000);
}

/* the period of 8 ticks of the song: (tempo / 125) * 1194 / division
 * counts of the PIT */
static void song_tempo(unsigned division, uint32_t tempo)
{
    int32_t period = 0;

    if (division)
        period = (int32_t)((uint32_t)((int32_t)tempo / 0x7D) * 0x4AAu) / (int32_t)division;
    song_period((uint16_t)period);
}

/* a number of 7 bits a byte at the current track's place */
static uint32_t song_number(void)
{
    fptr cur = GFP(track_cur), p = pfp(cur, 0);
    uint32_t v;
    unsigned b;

    b = pb(p, 0);
    p = fadd(p, 1);
    v = b;
    if (b & 0x80) {
        v &= 0x7F;
        do {
            b = pb(p, 0);
            v = (v << 7) + (b & 0x7F);
            p = fadd(p, 1);
        } while (b & 0x80);
    }
    spfp(cur, 0, p);
    return v;
}

/* the song's header read: its tracks' beginnings, each track's first
 * time and status */
static void song_rewind(fptr song)
{
    uint32_t length = be32(fadd(song, 4));
    unsigned i;
    fptr p;

    SW(song_tracks, be16(fadd(song, 0x0A)));
    SW(song_division, be16(fadd(song, 0x0C)));
    p = fadd(song, (uint16_t)length + 8);
    for (i = 0; (int)i < (int16_t)GW(song_tracks); i++) {
        length = be32(fadd(p, 4));
        spfp(DP(track_ptr, 0), 4 * i, fadd(p, 8));
        p = fadd(p, (uint16_t)length + 8);
    }
    for (i = 0; (int)i < (int16_t)GW(song_tracks); i++) {
        SFP(track_cur, DP(track_ptr, 4 * i));
        spd(FP(track_time), 4 * i, song_number());
        SBO(track_status, i, pb(pfp(GFP(track_cur), 0), 0));
    }
}

static void song_begin(void)
{
    SD(song_time, 0);
    SW(song_track, 0);
    SFP(track_cur, DP(track_ptr, 0));
    SFP(status_ptr, DP(track_status, 0));
    SB(song_ended, 0);
    SB(song_playing, 1);
    song_tempo(0x1E0, 0x7A120);
    SW(song_wait, 1);
}

/* CODE:0E86 */
static void song_halt(void)
{
    SB(song_playing, 0);
    song_period(0);
}

/* the track whose next event is the earliest becomes the current one;
 * the ticks until then, 0 when all tracks are at their end */
static unsigned song_next(void)
{
    unsigned si = 0, di, track = GW(song_track);
    uint32_t t;

    if (pb(GFP(status_ptr), 0) != 0x2F)
        spd(FP(track_time), 4 * track, pd(FP(track_time), 4 * track) + song_number());
    else
        spd(FP(track_time), 4 * track, 0x7FFFFFFF);
    for (di = 1; (int)di < (int16_t)GW(song_tracks); di++)
        if ((int32_t)pd(FP(track_time), 4 * di) < (int32_t)pd(FP(track_time), 4 * si)
            && GBO(track_status, di) != 0x2F)
            si = di;
    if (GBO(track_status, si) == 0x2F) {
        SB(song_ended, 1);
        song_halt();
        return 0;
    }
    t = pd(FP(track_time), 4 * si);
    di = (uint16_t)(t - GD(song_time));
    SD(song_time, t);
    SFP(track_cur, DP(track_ptr, 4 * si));
    SFP(status_ptr, DP(track_status, si));
    SW(song_track, si);
    return di;
}

static void song_note(unsigned voice, unsigned note, unsigned volume)
{
    if (!GW(song_sounding)) {
        SWO(song_volume, 2 * voice, volume);
        return;
    }
    if (!volume) {
        note_off(voice);
        SWO(song_volume, 2 * voice, 0);
        return;
    }
    if (GWO(song_volume, 2 * voice) != volume) {
        opl_set_volume(voice, volume);
        SWO(song_volume, 2 * voice, volume);
    }
    note_on(voice, (int)note);
}

/* a channel's event: the status' high bits say which, the low the voice */
static void song_channel(unsigned status)
{
    unsigned kind = status >> 4 & 7, voice = status & 0x0F;
    fptr cur = GFP(track_cur), p = pfp(cur, 0);

    if (voice < 0x0B)
        switch (kind) {
        case 0:
            note_off(voice);
            break;
        case 1:
            song_note(voice, pb(p, 0), pb(p, 1));
            break;
        case 2:
            if (GW(song_sounding))
                opl_set_volume(voice, pb(p, 1));
            SWO(song_volume, 2 * voice, pb(p, 1));
            break;
        case 5:
            if (GW(song_sounding))
                opl_set_volume(voice, pb(p, 0));
            SWO(song_volume, 2 * voice, pb(p, 0));
            break;
        case 6:
            opl_set_bend(voice, pb(p, 1) << 7 | pb(p, 0));
            break;
        }
    spw(cur, 0, pw(cur, 0) + GWO(event_bytes, 2 * kind));
}

/* CODE:114E: the AdLib's own events */
static void song_adlib(unsigned code, fptr data)
{
    unsigned i;

    if (code == 1) {
        for (i = 0; i < 0x1C; i++)
            spw(TIMBRE_SCRATCH, 2 * i, pb(data, i + 1));
        opl_set_timbre(pb(data, 0), TIMBRE_SCRATCH);
    } else if (code == 2)
        opl_set_mode(pb(data, 0));
    else if (code == 3)
        opl_set_range(pb(data, 0));
}

static void song_meta(void)
{
    fptr cur = GFP(track_cur), p = pfp(cur, 0);
    uint32_t n;

    if (pb(p, 0) == 0x2F) {
        spb(GFP(status_ptr), 0, 0x2F);
        spw(cur, 0, pw(cur, 0) - 1);
    } else if (pb(p, 0) == 0x51) {
        p = fadd(p, 2);
        n = (uint32_t)pb(p, 0) << 16 | (uint32_t)pb(p, 1) << 8 | pb(p, 2);
        spw(cur, 0, pw(cur, 0) + 5);
        song_tempo(GW(song_division), n);
    } else if (pb(p, 0) == 0x7F) {
        spw(cur, 0, pw(cur, 0) + 1);
        n = song_number();
        p = pfp(cur, 0);
        if (pb(p, 0) == 0 && pb(p, 1) == 0 && pb(p, 2) == 0x3F)
            song_adlib(pb(p, 3) << 8 | pb(p, 4), fadd(p, 5));
        spw(cur, 0, pw(cur, 0) + (uint16_t)n);
    } else {
        spw(cur, 0, pw(cur, 0) + 1);
        n = song_number();
        spw(cur, 0, pw(cur, 0) + (uint16_t)n);
    }
}

/* the song's events until one lies ahead: how many periods of the timer
 * until then (the ticks by 8, the rest dropped); at the song's end it
 * begins again */
static unsigned song_step(void)
{
    unsigned ticks;

    bi_at("song_step");
    if (!GB(song_playing))
        return 1;
    do {
        fptr cur = GFP(track_cur), p = pfp(cur, 0);
        unsigned status;

        if (pb(p, 0) & 0x80) {
            spb(GFP(status_ptr), 0, pb(p, 0));
            spw(cur, 0, pw(cur, 0) + 1);
        }
        status = pb(GFP(status_ptr), 0);
        if (status == 0xF7 || status == 0xF0)
            spw(cur, 0, pw(cur, 0) + (uint16_t)song_number());
        else if (status == 0xFF)
            song_meta();
        else
            song_channel(status);
        ticks = song_next();
    } while (!ticks && !GB(song_ended));
    if (!ticks) {
        song_rewind(GFP(song_ptr));
        song_begin();
        return 1;
    }
    return ticks >> 3;
}

/* the song's timer: after the periods the last step gave, the next step */
static void song_timer(void)
{
    unsigned wait;

    SW(song_wait, GW(song_wait) - 1);
    if (GW(song_wait) || GB(song_busy) == 1)
        return;
    SB(song_busy, 1);
    for (;;) {
        wait = song_step();
        SB(song_busy, 0);
        if ((uint16_t)(0 - GW(song_wait)) < wait)
            break;
        SW(song_wait, 0);
    }
    SW(song_wait, wait);
}

static void song_silence(void)
{
    unsigned i;

    if (GB(song_timer_on)) {
        timer_remove(GW(song_timer_handle));
        SW(song_timer_handle, 0xFFFF);
    }
    SB(song_timer_on, 0);
    for (i = 0; i < 0x0B; i++) {
        opl_set_volume(i, 0);
        note_off(i);
    }
}

static int song_start(fptr song)
{
    unsigned i;

    SFP(song_ptr, song);
    for (i = 0; i < 0x0B; i++) {
        opl_set_volume(i, 0);
        note_off(i);
    }
    song_halt();
    song_silence();
    song_rewind(GFP(song_ptr));
    if (!GB(song_timer_on)) {
        timer_handler(FP(song_timer), song_timer);
        SW(song_timer_handle, timer_add(FP(song_timer), 0x10000));
        SB(song_timer_on, 1);
    }
    for (i = 0; i < 0x0B; i++)
        SWO(song_volume, 2 * i, 0);
    if (!GB(song_timer_on))
        return 0;
    song_rewind(GFP(song_ptr));
    song_begin();
    return 1;
}

/* ---- the effects ---- */

static void effect_silence(unsigned si)
{
    opl_set_timbre(GWO(fx_voice, si), FP(timbre_silent));
    note_off(GWO(fx_voice, si));
}

/* a channel's effect begun: its volume, its record of the effects' file
 * (40h bytes: the ticks, the frequency, the step, the timbre), the key */
static void effect_start(unsigned si)
{
    unsigned voice = GWO(fx_voice, si);
    fptr rec;

    if (GWO(fx_count, si) & 0x8000)
        return;
    opl_set_volume(voice, GWO(fx_volume, si));
    /* as the original: at the voice's number, not twice it */
    SWO(song_volume, voice, GWO(fx_volume, si));
    rec = MKFP(GWO(effects_loaded, 2), GWO(effects_loaded, 0) + (GWO(fx_number, si) << 6));
    SWO(fx_left, si, pw(rec, 0));
    SWO(fx_ticks, si, pw(rec, 0));
    SWO(fx_freq0, si, pw(rec, 2));
    SWO(fx_freq, si, pw(rec, 2));
    SWO(fx_step, si, pw(rec, 4));
    opl_set_timbre(voice, fadd(rec, 6));
    note_on(voice, 0x19);
}

/* the effects' timer, 72.8 times a second: each sounding channel's
 * frequency goes on by its step; at its end the effect begins again
 * while its count lasts */
static void effects_tick(void)
{
    int si;

    bi_at("effects_tick");
    for (si = 6; si >= 0; si -= 2) {
        unsigned f, voice = GWO(fx_voice, si);

        if (!GWO(fx_ticks, si))
            continue;
        f = (uint16_t)(GWO(fx_freq, si) + GWO(fx_step, si));
        SWO(fx_freq, si, f);
        opl_write(0xA0 + voice, f & 0xFF);
        opl_write(0xB0 + voice, (f >> 8 & 3) | 0x28);
        SWO(fx_ticks, si, GWO(fx_ticks, si) - 1);
        if (GWO(fx_ticks, si))
            continue;
        effect_silence((unsigned)si);
        SWO(fx_count, si, GWO(fx_count, si) - 1);
        if (GWO(fx_count, si))
            effect_start((unsigned)si);
    }
}

/* the effects asked for in effects_asked (four records of 6 bytes: the
 * effect, the volume, a count) are begun: a count of 0 nothing, a volume
 * below 0 the effect and volume of before, a count with bit 15 only
 * silences the channel */
void effects_start(void)
{
    unsigned ch;

    bi_at("effects_start");
    for (ch = 0; ch < 4; ch++) {
        unsigned count = GWO(effects_asked, 6 * ch + 4);

        SWO(effects_asked, 6 * ch + 4, 0);
        if (!count)
            continue;
        SWO(fx_asked, 2 * ch, 1);
        SWO(fx_count, 2 * ch, count);
        if (GWO(effects_asked, 6 * ch + 2) & 0x8000)
            continue;
        SWO(fx_volume, 2 * ch, GWO(effects_asked, 6 * ch + 2));
        SWO(fx_number, 2 * ch, GWO(effects_asked, 6 * ch));
    }
    for (ch = 0; ch < 4; ch++)
        if (GWO(fx_asked, 2 * ch) == 1) {
            SWO(fx_asked, 2 * ch, 0);
            SWO(fx_left, 2 * ch, 1);
            SWO(fx_ticks, 2 * ch, 1);
            effect_silence(2 * ch);
            effect_start(2 * ch);
        }
}

/* how loud a channel's voice is at most */
void effects_volume(int channel, int volume)
{
    unsigned voice = GWO(fx_voice, 2 * channel);

    SWO(voice_scale, 2 * voice, volume);
    opl_set_volume(voice, GWO(song_volume, 2 * voice));
}

/* ---- what the program calls ---- */

/* mode 1 the speaker, 2 the AdLib without looking for it; the port: the
 * AdLib whatever the mode */
int sound_init(int mode)
{
    (void)mode;
    opl_init(0x388);
    SW(adlib_found, 1);
    timer_handler(FP(effects_timer), effects_tick);
    timer_add(FP(effects_timer), 0x4000);
    return GW(adlib_found);
}

/* the song's file into memory; where it is is kept (twice: play_song's
 * second argument chooses between two that are the same) */
int load_song(fptr dest, fptr name, fptr work)
{
    fptr p = load_file(dest, name, work);

    SWO(song_loaded, 0, FOFF(p));
    SWO(song_loaded, 2, FSEG(p));
    SWO(song_loaded, 4, FOFF(p));
    SWO(song_loaded, 6, FSEG(p));
    return FSEG(p);
}

int play_song(int a, int b)
{
    (void)a;
    if (!GWO(song_loaded, 2))
        return 0;
    song_start(b ? MKFP(GWO(song_loaded, 6), GWO(song_loaded, 4))
                 : MKFP(GWO(song_loaded, 2), GWO(song_loaded, 0)));
    return 1;
}

void stop_song(void)
{
    song_silence();
}

fptr load_effects(fptr dest, fptr name, fptr work)
{
    fptr p = load_file(dest, name, work);

    SWO(effects_loaded, 0, FOFF(p));
    SWO(effects_loaded, 2, FSEG(p));
    return p;
}
