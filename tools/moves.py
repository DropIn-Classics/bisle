#!/usr/bin/env python3
"""Where a unit of Battle Isle can move: BATTLE.EXE's reckoning of the
squares in reach (T0BA0:0323, `reach`) done again from a run's memory
and compared with what the game left there.

    moves.py RAM [--load SEG] [--grid]

RAM is a run's memory (run.py -ram) after the game reckoned a unit's
reach (a unit chosen to be moved, or taken out of a building).  The
routine keeps its arguments at F273A:0006, and the tool takes them from
there: the unit's square, the far pointer to the buffer, the unit, its
points, the side, the ground flags, the far pointer to the map's squares.
It fills a buffer of its own from the map and the units in the memory
and says how many of the buffer's bytes and of the squares' marks
(F27EE:1339) are the game's.  --grid prints the tool's buffer (the points
left on each square in reach, `.` out of reach).  Nothing of the game's
files is read.  --load is the segment the program was loaded at (0077).

The buffer and the marks are 68 rows of 64 squares (1104h bytes), a
square at column + 64 * row, whatever the map's width.

The costs (T0BA0:098F, `cost_map`), a negative byte a square:

  the ground: minus the ground record's +4 for a unit with 10h in its +4
    (an air unit), else minus its +3 where the type's +4 AND the ground's
    +2 is not 0 and B0h (-80: never entered) where it is 0; for a unit
    with 10h in its +6 FEh (-2) in place of the +3.  B0h as well where
    the ground's flags (+0) have one of the bits asked for (the callers
    give FFFFh: none, for a unit with 1 in its +6; else 6 for a unit of
    player 1 and 3 for one of player 0);
  then, in the map's order, the squares that hold a unit: of neither
    player (2 in its +4) B0h; of the other side B0h, and the six squares
    around it that are not B0h get 9Ah (the other side being player 1) or
    9Bh; of the own side FEh if it has 1000h or 2000h in its +4, else the
    ground's cost stays.  For a mover of two squares (40h in its type's
    +0Ch) every unit's square but its own is B0h.  Last, a square whose
    unit has 4 in its +6 when the mover has 2000h there, or 20h when the
    mover has 1000h, gets the ground's cost again (for an air unit even
    where the masks say no): a unit the mover can go into, presumably.

The reach: the unit's square gets its points (the unit's +0 from the
player's keys).  Then for p from the points down to 1, every square that
has p left gives each of the six around it that is not reached yet: 0 if
it is 9Ah (the mover being of player 0) or 9Bh (of player 1): a square
beside an enemy is entered and not left; else p plus its cost, if that
is not below 0.  The squares looked at for a p are those of a box around
the unit, points / 2 to the left and up and `points` wide and high (so
one less to the right and below).  Every square with 0 or more left gets
the side's mark (1, or 2 for player 1) and is counted.

So a type's move (UNIT.DAT's +0) is points, a square costs its ground's
+3 (or +4), and the number the unit's screen shows is half of it.

Checked against two runs (docs/HANDOFF.md has the keys): a unit taken
out of a depot on ISLE's map 03 (5 points, 9 squares) and a unit chosen
to move on the first map (14 points, 39 squares): all 1104h bytes of the
buffer and all marks as the game's.

    moves.py RAM --path SS:BP

The path the game found (find_path, T0BA0:0E2E) done again.  RAM is the
memory of a run stopped at the routine's end (`-break LT0BA0_138A#N`),
SS:BP the two registers of the runner's report there: the routine's
arguments are in its stack frame (the list, the unit, the squares it
goes from and to, the side, the map's squares), the path it wrote 1F40h
bytes into the list and its length at F27EE:0B62.

find_path goes over the squares marked in reach, best first by a key and
without ever going back to a better node behind it: a list of nodes in
the key's order, the start first; the routine takes node after node
along the list, and each neighbour (in the order up, right up, right
down, down, left down, left up) that is in reach and not yet a node
becomes one, put in after the current node, before the first node there
with a greater key.  The key is the distance to the aim in squares
(square_distance, T0E9B:1E90) times the type's +14h plus the ground's
cost times the type's +15h, 0 on the aim itself.  When the node taken is
the aim, the path is the chain of the nodes each was reached from.  The
start is not marked as taken, so it can become a node again.  Checked
against six calls in a battle against the computer (the computer's
units, side 1; paths of 2 to 14 squares): the same squares in each.

    moves.py RAM --fire ENTRY SS:SP

What a unit can fire at (fire_reach, T0BA0:05C8) done again.  ENTRY is
the memory of a run stopped at the routine (`-break fire_reach#N`) and
SS:SP the registers there (its arguments are on the stack: the buffer,
the square, the range, the side, the unit, the targets word, the map's
squares), RAM the same run's memory at the routine's end (`-break
LT0BA0_0988#N`).  The squares less than `range` squares from the unit's
are taken (the range spreads as the points of a move do, each square
costing 1, from range - 1 on the unit's square), but for the six around
the unit when the targets word has 40h; of those the squares with a unit
of the other side (not of neither player) whose class (+4 AND 3Ch) the
targets name, or which has 2 in its +6 when the targets have 2, get 1 in
the buffer and the mark 4 (8 for side 1).  The callers give the type's
range for air units (+7) with the targets AND FFD3h and the other range
(+8) with the targets AND FFEFh (T0D36:1275, 12F5).  Checked against six
calls in a battle against the computer (range 2, targets 2Ch, one or two
targets each): every byte of the buffer and every mark as the game's.
Not seen: a range above 2, a targets word with 40h.
"""
import argparse
import struct
import sys

