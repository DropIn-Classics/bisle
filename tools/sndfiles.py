#!/usr/bin/env python3
"""Battle Isle's music and sound effects (.SND, .PND, .MDI, .PDI and
.FXX, .PXX, .FX, .PX): read them, write them back, and play them as
BATTLE.EXE's sound routines do, into the AdLib's registers.

    sndfiles.py [GAMEDIR ...] [--list]
    sndfiles.py --run LOG

Without GAMEDIR the three games' folders are taken (with ISLE's two
intro folders and the ANIM folders).  One line per file: what it holds
and whether it, written back, gives the (unpacked) file's bytes; for a
song also how long it plays until it begins again.  --list prints a
song's events and an effect file's records.  --run LOG plays a run's
sounds again and compares every register write (below).

The files are packed in ISLE and DESERT (tpwmfiles.py).  Which file the
game loads: the name it is given ends in .SND or .FXX; unless the AdLib
is used, the letter after the dot is made a P (CODE:15F7, CODE:169E), so
.PND and .PXX are the PC speaker's.

A song (.SND, .PND; the intros' .MDI, .PDI) is a MIDI file as the player
(CODE:0B76 .. 13E4) reads it:

    "MThd", a length (4 bytes, the high byte first), then in the header
    +0Ah the number of tracks and +0Ch the division, ticks a quarter
    note (both high byte first); after the header's length the tracks,
    each "MTrk", a length and its events.  The player takes up to 16
    tracks, each from its start to its end event, and does not use the
    tracks' lengths but to find the next track.  (The files have one
    track, whose length is 4 bytes short: the end event not counted.)
    An event: the time since the track's event before in ticks (7 bits
    a byte, the high bit says another follows), a status byte (left out
    when it is the one before: a byte below 80h is data), its data.
        8v n x      note n of voice v off
        9v n x      note n on with the volume x; x 0 is note off
        Av n x      the voice's volume x          (2 bytes)
        Dv x        the voice's volume x          (1 byte)
        Ev l h      the pitch bend, h * 128 + l; 2000h is none
        Bv, Cv      2 and 1 bytes, nothing done
        F0, F7      a length and that many bytes, nothing done
        FF 2F       the track's end
        FF 51 03 t t t   the tempo, microseconds a quarter note
        FF 7F len 00 00 3F c c ...   the AdLib's own events by cc:
            1  a voice's timbre: the voice, then 28 bytes, 13 values
               for each of the voice's two operators and their two
               wave forms
            2  the mode: 1 the percussion mode (six voices and five
               drums), 0 nine voices
            3  the pitch bend's range in semitones (1 to 12)
        FF other    a length and that many bytes, nothing done
    A voice is the status' low four bits; only 0 to 10 are played.
    When every track is at its end the song begins again.

Its clock (CODE:0E9F, CODE:1547): the player has a timer of its own in
the program's timer module (T2354, whose periods are counts of the PIT,
1193182 a second) with the period (tempo / 125) * 1194 / division, eight
ticks of the song; an event's time in ticks is divided by 8 and the
rest dropped, so events less than 8 ticks apart come at once and a song
runs a little fast.  Until a tempo event the tempo is 500000 with a
division of 480.

An effect file (.FXX, .PXX; ANIM's and the intros' .FX, .PX) is records
of 40h bytes, the effect's number its place:

    +0   word   how many ticks it lasts
    +2   word   the frequency it starts with (the AdLib's 10 bits; the
                speaker's count is made from it)
    +4   word   what is added to the frequency each tick
    +6   28 words  the timbre, as a song's (the low bytes count)
    +3Eh word   not read by the program

The game asks for an effect with a record of 6 bytes for each of four
channels (the sound, the volume, a count) and CODE:16F8 starts them: a
count of 0 nothing, a volume below 0 keeps the sound and volume of
before, a count with bit 15 only silences the channel.  The four
channels are the voices 2, 4, 3 and 5, taken from the song while the
effect sounds; the effect's timer (CODE:16ED, 4000h PIT counts: 72.8 a
second) writes the frequency with the key on in octave 2 each tick, and
when the ticks are over silences the voice and starts the effect again
until the count is used up.

--run LOG.  LOG is the output of a run of ISLE/BATTLE.EXE with -dos and

    -log CODE:14BC -log CODE:14C7   every register and its value
    -log CODE:0B76 -log CODE:168D   a song started, stopped
    -log CODE:1334                  the player's step
    -log CODE:1711 -log CODE:172B -log CODE:1738   the effects asked for
    -log CODE:188A                  the effects' tick
    -log CODE:1792                  a voice's loudness set (CODE:177A)

The tool does at each of these what the program's routine does (the
song and the effect file are the last ones the run opened), with the
driver's tables taken from the player's BATTLE.EXE, and compares each
value the run wrote to the AdLib with its own, in order; then the times
between the player's steps with the timer's periods.

The player's step and the effects' tick run in the timer's interrupt,
between the writes of a routine the main program called (a song
started or stopped, effects started, a loudness set); the log does not
say where the routine stood, so the tool takes the interrupt to fall
right after the routine's last write and, where the run's next values
say otherwise, just before its next (Routine).  A write is two outputs,
the register's number and the value: an interrupt between the two
leaves the number of its own last write in the card, and the program's
value then goes to that register (read_log); the tool compares what the
program meant and counts these.

Checked (ISLE/BATTLE.EXE, the AdLib found):
  the title, 30 s without keys: TITEL.SND's first 145 steps, all 1889
      writes as played here;
  the battle against the computer to 215 s (HANDOFF.md's keys): the
      title, MENU.FXX's effects 0, 1 and 3, GAME.SND started three
      times and stopped twice, GAME.FXX's effects 0, 1 and 2, two
      changes of phase with fights (FIGHT.FXX's effects 0, 3 and 5 with
      counts of 1, C8h and FFFFh, the loudness set), 1189 steps of the
      player, 15467 ticks of the effects: all 51525 writes as played
      here; three interrupts taken to fall just before a write, two
      inside one (a value 00 and a value F8h went to register B5h, the
      key and octave of voice 5, instead of 2Dh and 6Dh).
  The waits between the player's steps were its timer's periods to a
  millisecond in 1104 of 1185; 79 were up to 2.7 ms off, a step late
  and the next as much early (the interrupt held up), and two were 284 and 304 ms
  late, at 4.5 s and 13.8 s, where the picture fades: set_palette has
  the interrupts off, so ticks are lost and the song drags in a fade.
Not run: the speaker (.PND, .PXX; started with /s), a song's end and
beginning again, a pitch bend other than none (the songs have Ev events;
whether one of them came in the runs' steps was not looked for), a
volume below 0, the songs and effects of the animations and the intros,
DESERT.EX2 and MOON.EXE.  The 68 files of the three games are written
back identical; ISLE's and DESERT's LOOSER.PND is cut short (3000h
bytes unpacked, no end event).
"""
import argparse
import copy
import os
import re
import struct
import sys
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir  # noqa: E402
import tpwmfiles  # noqa: E402

