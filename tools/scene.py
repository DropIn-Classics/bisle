#!/usr/bin/env python3
"""Battle Isle's fight scene (BATTLE.EXE's fight_step, T1F3C:000A), a
pass done again from a run's memory and compared with what the game made
of it, in its memory and on its screen.

    scene.py ENTRY [--exit RAM] [--vram VRAM] [--png DIR] [--game DIR]
             [--load SEG] [-v]

ENTRY is a run's memory (run.py -ram) stopped at fight_step (`-break
fight_step#N`), --exit and --vram the memory and the video memory of the
same run stopped at the next entry (`-break fight_step#N+1`).  The tool
does the pass that begins at ENTRY: what it changes in the scene's
values, and the picture it leaves.  With --exit it says whether the
game's memory has the same bytes (the scene's values F2D37:0008..0284,
the fight record's count of calls and, but after the last call, the two
units' records and the seed of rand), with --vram how many of the pixels
it drew the page shown has.
--png writes the picture (DIR/scene.png, 00.PAL's colours whatever the
scene's palette is, what is not drawn clear).  --game names the game's
folder (default ISLE), --load the segment the program was loaded at
(default 0077).

change_phase (T0408:000B) calls fight_step once a pass of its own loop
for each attack order: fight_step, then flip_page and copy_page, the
wait for 2 ticks, restore_sprites (which takes the pass's sprites off
the page drawn to), until fight_step returns 1.  So at an entry the page
shown holds the pass before in full.  Before the first fight
change_phase loads FIGHT.LIB, BUM.LIB, RAND.LIB and FIGHT.FXX (the
sounds; a run with -dos) and puts the libraries' lists of entries into
the fight record
(F27EE:2716; fight.py has its other fields):

    +26 long  the count of fight_step's calls in this fight
    +2C       bit 1: the sounds are played
    +32 far   UNIT.LIB's entries (in UNIT.DAT's order), 6 a type
    +3A far   BUM.LIB's: 0..5 an explosion, then 8 a kind of shot
    +3E far   RAND.LIB's: the frame's four pieces
    +42 far   FIGHT.LIB's: the ground's pieces
    +4A far   the sounds' records (6 bytes each), +4E far the routine
              that starts them (CODE:16F8); the tool leaves them out
    +56 far   draw_entry, +5A T24D8:0008 (draw_entry for an entry that
              may be packed), +5E srand, +62 random, +66 abs

The scene is in the attacker's half of the screen: x0 is 160 when the
attacker (A; the target is B) has bit 1 in its +4, else 0.

The first call sets F2D37:0008 and 0009 to 0, keeps the two counts (the
units' +2) in 00CC (A) and 01D9 (B), reckons (fight_reckon: fight.py)
and keeps the counts after it in 01DA (A) and 0067 (B).  The first two
calls draw the ground (T2112:000B), once for each page:

    the pieces of B's ground, then those of A's 90 (5Ah) lower: by the
    ground record's +5 (1..7) a list in F2D5F at 06h, 0Dh, 14h, 1Bh,
    22h, 3Bh, 54h: a count, then entry, x, y a piece; FIGHT.LIB's
    entry at x0 + x, y with colour base 5Fh;
    when B is not next to A (the record's +22h 0): srand(the offset of
    A's record), then for x = 0, 2, .. 150 a black column two pixels
    wide and random(8, 14) + 1 high around y 90;
    else where the two grounds differ: A's 2 or 5 and B's neither:
    entry 3 at (x0, 88) and (x0 + 76, 88); B's 2 or 5 and A's neither:
    entry 4 at (x0, 80) and (x0 + 76, 80); B's 7 and A's not: entry 7
    at y 80; A's 7 and B's not: entry 8 at y 88;
    the frame: RAND.LIB's entry 2 at (x0, 168), 0 at (x0, 0), 3 at
    (x0, 0), 1 at (x0 + 144, 0), colour base 40h.

Then, a call each (every call, the first two as well):

  while 0008 is 0, the units come in (T1F5A:093C; its calls counted in
  the long 005F).  A unit of the scene is 10h bytes, A's from 0068, B's
  from 00CE, as many as the count held within 1..6:
    +0 the direction (0 up .. 5, as on the map)   +1 hit: no longer drawn
    +2 far its place in the script                +6 how far still to go
    +8, +0Ah x and y in the scene                 +0Ch, +0Eh where drawn
  The first call gives each unit its script (T223C:0001: in F2D66, 128h
  for a unit with 4 or 20h in its +6, 0Eh for one with 10h in its +4, 40h
  for one with 8, else by its ground's +5: 72h, A6h, A6h, C8h, 0Eh, FEh,
  FEh), the nth unit of a side from the nth byte FEh on.  A call takes
  each unit a step on (T1F5A:0736): with nothing left to go the script's
  next: FEh, x, y, direction sets the unit there; FFh is the end (the
  unit is drawn and counts as arrived); else a direction and a length.
  With a length left the unit goes the byte F2D37:000E + its type of it
  (or what is left), by the direction: 0 y more, 1 x and y more, 2 x
  more and y less, 3 y less, 4 x and y less, 5 x less and y more.  Then
  it is drawn (T1F5A:0322): A's at x0 + x, 156 - y; B's at x0 + 128 - x,
  y, turned by 3 directions; UNIT.LIB's entry type * 6 + direction,
  colour base 20h for player 0's and 30h for player 1's; for a type with
  40h in its +0Ch (two squares) the next type's entry beside it (24 down,
  or 12 down and 16 left or right, by the direction; the other way round
  for B; the two entries exchanged for the directions 2, 3, 4), drawn
  first; for a unit with 10h in its +4 (in the air) its entry once more
  3 left and 2 lower in colour base 50h (a shadow), before it.  When all
  units of both sides have arrived, 0008 is 1.

  then while 0009 is 0, the shots (T1F5A:1A21, counted in the long
  0063).  A shot is 1Ch bytes, A's from 012E (their number 00CA), B's
  from 01DC (01D8):
    +0, +2 x, y          +4 flags: 1 flying, 2 exploding, 8 over, 4 A's,
    +5 far its entry        10h it hits, 20h not drawn, 40h its sound begun
    +9 the explosion's step (5 down)       +0Ah, +0Ch the signs of its way
    +0Eh, +10h the way's widths            +12h, +14h what is left over
    +16h the steps of the way              +18h those done
    +1Ah the unit it flies to              +1Bh the calls it waits first
  The first call sets them up (T1F5A:0C58 A's, T1F5A:101C B's): shot j
  of n flies from unit j (its place drawn, 6 right) to the other side's
  unit n - 1 - j (that modulo the other side's count when n is more),
  12 right and down of its place.  The other side's loss L (its count
  before less after): a shot whose n - 1 - j is below L hits (flag 10h),
  the others miss: their aim is random(-32, 32) lower.  The entry is
  BUM.LIB's 6 + 8 * kind + way, the kind F2D37:0029 + 2 * the firing
  type, the way one of eight (T1F5A:01E4: 0 up, 1 right up, 2 right, ..
  7 left up: upright or level when the other width is less than half).
  It waits random(0, 10000) AND 0Ch calls.  When L is more than n, the
  shots n .. L - 1 are made too: not drawn (flag 20h), at unit k's
  place, hitting unit k, lasting as long as shot k mod n flies.  B's are
  not made (0284 is 1) when B cannot answer: fight_reckon's rules, and
  here also when the squares are further apart than B's range (its
  type's +7 against a unit in the air, else +8, less 1).
  A call draws B's units, A's units, then steps each shot
  (T1F5A:1548): one that waits counts down; one that flies is drawn
  (base 0) and then goes as many steps as the byte F2D37:002A + 2 * its
  type says, a step of a line from its place to its aim; at its last
  step it explodes, and the unit it hits gets 1 in its +1; one that
  explodes shows BUM.LIB's entries 5 down to 0 at 12 left and up of its
  place, a call each, in colour base 30h when it hit, else 20h; then it
  is over.  When all are over, 0009 is 1.

  then the four sounds are stopped, the count of calls is 0 and
  fight_step returns 1 (no page is flipped after that call).

After the units and shots of a call the frame's pieces a sprite reached
into are drawn again (T1F5A:0176: above y 12 entry 0, below 144 entry 2,
left of x0 + 8 entry 3, right of x0 + 120 entry 1).  The sprites are
clipped by draw_entry to the drawing record's rectangle (DATA:00E4, from
the run's memory; 0, 0, 320, 179 in the runs: nothing but the scripts
keeps a sprite in its half).

Checked (HANDOFF.md has the runs): the first fight of the battle against
the computer, 38 of its 62 calls, and that fight with values poked in
before its first call (not next to each other; the attacker in the air;
a ship of two squares as the attacker and as the target; 6 against 2
and 2 against 6; other grounds and scripts), 90 calls: after each the
memory as the game's and every pixel drawn (27208) on the page shown.

Not done by the tool: the sounds (the records behind +4Ah: a sound for
each side's units when they come in, by the type's +41h, ended when they
have arrived unless the type's +43h has bit 1; one for each shot by the
type's +42h, and one for each explosion).
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir  # noqa: E402
import fight  # noqa: E402
import libfiles  # noqa: E402
import mapfiles  # noqa: E402
import screens  # noqa: E402

FAR, DATA = 0x27EE, 0x2D8D
RECORD = 0x2716                         # F27EE: the fight record
SCENE = 0x2D37                          # the frame of the scene's values
GROUNDS = 0x2D5F                        # the frame of the grounds' lists of pieces
SCRIPTS = 0x2D66                        # the frame of the units' scripts
SEED = 0x1938                           # DATA: rand's long
UNITS_A, UNITS_B, UNIT_REC = 0x68, 0xCE, 0x10
SHOTS_A, SHOTS_B, SHOT_REC = 0x12E, 0x1DC, 0x1C
PIECES = {1: 0x06, 2: 0x0D, 3: 0x14, 4: 0x1B, 5: 0x22, 6: 0x3B, 7: 0x54}
SCRIPT = {1: 0x72, 2: 0xA6, 3: 0xA6, 4: 0xC8, 5: 0x0E, 6: 0xFE, 7: 0xFE}
BESIDE = {0: (0, 0x18), 3: (0, 0x18), 1: (-0x10, 0x0C), 4: (-0x10, 0x0C), 2: (0x10, 0x0C), 5: (0x10, 0x0C)}
STEP = {0: (0, 1), 1: (1, 1), 2: (1, -1), 3: (0, -1), 4: (-1, -1), 5: (-1, 1)}


def s16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


class Mem:
    """a run's memory that can be written, read as fight.py's Ram"""

    def __init__(self, data, load):
        self.m = bytearray(data)
        self.load = load

    def lin(self, frame, off):
        return (self.load + frame) * 16 + off

    def b(self, a):
        return self.m[a]

    def w(self, a):
        return struct.unpack_from('<H', self.m, a)[0]

    def sw(self, a):
        return struct.unpack_from('<h', self.m, a)[0]

    def l(self, a):
        return struct.unpack_from('<I', self.m, a)[0]

    def far(self, a):
        off, seg = struct.unpack_from('<HH', self.m, a)
        return seg * 16 + off, (seg << 16) | off

    def setb(self, a, v):
        self.m[a] = v & 0xFF

    def setw(self, a, v):
        struct.pack_into('<H', self.m, a, v & 0xFFFF)

    def setl(self, a, v):
        struct.pack_into('<I', self.m, a, v & 0xFFFFFFFF)


class Scene:
    """a call of fight_step on a memory; .ops is what it drew: (list, entry,
    x, y, colour base) or ('column', x, y1, y2)"""

    def __init__(self, mem):
        m = self.m = mem
        self.rec = m.lin(FAR, RECORD)
        self.s = m.lin(SCENE, 0)
        self.ua, self.ub = m.far(self.rec)[0], m.far(self.rec + 4)[0]
        self.ta, self.tb = m.far(self.rec + 0x0A)[0], m.far(self.rec + 0x0E)[0]
        self.ga, self.gb = m.far(self.rec + 0x12)[0], m.far(self.rec + 0x16)[0]
        self.x0 = 0xA0 if m.w(self.ua + 4) & 1 else 0
        self.ops = []
        self.phase = ''

    # the library's rand and the game's random
    def srand(self, seed):
        self.m.setl(self.m.lin(DATA, SEED), seed & 0xFFFF)

    def random(self, lo, hi):
        a = self.m.lin(DATA, SEED)
        state, r = fight.rand(self.m.l(a))
        self.m.setl(a, state)
        return s16(r % ((min(hi, 0x7FFF) - lo + 1) & 0xFFFF) + lo)

    def touch(self, x, y):
        """T1F5A:0176: the frame's pieces a sprite at x, y reaches into"""
        s = self.s
        left, right = (0xA8, 0x118) if self.x0 else (8, 0x78)
        bits = (4 if x < left else 0) | (8 if x > right else 0) | (1 if y < 0x0C else 0) | (2 if y > 0x90 else 0)
        self.m.setb(s + 0xCB, self.m.b(s + 0xCB) | bits)

    def frame(self, bits):
        for bit, entry, x, y in ((1, 0, 0, 0), (2, 2, 0, 0xA8), (4, 3, 0, 0), (8, 1, 0x90, 0)):
            if bits & bit:
                self.ops.append(('rand', entry, self.x0 + x, y, 0x40))

    def ground(self):
        """T2112:000B and the frame (fight_step's first two calls)"""
        m = self.m
        ka, kb = m.b(self.ga + 5), m.b(self.gb + 5)
        for kind, dy in ((kb, 0), (ka, 0x5A)):
            if kind in PIECES:
                p = m.lin(GROUNDS, PIECES[kind])
                for i in range(m.b(p)):
                    entry, x, y = m.m[p + 1 + 3 * i:p + 4 + 3 * i]
                    self.ops.append(('fight', entry, self.x0 + x, y + dy, 0x5F))
        if m.w(self.rec + 0x22) == 0:
            self.srand(m.w(self.rec))
            for x in range(0, 0x98, 2):
                h = self.random(8, 0x0E)
                y = 0x5A - (h >> 1)
                self.ops.append(('column', self.x0 + x, y, y + h))
                self.ops.append(('column', self.x0 + x + 1, y, y + h))
        else:
            for cond, entry, y in ((kb not in (2, 5) and ka in (2, 5), 3, 0x58),
                                   (ka not in (2, 5) and kb in (2, 5), 4, 0x50),
                                   (kb == 7 and ka != 7, 7, 0x50), (ka == 7 and kb != 7, 8, 0x58)):
                if cond:
                    self.ops.append(('fight', entry, self.x0, y, 0x5F))
                    self.ops.append(('fight', entry, self.x0 + 0x4C, y, 0x5F))
        for bit in (2, 1, 4, 8):
            self.frame(bit)

    def script(self, unit, ground):
        """T223C:0001"""
        m = self.m
        if m.w(unit + 6) & 0x24:
            return 0x128
        if m.w(unit + 4) & 0x10:
            return 0x0E
        if m.w(unit + 4) & 8:
            return 0x40
        return SCRIPT.get(m.b(ground + 5), 0x0E)

    def place_units(self):
        """T1F5A:0001"""
        m, s = self.m, self.s
        for count in (s + 0xCC, s + 0x1D9):
            m.setb(count, min(max(m.b(count), 1), 6))
        for count, units, unit, ground in ((s + 0xCC, UNITS_A, self.ua, self.ga), (s + 0x1D9, UNITS_B, self.ub, self.gb)):
            off = self.script(unit, ground)
            for i in range(m.b(count)):
                while m.b(m.lin(SCRIPTS, off)) != 0xFE:
                    off += 1
                r = s + units + UNIT_REC * i
                m.setw(r + 2, off)
                m.setw(r + 4, m.load + SCRIPTS)
                off += 1
                m.setw(r + 6, 0)
                m.setb(r + 1, 0)

    def draw_unit(self, r, side):
        """T1F5A:0322"""
        m, s = self.m, self.s
        if m.b(r + 1):
            return
        two = m.b((self.tb if side else self.ta) + 0x0C) & 0x40
        if self.x0:
            xbase = 0x120 if side else 0xA0
            m.setw(s + 0xC8, 0x30)
            m.setw(s + 0x1D6, 0x20)
        else:
            xbase = 0x80 if side else 0
            m.setw(s + 0xC8, 0x20)
            m.setw(s + 0x1D6, 0x30)
        unit = self.ub if side else self.ua
        d = m.b(r)
        if side:
            d += 3
            if d > 5:
                d -= 6
        e1 = m.b(unit + 8) * 6 + d
        if side:
            x, y = s16(xbase - m.w(r + 8)), m.sw(r + 0x0A)
        else:
            x, y = s16(xbase + m.w(r + 8)), s16(0x9C - m.w(r + 0x0A))
        base = m.w(s + 0x1D6) if side else m.w(s + 0xC8)
        if two:
            e2 = e1 + 6
            dx, dy = BESIDE.get(m.b(r), (0, 0))
            if side:
                dx, dy = -dx, -dy
            if m.b(r) in (2, 3, 4):
                e1, e2 = e2, e1
            self.touch(x + dx, y + dy)
            self.ops.append(('unit', e2, x + dx, y + dy, base))
        if m.w(unit + 4) & 0x10:
            self.touch(x - 3, y - 2)
            self.ops.append(('unit', e1, x - 3, y + 2, 0x50))
        self.touch(x, y)
        self.ops.append(('unit', e1, x, y, base))
        m.setw(r + 0x0C, x)
        m.setw(r + 0x0E, y)

    def step_unit(self, r, side):
        """T1F5A:0736 -> 1 at the script's end"""
        m = self.m
        if m.w(r + 6) == 0:
            p = m.far(r + 2)[0]
            c = m.b(p)
            if c == 0xFF:
                self.draw_unit(r, side)
                return 1
            if c == 0xFE:
                m.setw(r + 8, m.b(p + 1))
                m.setw(r + 0x0A, m.b(p + 2))
                m.setb(r, m.b(p + 3))
                m.setw(r + 2, m.w(r + 2) + 4)
            else:
                m.setb(r, c)
                m.setw(r + 6, m.b(p + 1))
                m.setw(r + 2, m.w(r + 2) + 2)
        else:
            d = m.b(self.s + 0x0E + m.b((self.ub if side else self.ua) + 8))
            if m.sw(r + 6) < d:
                d = m.b(r + 6)
            sx, sy = STEP.get(m.b(r), (0, 0))
            m.setw(r + 8, m.w(r + 8) + sx * d)
            m.setw(r + 0x0A, m.w(r + 0x0A) + sy * d)
            m.setw(r + 6, m.w(r + 6) - d)
        self.draw_unit(r, side)
        return 0

    def units_pass(self):
        """T1F5A:093C -> 1 when all units have arrived"""
        m, s = self.m, self.s
        self.phase = 'the units come in'
        m.setl(s + 0x5F, m.l(s + 0x5F) + 1)
        m.setb(s + 0xCB, 0)
        if m.l(s + 0x5F) == 1:
            self.place_units()
        na = sum(self.step_unit(s + UNITS_A + UNIT_REC * i, 0) for i in range(m.b(s + 0xCC)))
        nb = sum(self.step_unit(s + UNITS_B + UNIT_REC * i, 1) for i in range(m.b(s + 0x1D9)))
        self.frame(m.b(s + 0xCB))
        if na == m.b(s + 0xCC) and nb == m.b(s + 0x1D9):
            m.setl(s + 0x5F, 0)
            return 1
        return 0

    def way(self, x1, y1, x2, y2):
        """T1F5A:01E4: which of a shot's eight pictures"""
        dx, dy = x2 - x1, y1 - y2
        ax, ay = abs(dx), abs(dy)
        if dx == 0:
            return 0 if dy >= 0 else 4
        if dy == 0:
            return 2 if dx >= 0 else 6
        if dx > 0 and dy > 0:
            return 0 if 2 * ax < ay else 2 if 2 * ay < ax else 1
        if dx > 0:
            return 4 if 2 * ax < ay else 2 if 2 * ay < ax else 3
        if dy > 0:
            return 0 if 2 * ax < ay else 6 if 2 * ay < ax else 7
        return 4 if 2 * ax < ay else 6 if 2 * ay < ax else 5

    def make_shots(self, mine):
        """T1F5A:0C58 (A's, mine 0) and T1F5A:101C (B's, mine 1) but for
        whether B answers"""
        m, s = self.m, self.s
        if mine:
            number, shots, own, other = s + 0x1D8, s + SHOTS_B, s + UNITS_B, s + UNITS_A
            own_count, other_count = m.b(s + 0x1D9), m.b(s + 0xCC)
            loss = (m.b(s + 0xCC) - m.b(s + 0x1DA)) & 0xFF
            flags, firing = 1, self.ub
        else:
            number, shots, own, other = s + 0xCA, s + SHOTS_A, s + UNITS_A, s + UNITS_B
            own_count, other_count = m.b(s + 0xCC), m.b(s + 0x1D9)
            loss = (m.b(s + 0x1D9) - m.b(s + 0x67)) & 0xFF
            flags, firing = 5, self.ua
        n = own_count
        m.setb(number, n)
        entries = m.far(self.rec + 0x3A)[0]
        j, k = 0, (n - 1) & 0xFF
        while j < n:
            r = shots + SHOT_REC * j
            t = k % other_count if n > other_count else k
            tx, ty = m.sw(other + UNIT_REC * t + 0x0C) + 0x0C, m.sw(other + UNIT_REC * t + 0x0E) + 0x0C
            m.setb(r + 0x1A, t)
            # B's routine takes j here where A's takes k; neither comes up
            # (k is below the own count)
            f = j if k < own_count else (j if mine else k) % own_count
            sx, sy = m.sw(own + UNIT_REC * f + 0x0C) + 6, m.sw(own + UNIT_REC * f + 0x0E)
            if k >= loss:
                ty += self.random(-0x20, 0x20)
            dx, dy = tx - sx, ty - sy
            m.setw(r + 0x0A, (dx > 0) - (dx < 0))
            m.setw(r + 0x0C, (dy > 0) - (dy < 0))
            entry = 6 + 8 * m.b(s + 0x29 + 2 * m.b(firing + 8)) + self.way(sx, sy, tx, ty)
            m.m[r + 5:r + 9] = m.m[entries + 4 * entry:entries + 4 * entry + 4]
            m.setw(r + 0x16, max(abs(dx), abs(dy)))
            m.setw(r + 0x18, 0)
            m.setw(r, sx)
            m.setw(r + 2, sy)
            m.setw(r + 0x12, 0)
            m.setw(r + 0x14, 0)
            m.setb(r + 4, flags | (0x10 if k < loss else 0))
            m.setb(r + 9, 5)
            m.setw(r + 0x0E, abs(dx))
            m.setw(r + 0x10, abs(dy))
            m.setb(r + 0x1B, self.random(0, 0x2710) & 0x0C)
            j += 1
            k = (k - 1) & 0xFF
        if loss > n:
            for i in range(n, loss):
                r = shots + SHOT_REC * i
                m.setb(r + 0x1A, i)
                m.setb(r + 4, flags | 0x30)
                m.setw(r + 0x18, 0)
                m.setb(r + 9, 5)
                m.setw(r + 0x16, m.w(shots + SHOT_REC * (i % n) + 0x16))
                m.setw(r, m.w(other + UNIT_REC * i + 0x0C))
                m.setw(r + 2, m.w(other + UNIT_REC * i + 0x0E))
                m.setb(r + 0x1B, 0)
            m.setb(number, loss)

    def answers(self):
        """the beginning of T1F5A:101C: F2D37:0284, 1 when B has no shots"""
        m = self.m
        targets = m.w(self.tb + 5)
        near = m.w(self.rec + 0x22) != 0
        far = max(abs(m.sw(self.rec + 0x1E) - m.sw(self.rec + 0x1A)), abs(m.sw(self.rec + 0x20) - m.sw(self.rec + 0x1C)))
        reach = m.b(self.tb + (7 if m.w(self.ua + 4) & 0x10 else 8)) - 1
        silent = bool(targets & 0x80) or not near or not m.w(self.ta + 0x0C) & targets & 0x3C \
            or bool(targets & 0x40) or far > reach
        m.setb(self.s + 0x284, 1 if silent else 0)
        return not silent

    def step_shot(self, r):
        """T1F5A:1548 -> 1 when the shot is over"""
        m, s = self.m, self.s
        flags = m.b(r + 4)
        if flags & 8:
            return 1
        if m.b(r + 0x1B):
            m.setb(r + 0x1B, m.b(r + 0x1B) - 1)
            return 0
        if not flags & 0x60:
            flags |= 0x40                # its sound begins here
            m.setb(r + 4, flags)
        if flags & 1:
            if not flags & 0x20:
                entries = m.far(self.rec + 0x3A)[0]
                ptr = bytes(m.m[r + 5:r + 9])
                entry = next(i for i in range(256) if m.m[entries + 4 * i:entries + 4 * i + 4] == ptr)
                self.ops.append(('bum', entry, m.sw(r), m.sw(r + 2), 0))
                self.touch(m.sw(r), m.sw(r + 2))
            pace = m.b(s + 0x2A + 2 * m.b((self.ua if flags & 4 else self.ub) + 8))
            i = 0
            while i < pace and m.sw(r + 0x18) < m.sw(r + 0x16):
                if not flags & 0x20:
                    m.setw(r + 0x12, m.w(r + 0x12) + m.w(r + 0x0E))
                    m.setw(r + 0x14, m.w(r + 0x14) + m.w(r + 0x10))
                    if m.sw(r + 0x12) > m.sw(r + 0x16):
                        m.setw(r + 0x12, m.w(r + 0x12) - m.w(r + 0x16))
                        m.setw(r, m.w(r) + m.w(r + 0x0A))
                    if m.sw(r + 0x14) > m.sw(r + 0x16):
                        m.setw(r + 0x14, m.w(r + 0x14) - m.w(r + 0x16))
                        m.setw(r + 2, m.w(r + 2) + m.w(r + 0x0C))
                m.setw(r + 0x18, m.w(r + 0x18) + 1)
                i += 1
            if m.w(r + 0x18) == m.w(r + 0x16):
                flags = flags & 0xFE | 2
                m.setb(r + 4, flags)
                if flags & 0x10:
                    m.setb(s + (UNITS_B if flags & 4 else UNITS_A) + UNIT_REC * m.b(r + 0x1A) + 1, 1)
            return 0
        if flags & 2:
            step = m.b(r + 9)
            if step < 0x80:
                x, y = s16(m.w(r) - 12), s16(m.w(r + 2) - 12)
                self.ops.append(('bum', step, x, y, 0x30 if flags & 0x10 else 0x20))
                self.touch(x, y)
                m.setb(r + 9, step - 1)
            else:
                m.setb(r + 4, flags | 8)
        return 0

    def shots_pass(self):
        """T1F5A:1A21 -> 1 when all shots are over"""
        m, s = self.m, self.s
        self.phase = 'the shots'
        m.setl(s + 0x63, m.l(s + 0x63) + 1)
        m.setb(s + 0xCB, 0)
        if m.l(s + 0x63) == 1:
            self.make_shots(0)
            m.setb(s + 0x1D8, m.b(s + 0x1D9))
            if self.answers():
                self.make_shots(1)
        for i in range(m.b(s + 0x1D9)):
            self.draw_unit(s + UNITS_B + UNIT_REC * i, 1)
        for i in range(m.b(s + 0xCC)):
            self.draw_unit(s + UNITS_A + UNIT_REC * i, 0)
        na = sum(self.step_shot(s + SHOTS_A + SHOT_REC * i) for i in range(m.b(s + 0xCA)))
        nb = m.b(s + 0x1D8)
        if not m.b(s + 0x284):
            nb = sum(self.step_shot(s + SHOTS_B + SHOT_REC * i) for i in range(m.b(s + 0x1D8)))
        self.frame(m.b(s + 0xCB))
        if na == m.b(s + 0xCA) and nb == m.b(s + 0x1D8):
            m.setl(s + 0x63, 0)
            return 1
        return 0

    def reckon(self, say):
        """fight_reckon by fight.py: the counts into the units, and rand's
        long after its srand and two numbers"""
        m = self.m
        seed = ((m.far(self.rec)[1] + m.far(self.rec + 4)[1]) ^ m.l(self.rec + 0x2E)) & 0xFFFF
        for _, u, off, n in fight.reckon(m, say):
            m.setb(u + off, n)
            if off == 3 and n == 0:
                m.setb(u + 2, 0)
        seed = fight.rand(fight.rand(seed)[0])[0]
        m.setl(m.lin(DATA, SEED), seed)

    def fight_step(self, say=lambda s: None):
        """T1F3C:000A -> 1 when the scene is over"""
        m, s, rec = self.m, self.s, self.rec
        m.setw(s + 0x0A, RECORD)        # F2D37:000A, the far pointer to the record
        m.setw(s + 0x0C, m.load + FAR)
        calls = m.l(rec + 0x26) + 1
        m.setl(rec + 0x26, calls)
        self.calls = calls
        if calls <= 2:
            if calls == 1:
                m.setb(s + 8, 0)
                m.setb(s + 9, 0)
                m.setb(s + 0xCC, m.b(self.ua + 2))
                m.setb(s + 0x1D9, m.b(self.ub + 2))
                self.reckon(say)
                m.setb(s + 0x1DA, m.b(self.ua + 2))
                m.setb(s + 0x67, m.b(self.ub + 2))
            self.ground()
        if not m.b(s + 8):
            m.setb(s + 8, self.units_pass())
            return 0
        if not m.b(s + 9):
            m.setb(s + 9, self.shots_pass())
            return 0
        self.phase = 'the end'
        m.setl(rec + 0x26, 0)
        return 1