FAR = 0x27EE
ARGS = (0x273A, 0x0006)                 # reach's arguments as it keeps them
UNITS = 0x2898
TYPES = 0x001F
GROUND = 0x0751
WIDTH = 0x246E
MARKS = 0x1339
SIZE = 0x1104


def s8(v):
    return v - 256 if v & 0x80 else v


def neighbours64(x, y, w, h):
    """T0BA0:0053 with T0BA0:020A: the six squares around (x, y) in the
    buffer, None for one off the map"""
    i = x + 64 * y
    n = [i - 64, 0, 0, i + 64, 0, 0]
    if x & 1:
        n[1], n[5], n[4], n[2] = i + 1, i - 1, i + 63, i + 65
        if y <= 0:
            n[0] = None
        elif h - 1 <= y:
            n[3] = n[2] = n[4] = None
        if x <= 0:
            n[5] = n[4] = None
        elif w - 1 <= x:
            n[1] = n[2] = None
    else:
        n[2], n[4], n[5], n[1] = i + 1, i - 1, i - 65, i - 63
        if y <= 0:
            n[0] = n[5] = n[1] = None
        elif h - 1 <= y:
            n[3] = None
        if x <= 0:
            n[4] = n[5] = None
        elif w - 1 <= x:
            n[1] = n[2] = None
    return [v for v in n if v is not None and v >= 0]


class Ram:
    def __init__(self, path, load):
        with open(path, 'rb') as f:
            self.m = f.read()
        self.load = load

    def lin(self, frame, off):
        return (self.load + frame) * 16 + off

    def b(self, a):
        return self.m[a]

    def w(self, a):
        return struct.unpack_from('<H', self.m, a)[0]

    def far(self, a):
        off, seg = struct.unpack_from('<HH', self.m, a)
        return seg * 16 + off

    def unit(self, n):
        return self.lin(FAR, UNITS) + 0x1A * n

    def type_of(self, n):
        return self.lin(FAR, TYPES) + 0x44 * self.b(self.unit(n) + 8)

    def size(self):
        a = self.lin(FAR, WIDTH)
        return self.w(a), self.w(a + 2)