SONGS = ('.SND', '.PND', '.MDI', '.PDI')
EFFECTS = ('.FXX', '.PXX', '.FX', '.PX')
RECORD = 0x40                           # an effect's record
PARAMS = 28                             # a timbre's values: 13 and 13, two wave forms
PIT = 1193182                           # the PIT's counts a second
ADLIB = b'\x00\x00\x3F'                   # begins the AdLib's own events (FF 7F)
FX_PERIOD = 0x4000                      # the effects' timer with the AdLib
FX_VOICE_NOTE = 0x19                    # the note an effect's voice is started with
MAX_TRACKS = 16
VOICES = 11                             # voices a song can name
# a channel event's bytes after the status, by (status >> 4) & 7 (DATA:143F)
EVENT_BYTES = (2, 2, 2, 2, 1, 1, 2)
# BATTLE.EXE: the frame of its DATA, and the driver's tables there
DATA = 0x2D8D
T_SCALE, T_PERC_BITS = 0x1336, 0x134C
T_MODULATOR, T_CARRIER = 0x1351, 0x135F
T_DRUMS = ((12, 0x136D), (15, 0x137B), (16, 0x1389), (14, 0x1397), (17, 0x13A5), (13, 0x13B3))
T_SLOTS, T_SLOTS_PERC = 0x13C1, 0x13D3
T_OFFSET, T_IS_CARRIER, T_VOICE, T_VOICE_PERC = 0x13E9, 0x13FB, 0x140D, 0x141F
T_FNUM, T_OCTAVE, T_NOTE = 0x144E, 0x15CE, 0x162E
T_FX_VOICE = 0x16D6
C_SILENT = 0x1986                       # CODE: the timbre a channel is silenced with


def unpacked(path):
    data = open(path, 'rb').read()
    return tpwmfiles.unpack(data) if data[:4] == tpwmfiles.MAGIC else data


def varlen(data, i):
    """a number of 7 bits a byte as CODE:0D0F reads it: (value, next)"""
    v = data[i] & 0x7F
    while data[i] & 0x80:
        i += 1
        v = v << 7 | data[i] & 0x7F
    return v, i + 1


def put_varlen(v):
    out = [v & 0x7F]
    while v > 0x7F:
        v >>= 7
        out.append(0x80 | v & 0x7F)
    return bytes(reversed(out))


def read_song(data):
    """{'head': the header's bytes after the length, 'tracks': [(events,
    length, rest)], 'tail', 'cut'}; an event is (ticks, bytes), the bytes
    with the status when the file has it; length is the track's as the
    file says it, rest what the track holds after its end event; cut: the
    file ends before the last track's end event (the bytes left are the
    tail)"""
    if data[:4] != b'MThd':
        raise ValueError('no MThd')
    n = struct.unpack_from('>I', data, 4)[0]
    head = data[8:8 + n]
    if n < 6:
        raise ValueError('a header of %d bytes' % n)
    count = struct.unpack_from('>H', head, 2)[0]
    if count > MAX_TRACKS:
        raise ValueError('%d tracks, the player has room for %d' % (count, MAX_TRACKS))
    i = 8 + n
    tracks = []
    cut = False
    for number in range(count):
        if data[i:i + 4] != b'MTrk':
            raise ValueError('no MTrk at %d' % i)
        size = struct.unpack_from('>I', data, i + 4)[0]
        end = i + 8 + size
        i += 8
        events = []
        status = 0
        while True:
            try:
                ticks, j = varlen(data, i)
                k = j
                if data[k] & 0x80:
                    status = data[k]
                    k += 1
                if status in (0xF0, 0xF7):
                    size2, k = varlen(data, k)
                    k += size2
                elif status == 0xFF:
                    kind = data[k]
                    size2, k = varlen(data, k + 1)
                    k += size2
                elif (status >> 4) & 7 < len(EVENT_BYTES) and status & 0x80:
                    k += EVENT_BYTES[(status >> 4) & 7]
                else:
                    raise ValueError('status %02X at %d: the player has no length for it' % (status, j))
                if k > len(data):
                    raise IndexError
            except IndexError:
                cut = True
                break
            events.append((ticks, data[j:k]))
            i = k
            if status == 0xFF and kind == 0x2F:
                break
        if (cut or i > end) and number < count - 1:
            raise ValueError('track %d ends after its length and is not the last' % number)
        tracks.append((events, size, b'' if cut else data[i:end]))
        i = max(i, end) if not cut else i
    return {'head': head, 'tracks': tracks, 'tail': data[i:], 'cut': cut}