class Libraries:
    def __init__(self, game):
        lib = screens.find(game, 'LIB')
        self.lists = {name: [e for _, e in libfiles.read(screens.unpacked(screens.find(lib, name.upper() + '.LIB')))]
                      for name in ('fight', 'bum', 'rand')}
        self.lists['unit'] = mapfiles.sorted_entries(game, 'UNIT')

    def check(self, mem, rec, ops):
        """whether the run's memory has the files' entries where the fight
        record's lists point"""
        bad = set()
        for name, at in (('unit', 0x32), ('bum', 0x3A), ('rand', 0x3E), ('fight', 0x42)):
            entries = mem.far(rec + at)[0]
            for n in sorted({op[1] for op in ops if op[0] == name}):
                e = self.lists[name][n]
                raw = libfiles.entry_bytes(dict(e, tail=b'', packed=None))
                a = mem.far(entries + 4 * n)[0]
                if bytes(mem.m[a:a + len(raw)]) != raw:
                    bad.add((name, n))
        return bad


def picture(files, libs, ops, clip, behind=None):
    """the ops drawn in their order; behind: those of the ground, drawn
    before them"""
    s = screens.Screen(files, clip)
    for op in (behind or []) + ops:
        if op[0] == 'column':
            s.fill(op[1], op[2], op[1], op[3], 0)
        else:
            name, n, x, y, base = op
            s.entry(x, y, libs.lists[name][n], base)
    return s.pix


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('entry')
    ap.add_argument('--exit', help="the run's memory at the next entry of fight_step")
    ap.add_argument('--vram', help="the run's video memory at the next entry")
    ap.add_argument('--png', metavar='DIR')
    ap.add_argument('--game')
    ap.add_argument('--load', default='0077')
    ap.add_argument('-v', action='store_true', help='what the pass drew')
    a = ap.parse_args()
    load = int(a.load, 16)
    game = a.game or os.path.join(game_dir(), 'ISLE')
    data = open(a.entry, 'rb').read()
    sc = Scene(Mem(data, load))
    m = sc.m
    result = sc.fight_step(print if a.v else (lambda s: None))
    print('call %d: %s, returns %d; A unit %02X type %d, %d units, B unit %02X type %d, %d units; %d drawn' % (
        sc.calls, sc.phase, result, m.b(sc.rec + 8), m.b(sc.ua + 8), m.b(sc.s + 0xCC),
        m.b(sc.rec + 9), m.b(sc.ub + 8), m.b(sc.s + 0x1D9), len(sc.ops)))
    if a.v:
        for op in sc.ops:
            print('  ' + ('column x %d, y %d..%d' % op[1:] if op[0] == 'column' else '%s %d at %d, %d base %02X' % op))
    ok = True
    libs = Libraries(game)
    bad = libs.check(m, sc.rec, sc.ops)
    if bad:
        ok = False
        print("not the files' entries in the run's memory: %s" % ', '.join('%s %d' % b for b in sorted(bad)))
    if a.exit:
        out = Mem(open(a.exit, 'rb').read(), load)
        regions = [("the scene's values", sc.s + 8, 0x285 - 8), ('the count of calls', sc.rec + 0x26, 4)]
        if not result:                  # after the last call change_phase goes on with the units
            regions += [("A's record", sc.ua, 0x1A), ("B's record", sc.ub, 0x1A), ("rand's long", m.lin(DATA, SEED), 4)]
        for name, at, n in regions:
            diff = [i for i in range(n) if m.m[at + i] != out.m[at + i]]
            if diff:
                ok = False
                print('%s: %d bytes differ, the first at +%X (%02X, the game %02X)' % (
                    name, len(diff), diff[0] + (8 if at == sc.s + 8 else 0), m.m[at + diff[0]], out.m[at + diff[0]]))
        if ok:
            print('the memory after the call: as the game (%d bytes)' % sum(n for _, _, n in regions))
    if a.vram or a.png:
        files = screens.Files(game)
        ram = screens.Ram(data, load)
        behind = []
        if sc.calls > 2:
            ground = Scene(Mem(data, load))
            ground.ground()
            behind = ground.ops
        pix = picture(files, libs, sc.ops, ram.clip(), behind)
        if a.vram:
            if result:
                print('no page is flipped after the last call: nothing to compare')
            else:
                shown = 1 - screens.Ram(open(a.exit, 'rb').read(), load).drawn_page() if a.exit else None
                vram = open(a.vram, 'rb').read()
                for page in (0, 1):
                    n, same, box = screens.compare(pix, vram, page)
                    print('  page %d%s: %d pixels drawn, %d as in the video memory%s' % (
                        page, ' (shown)' if page == shown else '', n, same,
                        '' if box is None else ', the others within x %d..%d, y %d..%d' % (box[0], box[2], box[1], box[3])))
                    if page == shown and same != n:
                        ok = False
        if a.png:
            os.makedirs(a.png, exist_ok=True)
            screens.to_png(os.path.join(a.png, 'scene.png'), game, pix)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