def cost_map(ram, unit, side, flags, squares):
    """T0BA0:098F"""
    w, h = ram.size()
    me, t = ram.unit(unit), ram.type_of(unit)
    mask, cls = ram.b(t + 4), ram.w(t + 0x0C)
    mode = 0 if ram.w(me + 4) & 0x10 else 2
    if ram.w(me + 6) & 0x10:
        mode = 1

    def ground(x, y, masked=True):
        g = ram.lin(FAR, GROUND) + 6 * ram.b(squares + 2 * (x + w * y))
        fits = ram.b(g + 2) & mask
        if mode == 0:
            return -ram.b(g + 4) & 0xFF if fits or not masked else 0xB0
        if mode == 1:
            return 0xFE if fits else 0xB0
        return -ram.b(g + 3) & 0xFF if fits else 0xB0

    buf = bytearray(b'\xFF' * SIZE)
    for y in range(h):
        for x in range(w):
            g = ram.lin(FAR, GROUND) + 6 * ram.b(squares + 2 * (x + w * y))
            c = ground(x, y)
            if flags != 0xFFFF and ram.w(g) & flags:
                c = 0xB0
            buf[x + 64 * y] = c
    for y in range(h):
        for x in range(w):
            u = ram.b(squares + 2 * (x + w * y) + 1)
            if u > 0xF0:
                continue
            i = x + 64 * y
            c4 = ram.w(ram.unit(u) + 4)
            if c4 & 2:
                buf[i] = 0xB0
            elif bool(c4 & 1) != (side == 1) if c4 & 1 else side != 0:
                buf[i] = 0xB0
                for n in neighbours64(x, y, w, h):
                    if buf[n] != 0xB0:
                        buf[n] = 0x9A if c4 & 1 else 0x9B
            elif c4 & 0x3000:
                buf[i] = 0xFE
            if cls & 0x40 and u != unit:
                buf[i] = 0xB0
            u6, m6 = ram.w(ram.unit(u) + 6), ram.w(me + 6)
            if u6 & 4 and m6 & 0x2000 or u6 & 0x20 and m6 & 0x1000:
                buf[i] = ground(x, y, masked=False)
    return buf


def reach(ram, pos, unit, points, side, flags, squares):
    """T0BA0:0323 -> the buffer"""
    w, h = ram.size()
    buf = cost_map(ram, unit, side, flags, squares)
    x, y = (pos >> 1) % w, (pos >> 1) // w
    x0, y0 = max(0, x - (points >> 1)), max(0, y - (points >> 1))
    x1, y1 = min(w, x0 + points), min(h, y0 + points)
    stop = 0x9B if side else 0x9A
    buf[x + 64 * y] = points & 0xFF
    for p in range(s8(points & 0xFF), 0, -1):
        for yy in range(y0, y1):
            for xx in range(x0, x1):
                if s8(buf[xx + 64 * yy]) != p:
                    continue
                for n in neighbours64(xx, yy, w, h):
                    if s8(buf[n]) >= 0:
                        continue
                    if buf[n] == stop:
                        buf[n] = 0
                    elif s8(buf[n]) + p >= 0:
                        buf[n] = (buf[n] + p) & 0xFF
    return buf


def square_distance(x1, y1, x2, y2):
    """T0E9B:1E90"""
    if y1 > y2:
        x1, y1, x2, y2 = x2, y2, x1, y1
    if x2 >= x1:
        y1, y2 = y1 - x1 // 2, y2 - x2 // 2
    else:
        x1, x2 = x2, x1
        y1, y2 = y1 - (x1 + 1) // 2, y2 - (x2 + 1) // 2
    return x2 - x1 + max(0, y2 - y1)