def write_song(song):
    out = [b'MThd', struct.pack('>I', len(song['head'])), song['head']]
    for events, size, rest in song['tracks']:
        out += [b'MTrk', struct.pack('>I', size), b''.join(put_varlen(t) + b for t, b in events), rest]
    return b''.join(out) + song['tail']


def read_effects(data):
    """[(ticks, frequency, step, the 28 words, the last word)]"""
    if len(data) % RECORD:
        raise ValueError('%d bytes, not records of %d' % (len(data), RECORD))
    out = []
    for i in range(0, len(data), RECORD):
        w = struct.unpack_from('<%dH' % (RECORD // 2), data, i)
        out.append((w[0], w[1], w[2], w[3:3 + PARAMS], w[3 + PARAMS]))
    return out


def write_effects(records):
    return b''.join(struct.pack('<%dH' % (RECORD // 2), t, f, s, *p, last) for t, f, s, p, last in records)


def s16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


class Tables:
    """the driver's tables, from the player's BATTLE.EXE"""

    def __init__(self, path):
        exe = open(path, 'rb').read()
        if exe[:2] != b'MZ':
            raise ValueError('%s is no program' % path)
        image = struct.unpack_from('<H', exe, 8)[0] * 16
        d = image + DATA * 16

        def bytes_at(off, n):
            return list(exe[d + off:d + off + n])
        self.scale = list(struct.unpack_from('<%dH' % VOICES, exe, d + T_SCALE))
        self.perc_bits = bytes_at(T_PERC_BITS, 5)
        self.modulator = bytes_at(T_MODULATOR, 13)
        self.carrier = bytes_at(T_CARRIER, 13)
        self.drums = [(slot, bytes_at(off, 13)) for slot, off in T_DRUMS]
        self.slots = bytes_at(T_SLOTS, 18)
        self.slots_perc = bytes_at(T_SLOTS_PERC, 22)
        self.offset = bytes_at(T_OFFSET, 18)
        self.is_carrier = bytes_at(T_IS_CARRIER, 18)
        self.voice = bytes_at(T_VOICE, 18)
        self.voice_perc = bytes_at(T_VOICE_PERC, 18)
        self.fnum = struct.unpack_from('<192H', exe, d + T_FNUM)
        self.octave = bytes_at(T_OCTAVE, 96)
        self.note = bytes_at(T_NOTE, 96)
        self.fx_voice = struct.unpack_from('<4H', exe, d + T_FX_VOICE)
        self.silent = struct.unpack_from('<%dH' % PARAMS, exe, image + C_SILENT)


class Sound:
    """BATTLE.EXE's sound routines with an AdLib: the registers' driver
    (CODE:0215 .. 0B5F), the songs' player (CODE:0B60 .. 13E4) and the
    effects (CODE:16ED .. 19BF).  Every value written to a register is
    added to self.out as (register, value)."""

    def __init__(self, tables):
        self.t = tables
        self.out = []
        self.gate = None                        # called before and after each write (Routine)
        self.scale = list(tables.scale)         # DATA:1336, a voice's loudness
        self.params = [[0] * 14 for _ in range(18)]     # DATA:1B33, an operator's values
        self.vol = [0] * VOICES                 # DATA:1AF6
        self.bend = [0] * 9                     # DATA:1B1E
        self.keyon = [0] * 9                    # DATA:1AED
        self.note = [0] * 9                     # DATA:1B08
        self.last = [0] * 9                     # DATA:1B14, the value last written to B0h + voice
        self.perc = self.perc_bits = 0          # DATA:1C2F, 1B31
        self.voices = 0                         # DATA:1B02
        self.am = self.vib = self.notesel = 0   # DATA:1B30, 1B32, 1AEC
        self.wave = 0                           # DATA:1B06
        self.range = 0                          # DATA:1B12
        self.adlib = 0                          # DATA:16EC
        # the player
        self.music = 1                          # DATA:1436
        self.song = None
        self.volumes = bytearray(2 * VOICES + 2)        # DATA:1CCA, words
        self.playing = 0                        # DATA:1CE6
        self.ended = 0                          # DATA:1CC4
        self.period = 0                         # its timer's period in PIT counts
        self.again = False                      # the song came to its end and began again
        # the effects, four channels
        self.fx = None
        self.fx_count = [0] * 4                 # DATA:16A6
        self.fx_vol = [0] * 4                   # DATA:16AE
        self.fx_sound = [0] * 4                 # DATA:16B6
        self.fx_left = [0] * 4                  # DATA:16BE
        self.fx_freq = [0] * 4                  # DATA:16C6
        self.fx_step = [0] * 4                  # DATA:169E

    # ---- the registers (CODE:0215 .. 0B5F, 13E6 .. 14F1)

    def write(self, reg, value):
        if self.gate:
            self.gate(True)
        self.out.append((reg & 0xFF, value & 0xFF))
        if self.gate:
            self.gate(False)

    def clone(self):
        """a copy to try a routine on: its own state, no writes yet"""
        c = copy.copy(self)
        for name, value in vars(self).items():
            if isinstance(value, (list, bytearray)):
                setattr(c, name, copy.deepcopy(value))
        c.out = []
        c.gate = None
        return c

    def probe(self):
        """adlib_probe's writes; the status it reads is not modelled"""
        for reg, value in ((4, 0x60), (4, 0x80), (2, 0xFF), (4, 0x21), (4, 0x60), (4, 0x80)):
            self.write(reg, value)
        return 1

    def init(self, mode=0):
        """sound_init (CODE:1588): the probe, the registers cleared, the
        timbres the driver starts with"""
        found = self.probe()
        for reg in range(1, 0xF6):
            self.write(reg, 0)
        self.write(4, 6)
        for v in range(9):
            self.bend[v] = 0x2000
            self.keyon[v] = self.note[v] = 0
        self.vol = [0x7F] * VOICES
        self.set_mode(0)
        self.set_depths(0, 0, 0)
        self.set_range(1)
        self.set_wave(1)
        self.adlib = 0 if mode == 1 else 1 if mode == 2 else found

    def set_mode(self, perc):                   # CODE:02AE
        if perc:
            self.note[8], self.bend[8] = 0x18, 0x2000
            self.freq(8)
            self.note[7], self.bend[7] = 0x1F, 0x2000
            self.freq(7)
        self.perc = perc & 0xFF
        self.voices = 11 if perc else 9
        self.perc_bits = 0
        for slot in range(18):
            self.set_operator(slot, self.t.carrier if self.t.is_carrier[slot] else self.t.modulator, 0)
        if self.perc:
            for slot, values in self.t.drums:
                self.set_operator(slot, values, 0)
        self.write_bd()

    def set_wave(self, on):                     # CODE:0301
        self.wave = 0x20 if on else 0
        for slot in range(18):
            self.write(0xE0 + self.t.offset[slot], 0)
        self.write(1, self.wave)

    def set_range(self, n):                     # CODE:0340
        self.range = min(max(n, 1), 12)

    def set_depths(self, am, vib, notesel):     # CODE:035E
        self.am, self.vib, self.notesel = am, vib, notesel
        self.write_bd()
        self.write_08()

    def voice_slots(self, v):
        t = self.t.slots_perc if self.perc else self.t.slots
        return t[2 * v], t[2 * v + 1]

    def set_timbre(self, v, words):             # CODE:037B
        if not self.adlib or v >= self.voices:
            return
        a, b = self.voice_slots(v)
        self.set_operator(a, [w & 0xFF for w in words[0:13]], words[26])
        if b != 0xFF:
            self.set_operator(b, [w & 0xFF for w in words[13:26]], words[27])

    def set_volume(self, v, x):                 # CODE:042F
        if v >= self.voices:
            return
        x = min(x & 0xFFFF, 0x7F)
        self.vol[v] = ((self.scale[v] * x & 0xFFFF) // 0x7F) & 0xFF
        a, b = self.voice_slots(v)
        self.write_level(a)
        if b != 0xFF:
            self.write_level(b)

    def melodic(self, v, top):
        return (not self.perc and v < 9) or v < top

    def set_bend(self, v, x):                   # CODE:04B5
        if (not self.perc and v < 9) or v <= 6:
            self.bend[v] = min(x, 0x3FFF)
            self.freq(v)

    def note_on(self, v, n):                    # CODE:04EC
        n = max(n - 12, 0)
        if self.melodic(v, 6):
            self.note[v] = n & 0xFF
            self.keyon[v] = 0x20
            self.freq(v)
        elif self.perc and v <= 10:
            if v == 6:
                self.note[6] = n & 0xFF
                self.freq(6)
            elif v == 8 and self.note[8] != n:
                self.note[8] = n & 0xFF
                self.note[7] = n + 7 & 0xFF
                self.freq(8)
                self.freq(7)
            self.perc_bits |= self.t.perc_bits[v - 6]
            self.write_bd()

    def note_off(self, v):                      # CODE:0597
        if self.melodic(v, 6):
            self.keyon[v] = 0
            self.last[v] &= 0xDF
            self.write(0xB0 + v, self.last[v])
        elif self.perc and v <= 10:
            self.perc_bits &= ~self.t.perc_bits[v - 6] & 0xFF
            self.write_bd()

    def set_operator(self, slot, values, wave):         # CODE:06CD
        self.params[slot] = list(values[:13]) + [wave & 3]
        self.write_bd()
        self.write_08()
        self.write_level(slot)
        p = self.params[slot]
        off = self.t.offset[slot]
        if not self.t.is_carrier[slot]:
            self.write(0xC0 + self.t.voice[slot], p[2] << 1 | (0 if p[12] else 1))
        self.write(0x60 + off, p[3] << 4 | p[6] & 0x0F)
        self.write(0x80 + off, p[4] << 4 | p[7] & 0x0F)
        self.write(0x20 + off, (0x80 if p[9] else 0) + (0x40 if p[10] else 0) + (0x20 if p[5] else 0) +
                   (0x10 if p[11] else 0) + (p[1] & 0x0F))
        self.write(0xE0 + off, p[13] & 3 if self.wave else 0)

    def write_level(self, slot):                # CODE:07F4
        v = (self.t.voice_perc if self.perc else self.t.voice)[slot]
        p = self.params[slot]
        level = 0x3F - (p[8] & 0x3F)
        if self.t.is_carrier[slot] or (self.perc and v > 6):
            level = (self.vol[v] * level + 0x40) >> 7
        self.write(0x40 + self.t.offset[slot], 0x3F - level | p[0] << 6)

    def write_08(self):                         # CODE:0897
        self.write(8, 0x40 if self.notesel else 0)

    def write_bd(self):                         # CODE:0A19
        self.write(0xBD, (0x80 if self.am else 0) | (0x40 if self.vib else 0) |
                   (0x20 if self.perc else 0) | self.perc_bits)

    def freq(self, v):                          # CODE:0A99, CODE:13E6
        x = self.bend[v] - 0x2000 & 0xFFFF
        if x:
            x = (s16(x) >> 5) * self.range & 0xFFFF
        x = s16(x + (self.note[v] << 8) + 8) >> 4       # sixteenths of a semitone
        x = 0 if x < 0 else min(x, 0x5FF)
        f = self.t.fnum[self.t.note[x >> 4] * 16 + (x & 0x0F)]
        octave = self.t.octave[x >> 4] - 1 & 0xFF
        if f & 0x8000:
            octave = octave + 1 & 0xFF
        if octave & 0x80:
            octave = octave + 1 & 0xFF
            f = s16(f) >> 1 & 0xFFFF
        self.write(0xA0 + v, f)
        self.last[v] = (f >> 8 & 3) + (octave << 2) + self.keyon[v] & 0xFF
        self.write(0xB0 + v, self.last[v])

    # ---- the songs (CODE:0B60 .. 13E4)

    def get_volume(self, i):
        return self.volumes[i] | self.volumes[i + 1] << 8

    def put_volume(self, i, x):
        self.volumes[i] = x & 0xFF
        self.volumes[i + 1] = x >> 8 & 0xFF

    def silence(self):                          # CODE:0BED's loop
        for v in range(VOICES):
            self.set_volume(v, 0)
            self.note_off(v)

    def stop(self):                             # CODE:168D
        self.silence()

    def start(self, song):                      # CODE:0B76
        self.song = song
        self.silence()
        self.playing = 0
        self.silence()
        for v in range(VOICES):
            self.put_volume(2 * v, 0)
        self.rewind()

    def rewind(self):                           # CODE:0D9F, CODE:0E37
        d = self.song
        n = struct.unpack_from('>I', d, 4)[0]
        count, self.division = struct.unpack_from('>HH', d, 0x0A)
        i = 8 + n
        self.at = []
        for _ in range(count):
            self.at.append(i + 8)
            i += 8 + struct.unpack_from('>I', d, i + 4)[0]
        self.time = []
        self.status = []
        for k in range(count):
            t, self.at[k] = varlen(d, self.at[k])
            self.time.append(t)
            self.status.append(d[self.at[k]])
        self.now = 0
        self.cur = 0
        self.ended = 0
        self.playing = 1
        self.set_tempo(480, 500000)

    def set_tempo(self, division, tempo):       # CODE:0E9F
        period = tempo // 125 * 1194 // division & 0xFFFF if division else 0
        self.period = period or 0x10000

    def step(self):
        """CODE:1334: the events up to the next that is later; how many
        of the timer's periods to wait"""
        if not self.playing:
            return 1
        d = self.song
        while True:
            k = self.cur
            if d[self.at[k]] & 0x80:
                self.status[k] = d[self.at[k]]
                self.at[k] += 1
            st = self.status[k]
            if st in (0xF0, 0xF7):
                n, i = varlen(d, self.at[k])
                self.at[k] = i + n
            elif st == 0xFF:
                self.meta(k)
            else:
                self.channel(k, st)
            wait = self.next_event()
            if wait or self.ended:
                break
        if not wait:
            self.rewind()
            self.again = True
            return 1
        return wait >> 3

    def channel(self, k, st):                   # CODE:103C
        d, i = self.song, self.at[k]
        kind, v = st >> 4 & 7, st & 0x0F
        if v < VOICES:
            if kind == 0:
                self.note_off(v)
            elif kind == 1:
                self.note_event(v, d[i], d[i + 1])
            elif kind in (2, 5):
                x = d[i + 1] if kind == 2 else d[i]
                if self.music:
                    self.set_volume(v, x)
                self.put_volume(2 * v, x)
            elif kind == 6:
                self.set_bend(v, d[i + 1] << 7 | d[i])
        self.at[k] = i + EVENT_BYTES[kind]

    def note_event(self, v, n, x):              # CODE:0FE7
        if not self.music:
            self.put_volume(2 * v, x)
        elif not x:
            self.note_off(v)
            self.put_volume(2 * v, 0)
        else:
            if self.get_volume(2 * v) != x:
                self.set_volume(v, x)
                self.put_volume(2 * v, x)
            self.note_on(v, n)

    def meta(self, k):                          # CODE:11C4
        d, i = self.song, self.at[k]
        if d[i] == 0x2F:
            self.status[k] = 0x2F
            self.at[k] = i - 1
        elif d[i] == 0x51:
            self.set_tempo(self.division, d[i + 2] << 16 | d[i + 3] << 8 | d[i + 4])
            self.at[k] = i + 5
        else:
            kind = d[i]
            n, i = varlen(d, i + 1)
            if kind == 0x7F and d[i:i + 3] == ADLIB:
                code = d[i + 3] << 8 | d[i + 4]
                if code == 1:
                    self.set_timbre(d[i + 5], list(d[i + 6:i + 6 + PARAMS]))
                elif code == 2:
                    self.set_mode(d[i + 5])
                elif code == 3:
                    self.set_range(d[i + 5])
            self.at[k] = i + n

    def next_event(self):                       # CODE:0EF8
        d, k = self.song, self.cur
        if self.status[k] != 0x2F:
            t, self.at[k] = varlen(d, self.at[k])
            self.time[k] += t
        else:
            self.time[k] = 0x7FFFFFFF
        best = 0
        for i in range(1, len(self.time)):
            if self.time[i] < self.time[best] and self.status[i] != 0x2F:
                best = i
        if self.status[best] == 0x2F:
            self.ended = 1
            self.playing = 0
            return 0
        wait = self.time[best] - self.now & 0xFFFF
        self.now = self.time[best]
        self.cur = best
        return wait

    # ---- the effects (CODE:16ED .. 19BF)

    def fx_ask(self, asked):
        """CODE:16F8 with the four channels' (count, volume, sound); the
        volume and the sound are None where the routine does not read them"""
        new = []
        for ch, (count, vol, sound) in enumerate(asked):
            if not count:
                continue
            new.append(ch)
            self.fx_count[ch] = count
            if vol is not None and not vol & 0x8000:
                self.fx_vol[ch] = vol
                self.fx_sound[ch] = sound
        for ch in new:
            self.fx_left[ch] = 1
            self.fx_silence(ch)
            self.fx_start(ch)

    def fx_silence(self, ch):                   # CODE:1964
        v = self.t.fx_voice[ch]
        self.set_timbre(v, self.t.silent)
        self.note_off(v)

    def fx_start(self, ch):                     # CODE:17A7
        if self.fx_count[ch] & 0x8000:
            return
        v = self.t.fx_voice[ch]
        self.set_volume(v, self.fx_vol[ch])
        self.put_volume(v, self.fx_vol[ch])     # at the voice, not twice the voice: as the program
        ticks, f, step, params, _ = self.fx[self.fx_sound[ch]]
        self.fx_left[ch], self.fx_freq[ch], self.fx_step[ch] = ticks, f, step
        self.set_timbre(v, params)
        self.note_on(v, FX_VOICE_NOTE)

    def fx_tick(self):                          # CODE:188A
        for ch in (3, 2, 1, 0):
            if not self.fx_left[ch]:
                continue
            v = self.t.fx_voice[ch]
            f = self.fx_freq[ch] = self.fx_freq[ch] + self.fx_step[ch] & 0xFFFF
            self.write(0xA0 + v, f)
            self.write(0xB0 + v, f >> 8 & 3 | 0x28)
            self.fx_left[ch] = self.fx_left[ch] - 1 & 0xFFFF
            if not self.fx_left[ch]:
                self.fx_silence(ch)
                self.fx_count[ch] = self.fx_count[ch] - 1 & 0xFFFF
                if self.fx_count[ch]:
                    self.fx_start(ch)

    def fx_scale(self, v, x):                   # CODE:177A, from the voice on
        self.scale[v] = x
        self.set_volume(v, self.get_volume(2 * v))


def song_length(tables, data):
    """(seconds, steps) a song plays until it begins again, by its
    timer's periods"""
    s = Sound(tables)
    s.init()
    s.start(data)
    counts = steps = 0
    while steps < 1000000:
        before = s.period
        wait = s.step()
        steps += 1
        if s.again:
            return counts / PIT, steps
        counts += (before + (wait - 1) * s.period) if wait else 0
    raise ValueError('no end')


def describe(event):
    ticks, b = event
    st = b[0]
    if st < 0x80:
        return 'data %s' % b.hex()
    if st == 0xFF:
        kind = b[1]
        n, i = varlen(b, 2)
        body = b[i:]
        if kind == 0x2F:
            return 'end'
        if kind == 0x51:
            return 'tempo %d' % int.from_bytes(body, 'big')
        if kind == 0x7F and body[:3] == ADLIB:
            code = body[3] << 8 | body[4]
            if code == 1:
                return 'timbre of voice %d: %s' % (body[5], ' '.join('%X' % x for x in body[6:]))
            if code == 2:
                return 'mode %d' % body[5]
            if code == 3:
                return 'bend range %d' % body[5]
        return 'meta %02X, %d bytes' % (kind, n)
    kind, v = st >> 4 & 7, st & 0x0F
    if st in (0xF0, 0xF7):
        return 'F%X, %d bytes' % (st & 15, len(b) - 1)
    name = ('off', 'on', 'volume', 'B', 'C', 'volume', 'bend')[kind]
    return 'voice %d %s %s' % (v, name, ' '.join('%d' % x for x in b[1:]))


def list_song(song):
    for n, (events, _, rest) in enumerate(song['tracks']):
        t = 0
        for ticks, b in events:
            t += ticks
            text = describe((ticks, b)) if b[0] & 0x80 else 'the same, %s' % ' '.join('%d' % x for x in b)
            print('    track %d %7d %s' % (n, t, text))


def song_line(song, tables, data):
    events = [e for ev, _, _ in song['tracks'] for e in ev]
    used = sorted({b[0] & 0x0F for _, b in events if 0x80 <= b[0] < 0xF0})
    fmt, count, division = struct.unpack_from('>HHH', song['head'])
    timbres = sum(1 for e in events if describe(e).startswith('timbre'))
    line = 'a song: format %d, %d track%s, division %d, %d events, voices %s, %d timbres' % (
        fmt, count, '' if count == 1 else 's', division, len(events), ' '.join('%d' % v for v in used), timbres)
    over = [sum(len(put_varlen(t)) + len(b) for t, b in ev) - size for ev, size, _ in song['tracks']]
    if any(over) and not song['cut']:
        line += ", the events %s bytes longer than the track's length" % ' '.join('%d' % n for n in over)
    rest = sum(len(r) for _, _, r in song['tracks'])
    if rest:
        line += ', %d bytes after a track\'s end' % rest
    if song['cut']:
        line += ', CUT: the file ends before the end event (%d bytes left)' % len(song['tail'])
    elif song['tail']:
        line += ', %d bytes after the tracks' % len(song['tail'])
    if tables and not song['cut']:
        seconds, steps = song_length(tables, data)
        line += ', %.1f s in %d steps' % (seconds, steps)
    return line


def effects_line(records):
    longest = max(t for t, _, _, _, _ in records)
    return '%d effects, the longest %d ticks, the last words %s' % (
        len(records), longest, ' '.join(sorted({'%04X' % last for _, _, _, _, last in records})))


def list_effects(records):
    for n, (ticks, f, step, params, last) in enumerate(records):
        print('    %2d %5d ticks (%.2f s), from %d by %d: %s' % (
            n, ticks, ticks * FX_PERIOD / PIT, f, s16(step), ' '.join('%X' % p for p in params)))


def sound_files(root, dirs):
    """(shown name, path) of every song and effect file"""
    for d in dirs:
        for folder, _, names in os.walk(d):
            for name in sorted(names):
                ext = os.path.splitext(name)[1].upper()
                if ext in SONGS or ext in EFFECTS:
                    path = os.path.join(folder, name)
                    yield os.path.relpath(path, root), path, ext


def find_tables(root):
    for name in ('BATTLE.EXE', 'battle.exe'):
        path = os.path.join(root, 'ISLE', name)
        if os.path.isfile(path):
            return Tables(path)
    return None


def find_file(root, name):
    """a file of ISLE by the name a run opened it with"""
    parts = name.replace('/', '\\').split('\\')
    d = os.path.join(root, 'ISLE')
    for part in parts:
        for f in os.listdir(d):
            if f.upper() == part.upper():
                d = os.path.join(d, f)
                break
        else:
            raise ValueError('no %s in ISLE' % name)
    return d


class Routine:
    """one of the sound routines the main program calls, run a register
    write at a time: the timer's routines (the player's step, the
    effects' tick) come in between its writes, as in the run.  It stops
    right after each write and again just before the next (the value
    reckoned): an interrupt between two writes fell either into the
    port's reads after the first or into what the routine does up to
    the second, and replay() takes the one the run's values agree with.
    What it does after its last write is done with that write; how many
    writes it has is found by doing it on a copy first."""

    def __init__(self, sound, call):
        trial = sound.clone()
        call(trial)
        self.n = len(trial.out)
        self.sound = sound
        self.call = call
        self.count = 0
        self.over = False
        self.before = False             # it stands just before a write
        self.thread = None
        self.go = threading.Semaphore(0)
        self.back = threading.Semaphore(0)
        if not self.n:
            call(sound)
            self.over = True

    def run(self):
        self.sound.gate = self.wait
        try:
            self.call(self.sound)
        finally:
            self.sound.gate = None
            self.over = True
            self.back.release()

    def wait(self, before):
        if threading.current_thread() is not self.thread:
            return
        if not before:
            self.count += 1
            if self.count >= self.n:
                return
        self.before = before
        self.back.release()
        self.go.acquire()

    def advance(self):
        """to its next stop"""
        if self.thread:
            self.go.release()
        else:
            self.thread = threading.Thread(target=self.run, daemon=True)
            self.thread.start()
        self.back.acquire()

    def up_to_write(self):
        if not self.over and not self.before:
            self.advance()

    def write(self):
        self.up_to_write()
        if not self.over:
            self.advance()

    def finish(self):
        while not self.over:
            self.advance()


LOG = re.compile(r'^log \S+:([0-9A-F]{4}) t=([0-9.]+) hit=\d+ regs .* AX=([0-9A-F]{4}) .* SI=([0-9A-F]{4}) ')
OPEN = re.compile(r'^int21 AX=3D.* (\S+)\s*$')


def read_log(log):
    """a run's log as [(address, time, AX, SI)], a register write as
    ('w', time, register, value, the register meant), a file opened as
    ('open', name, 0, 0).  A write is two outputs, the register's number
    and the value; when the timer's interrupt falls between them, its
    routines' writes leave another number in the card and the value goes
    to that register: ('reg', time, the register meant, 0) where the
    write is cut, and the write after the interrupt with both registers."""
    events = []
    reg = last = None
    cut = []
    for line in open(log, encoding='latin-1'):
        m = OPEN.match(line)
        if m:
            events.append(('open', m.group(1), 0, 0))
            continue
        m = LOG.match(line)
        if not m:
            continue
        at, t, ax, si = int(m.group(1), 16), float(m.group(2)), int(m.group(3), 16), int(m.group(4), 16)
        if at == 0x14BC:
            reg = ax & 0xFF
        elif at == 0x14C7:
            if reg is None:
                events.append(('w', t, last, ax & 0xFF, cut.pop()))
            else:
                events.append(('w', t, reg, ax & 0xFF, reg))
                last, reg = reg, None
        else:
            if reg is not None:
                events.append(('reg', t, reg, 0))
                cut.append(reg)
                reg = None
            events.append((at, t, ax, si))
    return events


def replay(root, tables, log):
    """a run's log played again: (writes compared, [what differs],
    steps timed, [those more than a millisecond off], how often an
    interrupt fell just before a routine's write, how often inside one)"""
    s = Sound(tables)
    s.init()
    events = read_log(log)
    song = None
    seen = 0                    # writes of the run
    done = 0                    # of s.out compared
    bad = []
    asked = []
    routine = None
    steps = []                  # (time, the periods to wait, the period before, after); None: started anew
    late_falls = 0
    strays = 0

    def call(f):
        nonlocal routine
        if routine:
            routine.finish()
        routine = Routine(s, f)

    def ask():
        """the four channels are read: CODE:16F8 starts those asked for"""
        got = [tuple(x) for x in asked]
        call(lambda snd: snd.fx_ask(got))

    def interrupt(i, handler):
        """the timer's routine `handler` at event i: where the routine
        under way stands is taken from the writes that follow in the run"""
        nonlocal late_falls
        if routine and not routine.over and not routine.before:
            follow = []
            for e in events[i + 1:]:
                if e[0] != 'w':
                    break
                follow.append((e[2], e[3]))
            trial = s.clone()
            handler(trial)
            if trial.out != follow[:len(trial.out)]:
                routine.up_to_write()
                late_falls += 1
        return handler(s)

    for i, e in enumerate(events):
        at, t, ax, si = e[:4]
        if at == 'open':
            ext = os.path.splitext(t)[1].upper()
            if ext in SONGS:
                song = unpacked(find_file(root, t))
            elif ext in EFFECTS:
                s.fx = read_effects(unpacked(find_file(root, t)))
        elif at == 'reg':
            if routine:
                routine.up_to_write()
        elif at == 0x1711:
            if si == 0:
                asked = []
            asked.append([ax, None, None])
            if len(asked) == 4 and not ax:
                ask()
        elif at == 0x172B:
            asked[-1][1] = ax
            if len(asked) == 4 and ax & 0x8000:
                ask()
        elif at == 0x1738:
            asked[-1][2] = ax
            if len(asked) == 4:
                ask()
        elif at == 'w':
            got = (e[4], si)
            strays += e[4] != ax
            seen += 1
            if done == len(s.out) and routine and not routine.over:
                routine.write()
            want = s.out[done] if done < len(s.out) else None
            if got != want and len(bad) < 10:
                bad.append('write %d at %.3f s: the run %02X=%02X, here %s' % (
                    seen, t, got[0], got[1], '%02X=%02X' % want if want else 'nothing'))
            done += 1
        elif at == 0x0B76:
            data = song
            call(lambda snd: snd.start(data))
            steps.append(None)
        elif at == 0x168D:
            call(lambda snd: snd.stop())
            steps.append(None)
        elif at == 0x1792:
            v, x = si >> 1, ax
            call(lambda snd: snd.fx_scale(v, x))
        elif at == 0x1334:
            before = s.period
            steps.append((t, interrupt(i, lambda snd: snd.step()), before, s.period))
        elif at == 0x188A:
            interrupt(i, lambda snd: snd.fx_tick())
    if routine:
        routine.finish()
    if done < len(s.out):
        bad.append('%d writes here that the run does not have' % (len(s.out) - done))
    late = []
    timed = 0
    for a, b in zip(steps, steps[1:]):
        if a is None or b is None:
            continue
        (t0, wait, before, after), t1 = a, b[0]
        want = (before + (wait - 1) * after) / PIT if wait else 0.0
        timed += 1
        if abs(t1 - t0 - want) > 0.001:
            late.append((t0, t1 - t0 - want))
    return seen, bad, timed, late, late_falls, strays


def game_dirs(root):
    for game in ('ISLE', 'DESERT', 'MOON'):
        d = os.path.join(root, game)
        if os.path.isdir(d):
            yield d


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('dirs', nargs='*', metavar='GAMEDIR')
    ap.add_argument('--list', action='store_true', help="print the songs' events and the effects' records")
    ap.add_argument('--run', metavar='LOG', help="play a run's sounds again and compare the registers")
    a = ap.parse_args()
    root = game_dir()
    tables = find_tables(root)
    if a.run:
        if not tables:
            sys.exit('no ISLE/BATTLE.EXE for the tables')
        seen, bad, timed, late, falls, strays = replay(root, tables, a.run)
        for line in bad:
            print(line)
        print('%d register writes in the run, %s' % (
            seen, 'all as played here' if not bad else 'NOT all as played here'))
        print('%d times an interrupt taken to fall just before a write; %d times it fell inside one, whose value '
              'then went to another register' % (falls, strays))
        print("%d waits between the player's steps, %d of them as its timer's periods say to a millisecond" % (
            timed, timed - len(late)))
        for t, d in late[:20]:
            print('    the step at %.3f s: the next %+.1f ms' % (t, d * 1000))
        sys.exit(1 if bad else 0)
    total = bad = 0
    for rel, path, ext in sound_files(root, a.dirs or list(game_dirs(root))):
        total += 1
        try:
            data = unpacked(path)
            if ext in SONGS:
                got = read_song(data)
                same = write_song(got) == data
                line = song_line(got, tables, data)
            else:
                got = read_effects(data)
                same = write_effects(got) == data
                line = effects_line(got)
        except (OSError, ValueError, IndexError, struct.error) as e:
            print('%s %s' % (rel, e))
            bad += 1
            continue
        bad += not same
        print('%s %s, %s' % (rel, line, 'written back identical' if same else 'NOT identical'))
        if a.list:
            (list_song if ext in SONGS else list_effects)(got)
    print('%d files, %s' % (total, 'all ok' if not bad else '%d NOT ok' % bad))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