def find_path(ram, unit, start, aim, side, squares):
    """T0BA0:0E2E -> the squares' offsets from the aim back to the start,
    or None"""
    w, h = ram.size()
    t = ram.type_of(unit)
    w_dist, w_ground = s8(ram.b(t + 0x14)), s8(ram.b(t + 0x15))
    air = ram.w(ram.unit(unit) + 4) & 0x10
    marks = ram.lin(FAR, MARKS)
    bit = 2 if side else 1
    ax, ay = (aim >> 1) % w, (aim >> 1) // w
    # a node: [column, row, from, key, before, after]
    nodes = {0: [0, 0, 0, 0, 0, 1], 1: [(start >> 1) % w, (start >> 1) // w, 1, 0, 0, 2],
             2: [0, 0, 0, 0x7530, 1, 0]}
    taken, free, cur = set(), 3, 0
    while True:
        cur = nodes[cur][5]
        if cur == 0:
            return None
        x, y = nodes[cur][0], nodes[cur][1]
        if (x, y) == (ax, ay):
            path = [2 * (ax + w * ay)]
            while cur > 1:
                cur = nodes[cur][2]
                path.append(2 * (nodes[cur][0] + w * nodes[cur][1]))
                if len(path) > 0x316:
                    return None
            return path
        for n in neighbours64(x, y, w, h):
            if not ram.b(marks + n) & bit or n in taken:
                continue
            nx, ny = n % 64, n // 64
            d = square_distance(nx, ny, ax, ay)
            g = ram.lin(FAR, GROUND) + 6 * ram.b(squares + 2 * (nx + w * ny))
            key = d * w_dist + s8(ram.b(g + (4 if air else 3))) * w_ground if d else 0
            at = nodes[cur][5]
            while nodes[at][3] <= key:
                at = nodes[at][5]
            nodes[free] = [nx, ny, cur, key, nodes[at][4], at]
            nodes[nodes[at][4]][5] = free
            nodes[at][4] = free
            taken.add(n)
            free += 1
            if free > 0x316:
                return None


def path_main(ram, frame):
    """find_path's arguments from its stack frame, the path the game wrote"""
    ss, bp = (int(v, 16) for v in frame.split(':'))
    f = ss * 16 + bp
    theirs, unit, start, aim, side = ram.far(f + 6), ram.w(f + 0x0A), ram.w(f + 0x0C), ram.w(f + 0x0E), ram.w(f + 0x10)
    squares = ram.far(f + 0x12)
    w, h = ram.size()
    n = ram.w(ram.lin(FAR, 0x0B62))
    game = [ram.w(theirs + 0x1F40 + 2 * i) for i in range(n)]
    path = find_path(ram, unit & 0xFF, start, aim, side, squares)

    def show(p):
        return ' '.join('%d,%d' % ((v >> 1) % w, (v >> 1) // w) for v in p) if p else 'none'
    print('unit %02X (type %d), side %d: from %s to %s' % (
        unit & 0xFF, ram.b(ram.unit(unit & 0xFF) + 8), side, show([start]), show([aim])))
    print('the path:', show(path))
    if (path or []) == game:
        print('as the game\'s (%d squares)' % n)
        return 0
    print('the game\'s:', show(game))
    return 1


def fire_reach(ram, pos, rng, side, unit, targets, squares):
    """T0BA0:05C8 -> the buffer: 1 on a target's square, FFh elsewhere"""
    w, h = ram.size()
    buf = bytearray(b'\xFF' * SIZE)
    x, y = (pos >> 1) % w, (pos >> 1) // w
    x0, y0 = max(0, x - rng), max(0, y - rng)
    x1, y1 = min(w, x0 + 2 * rng + 1), min(h, y0 + 2 * rng + 1)
    buf[x + 64 * y] = (0xFF + rng) & 0xFF
    for p in range(s8(buf[x + 64 * y]), 0, -1):
        for yy in range(y0, y1):
            for xx in range(x0, x1):
                if s8(buf[xx + 64 * yy]) != p:
                    continue
                for n in neighbours64(xx, yy, w, h):
                    if s8(buf[n]) < 0 and s8(buf[n]) + p >= 0:
                        buf[n] = (buf[n] + p) & 0xFF
    if targets & 0x40:
        at = (ram.w(ram.unit(unit) + 0x0B + 2 * side) - 1) >> 1
        for n in neighbours64(at % w, at // w, w, h):
            buf[n] = 0xFF
    for y in range(h):
        for x in range(w):
            i = x + 64 * y
            if s8(buf[i]) < 0:
                continue
            buf[i] = 0xFF
            u = ram.b(squares + 2 * (x + w * y) + 1)
            if u > 0xF0:
                continue
            c4, u6 = ram.w(ram.unit(u) + 4), ram.w(ram.unit(u) + 6)
            if c4 & 2 or not (side != 1 if c4 & 1 else side != 0):
                continue
            if targets & 0x3C & c4 or u6 & 2 and targets & 2:
                buf[i] = 1
    return buf


def fire_main(ram, entry, frame):
    """fire_reach's arguments from the stack at its entry (the memory
    `entry`), its buffer and marks from the memory at its end"""
    ss, sp = (int(v, 16) for v in frame.split(':'))
    f = ss * 16 + sp - 2
    theirs, pos, rng, side = entry.far(f + 6), entry.w(f + 0x0A), entry.w(f + 0x0C), entry.w(f + 0x0E)
    unit, targets, squares = entry.w(f + 0x10) & 0xFF, entry.w(f + 0x12), entry.far(f + 0x14)
    w, h = entry.size()
    buf = fire_reach(entry, pos, rng, side, unit, targets, squares)
    print('unit %02X (type %d) at column %d row %d, range %d, side %d, targets %04X' % (
        unit, entry.b(entry.unit(unit) + 8), (pos >> 1) % w, (pos >> 1) // w, rng, side, targets))
    game = ram.m[theirs:theirs + SIZE]
    same = sum(1 for i in range(SIZE) if buf[i] == game[i])
    bit = 8 if side & 1 else 4
    marks = ram.lin(FAR, MARKS)
    mine = [s8(buf[i]) > 0 for i in range(SIZE)]
    got = [bool(ram.b(marks + i) & bit) for i in range(SIZE)]
    print('targets: %s' % (' '.join('%d,%d' % (i % 64, i // 64) for i in range(SIZE) if mine[i]) or 'none'))
    print('the buffer: %d of %d bytes as the game\'s; the marks: %d of %d' % (
        same, SIZE, sum(1 for i in range(SIZE) if mine[i] == got[i]), SIZE))
    return 0 if same == SIZE and mine == got else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('ram')
    ap.add_argument('--load', default='0077')
    ap.add_argument('--grid', action='store_true')
    ap.add_argument('--path', metavar='SS:BP')
    ap.add_argument('--fire', nargs=2, metavar=('ENTRY', 'SS:SP'))
    a = ap.parse_args()
    ram = Ram(a.ram, int(a.load, 16))
    if a.path:
        return path_main(ram, a.path)
    if a.fire:
        return fire_main(ram, Ram(a.fire[0], int(a.load, 16)), a.fire[1])
    r = ram.lin(*ARGS)
    pos, unit, points, side, flags = ram.w(r), ram.w(r + 6), ram.w(r + 8), ram.w(r + 0x0A), ram.w(r + 0x0C)
    theirs, squares = ram.far(r + 2), ram.far(r + 0x0E)
    w, h = ram.size()
    print('unit %02X (type %d) at column %d row %d, %d points, side %d, ground flags %04X, map %d by %d' % (
        unit & 0xFF, ram.b(ram.unit(unit & 0xFF) + 8), (pos >> 1) % w, (pos >> 1) // w, points, side, flags, w, h))
    buf = reach(ram, pos, unit & 0xFF, points, side, flags, squares)
    if a.grid:
        for y in range(h):
            print(' '.join('%2d' % s8(buf[x + 64 * y]) if s8(buf[x + 64 * y]) >= 0 else ' .' for x in range(w)))
    game = ram.m[theirs:theirs + SIZE]
    same = sum(1 for i in range(SIZE) if buf[i] == game[i])
    print('the buffer: %d of %d bytes as the game\'s' % (same, SIZE))
    bit = 2 if side == 1 else 1
    marks = ram.lin(FAR, MARKS)
    mine = [s8(buf[i]) >= 0 for i in range(SIZE)]
    got = [bool(ram.b(marks + i) & bit) for i in range(SIZE)]
    print('in reach: %d squares, the game\'s marks %d, %d of %d the same' % (
        sum(mine), sum(got), sum(1 for i in range(SIZE) if mine[i] == got[i]), SIZE))
    return 0 if same == SIZE and mine == got else 1


if __name__ == '__main__':
    sys.exit(main())
