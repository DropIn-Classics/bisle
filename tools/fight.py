#!/usr/bin/env python3
"""Battle Isle's reckoning of a fight (BATTLE.EXE's fight_reckon,
T2190:000D), done again from a run's memory and compared with what the
game made of it.

    fight.py ENTRY [--exit RAM] [--load SEG] [-v]

ENTRY is a run's memory (run.py -ram) stopped at fight_reckon (`-break
fight_reckon#N`), --exit the same run's stopped at the routine's end
(`-break LT2190_0ABA#N`: the POP DS before its RETF).  The tool prints
the fight's values and the counts it reckons for both units; with --exit
it says whether the game's memory has them.  --load is the segment the
runner loaded the program at (default 0077).  Nothing of the game's
files is read: every value, the tables too, comes from the run's memory.

The fight record (F27EE:2716, the far pointer F2D37:000A; T0408:000B
fills it for each attack order when the phase changes):

    +00 far   the attacker's unit record (A)     +04 far  the target's (B)
    +08       A's number                          +09      B's
    +0A far   A's type (UNIT.DAT's record)        +0E far  B's
    +12 far   the ground record of A's square     +16 far  of B's
    +1A,+1C   A's column and row                  +1E,+20  B's
    +22       1: B's square is one of the six around A's
    +24       a percentage added to A's hit value (below)
    +25       a percentage added to B's armour
    +26 long  counts the calls of T1F3C:000A; the reckoning is done at 1
    +2E long  F27EE:251F, the count of the map loop's passes
    +5E far   srand (CODE:2627)                   +62 far  random(lo, hi)

fight_reckon, with n a side's count (the unit's +2, or +3 for a type
with flag 4 of +0Eh; 0 is taken as 1, more than 6 as 6), e its unit's +1
plus 1, arm its type's +1, hit its type's value against the other's class
(+9 for 10h, +0Bh for 8, else +0Ah), and att, def the two signed bytes
at F2D65:000E + 2 * the scene (+5) of the ground it stands on:

    hitA = hitA + hitA * pct24 / 100       (in 16.16, the whole part kept)
    armB = armB + armB * pct25 / 100
    f(n, v, e, g, d) = n * v + n * v * (e * 256 / 6 + 1) / d + g * (v / 2)
    P = f(nA, hitA, eA, attA, 256)         A's attack
    Q = f(nA, armA, eA, defA, 256)         A's defence
    R = f(nB, hitB, eB, attB, 256)         B's attack
    S = f(nB, armB, eB, defB, 512)         B's defence (512, not 256)
    cA = eA, but 6 when eA > 7             cB = eB, but 2 when eB < 2
                                           and 6 when eB > 6
    lossB = (P - S) / (armB * (8 - cA) / 2)
    lossA = (R - Q) / (armA * (8 - cB) / 2)
    lossB <= 0 and S / nB + armB / 2 < P: lossB = 1
    lossA <= 0 and Q / nA + armA / 2 < R: lossA = 1
    a loss of 7 .. 11 is 6;  lossB is at most 2 * nA, lossA 2 * nB
    srand(the low word of (A's far pointer + B's, as longs) XOR +2E)
    r1 = random(0, 200h), r2 = random(0, 200h)
    34h - 6 * eA > r1: lossB - 1;  1DBh - 14h * eA < r1: lossB + 1
    34h - 6 * eB > r2: lossA - 1;  1DBh - 14h * eB < r2: lossA + 1
    lossB within 0 .. nB, lossA within 0 .. nA
    B's count = nB - lossB;  A's count = nA - lossA unless B cannot answer

All divisions are of longs and cut off towards 0 (CODE:3D30); a divisor
of 0 is a divide error there (the tool says so).  random(lo, hi) is
rand() mod (hi - lo + 1) + lo (T0D36:05FF), rand() the long at DATA:1938
times 15A4E35h plus 1, of which bits 16 .. 30 (CODE:263F).  With a type
of flag 4 the count written is the unit's +3, and its +2 becomes 0 when
that is 0.

The unit's +1 is its experience, 0 .. 6: after the fight T0408:000B adds
one for a unit whose enemy lost something (unless the enemy has 20h in
its +6) and one more when the enemy's count is 0 (read; a unit's e was 1
in its first fight and 2 in its second).

B cannot answer (A's count is left) when B's type has 80h in its targets
word (+5), when +22 is 0 (B is not next to A), when B's targets AND 3Ch
do not name A's class (+0Ch), or when B's targets have 40h.

The percentages (T0408:29C6, before the fight; the tool reckons them
again from the map and says whether the record has them): 0 both unless
A stands on one of the six squares around B.  Else, d being A's
direction from B (0 up, then clockwise): pct24 is the sum of the bytes
F27EE:000D + k for k = 1 .. 5 where the square in direction d + k from B
holds a unit on A's side whose type's targets name B's class (or have 2
where B's +6 has it) and have not 40h; a byte's sum, so it wraps at 256.
pct25 is 50h for each of the two squares next to both units (d' + 1 and
d' + 5 from A, d' B's direction from A) that holds such a unit on B's
side against A, at most 96h.  "On the side of" is T0D36:00F5 on the two
units' +4: both have bit 1, or both bit 2, or neither has bit 1.
"""
import argparse
import struct
import sys

FAR = 0x27EE                            # the frame of the game's state
DATA = 0x2D8D
RECORD = 0x2716                         # F27EE: the fight record
UNITS = 0x2898                          # F27EE: the units, 1Ah bytes each
TYPES = 0x001F                          # F27EE: UNIT.DAT, 44h bytes a type
BONUS = 0x000D                          # F27EE: pct24's bytes by direction
WIDTH = 0x246E                          # F27EE: the map's width, +2 its height
MAPS = 0x4152                           # F27EE: the two windows' far pointers to the squares
SCENES = (0x2D65, 0x000E)               # att, def by the ground's scene


class DivideError(Exception):
    pass


def s32(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


def mul(a, b):
    return s32(a * b)


def div(a, b):
    """CODE:3D30: signed longs, cut off towards 0"""
    if b == 0:
        raise DivideError()
    q = abs(a) // abs(b)
    return -q if (a < 0) != (b < 0) else q


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
        """a far pointer stored at a -> (linear address, the pointer as a long)"""
        off, seg = struct.unpack_from('<HH', self.m, a)
        return seg * 16 + off, (seg << 16) | off


def rand(state):
    state = (state * 0x015A4E35 + 1) & 0xFFFFFFFF
    return state, (state >> 16) & 0x7FFF


def same_side(x, y):
    """T0D36:00F5 on two units' +4"""
    if x & 1:
        return bool(y & 1)
    if x & 2:
        return bool(y & 2)
    return not y & 1


def neighbours(ram, pos):
    """T0BA0:0116: the six squares around the square at offset pos (two
    bytes a square), FFFFh for one off the map"""
    w, h = ram.w(ram.lin(FAR, WIDTH)), ram.w(ram.lin(FAR, WIDTH + 2))
    x, y = (pos >> 1) % w, (pos >> 1) // w
    n = [0] * 6
    n[0], n[3] = pos - 2 * w, pos + 2 * w
    if x & 1:
        n[1], n[5], n[4], n[2] = pos + 2, pos - 2, pos + 2 * w - 2, pos + 2 * w + 2
        if y <= 0:
            n[0] = 0xFFFF
        elif h - 1 <= y:
            n[3] = n[2] = n[4] = 0xFFFF
        if x <= 0:
            n[5] = n[4] = 0xFFFF
        elif w - 1 <= x:
            n[1] = n[2] = 0xFFFF
    else:
        n[2], n[4], n[1], n[5] = pos + 2, pos - 2, pos - 2 * w + 2, pos - 2 * w - 2
        if y <= 0:
            n[0] = n[5] = n[1] = 0xFFFF
        elif h - 1 <= y:
            n[3] = 0xFFFF
        if x <= 0:
            n[4] = n[5] = 0xFFFF
        elif w - 1 <= x:
            n[1] = n[2] = 0xFFFF
    return [v & 0xFFFF for v in n]


def unit(ram, number):
    return ram.lin(FAR, UNITS) + 0x1A * number


def flank(ram, squares, win, a, b):
    """T0408:2BBD: the squares around b's that hold a unit on a's side
    which can fire at b, and a's direction from b (None when not there)"""
    ub = unit(ram, b)
    cls, other = ram.w(ub + 4) & 0x3C, ram.w(ub + 6) & 2
    n = neighbours(ram, ram.w(ub + 0x0B + 2 * win) - 1)
    held, way = [0] * 6, None
    for i in range(6):
        if n[i] == 0xFFFF:
            continue
        u = ram.b(squares + n[i] + 1)
        if u > 0xF0:
            continue
        targets = ram.w(ram.lin(FAR, TYPES) + 0x44 * ram.b(unit(ram, u) + 8) + 5)
        if (cls & targets or other & targets) and not targets & 0x40 \
                and same_side(ram.w(unit(ram, u) + 4), ram.w(unit(ram, a) + 4)):
            held[i] = 1
        if u == a:
            way = i
    return held, way


def percentages(ram, a, b):
    """T0408:29C6 -> (pct24, pct25); None when the tool cannot tell"""
    win = 0 if ram.b(ram.lin(FAR, 0x26CA)) == 2 else 1
    # T0408:000B takes the window's squares, then swaps the two pointers
    # for the fight: at fight_reckon the other one is that window's
    squares = ram.far(ram.lin(FAR, MAPS + 4 * (1 - win)))[0]
    n = neighbours(ram, ram.w(unit(ram, b) + 0x0B + 2 * win) - 1)
    # as the code: the byte after the square's offset, FFFFh + 1 being 0
    if all(ram.b(squares + ((v + 1) & 0xFFFF)) != a for v in n):
        return 0, 0
    held, way = flank(ram, squares, win, a, b)
    if way is None:
        return None
    p24 = sum(ram.b(ram.lin(FAR, BONUS) + k) for k in range(1, 6) if held[(way + k) % 6]) & 0xFF
    held, way = flank(ram, squares, win, b, a)
    if way is None:
        return None
    p25 = min(0x50 * (held[(way + 1) % 6] + held[(way + 5) % 6]), 0x96)
    return p24, p25


def count(ram, u, t):
    """-> (the count as reckoned with, the offset of its byte)"""
    off = 3 if ram.w(t + 0x0E) & 4 else 2
    n = ram.b(u + off)
    return (1 if n == 0 else min(n, 6)), off


def hit(ram, t, cls):
    return ram.b(t + (9 if cls & 0x10 else 0x0B if cls & 8 else 0x0A))


def boosted(v, pct):
    return ((v << 16) + mul(div(v << 16, 100), pct)) >> 16


def strength(n, v, e, g, d):
    return mul(n, v) + div(mul(n, mul(v, div(e << 8, 6) + 1)), d) + mul(g, div(v, 2))


def signed(b):
    return b - 256 if b & 0x80 else b


def reckon(ram, say):
    r = ram.lin(FAR, RECORD)
    (ua, pa), (ub, pb) = ram.far(r), ram.far(r + 4)
    ta, tb = ram.far(r + 0x0A)[0], ram.far(r + 0x0E)[0]
    ga, gb = ram.far(r + 0x12)[0], ram.far(r + 0x16)[0]
    targets = ram.w(tb + 5)
    silent = bool(targets & 0x80) or ram.w(r + 0x22) == 0 \
        or not targets & 0x3C & ram.w(ta + 0x0C) or bool(targets & 0x40)
    na, offa = count(ram, ua, ta)
    nb, offb = count(ram, ub, tb)
    ea, eb = ram.b(ua + 1) + 1, ram.b(ub + 1) + 1
    arma, armb = ram.b(ta + 1), ram.b(tb + 1)
    hita, hitb = hit(ram, ta, ram.w(ub + 4)), hit(ram, tb, ram.w(ua + 4))
    p24, p25 = ram.b(r + 0x24), ram.b(r + 0x25)
    say('A: unit %02X type %d count %d (+%d) e %d armour %d hit %d ground scene %d' % (
        ram.b(r + 8), ram.b(ua + 8), na, offa, ea, arma, hita, ram.b(ga + 5)))
    say('B: unit %02X type %d count %d (+%d) e %d armour %d hit %d ground scene %d' % (
        ram.b(r + 9), ram.b(ub + 8), nb, offb, eb, armb, hitb, ram.b(gb + 5)))
    say('next to each other %d, percentages %d %d, B %s' % (
        ram.w(r + 0x22), p24, p25, 'cannot answer' if silent else 'answers'))
    hita, armb = boosted(hita, p24), boosted(armb, p25)
    scenes = ram.lin(*SCENES)
    atta, defa = (signed(ram.b(scenes + 2 * ram.b(ga + 5) + i)) for i in (0, 1))
    attb, defb = (signed(ram.b(scenes + 2 * ram.b(gb + 5) + i)) for i in (0, 1))
    P = strength(na, hita, ea, atta, 0x100)
    Q = strength(na, arma, ea, defa, 0x100)
    R = strength(nb, hitb, eb, attb, 0x100)
    S = strength(nb, armb, eb, defb, 0x200)
    ca = 6 if ea > 7 else ea                    # never below 1
    cb = 2 if eb <= 1 else 6 if eb >= 7 else eb
    lossb = div(P - S, div(mul(armb, 8 - ca), 2))
    lossa = div(R - Q, div(mul(arma, 8 - cb), 2))
    say('hitA %d armB %d, P %d Q %d R %d S %d, losses A %d B %d' % (hita, armb, P, Q, R, S, lossa, lossb))
    if lossb <= 0 and div(S, nb) + div(armb, 2) < P:
        lossb = 1
    if lossa <= 0 and div(Q, na) + div(arma, 2) < R:
        lossa = 1
    if 6 < lossb < 12:
        lossb = 6
    if 6 < lossa < 12:
        lossa = 6
    lossb, lossa = min(lossb, 2 * na), min(lossa, 2 * nb)
    state = ((pa + pb) ^ (ram.w(r + 0x2E) | ram.w(r + 0x30) << 16)) & 0xFFFF
    state, r1 = rand(state)
    state, r2 = rand(state)
    r1, r2 = r1 % 0x201, r2 % 0x201
    say('seed %04X random %d %d' % ((pa + pb ^ ram.w(r + 0x2E)) & 0xFFFF, r1, r2))
    if 0x34 - 6 * ea > r1:
        lossb -= 1
    if 0x1DB - 0x14 * ea < r1:
        lossb += 1
    if 0x34 - 6 * eb > r2:
        lossa -= 1
    if 0x1DB - 0x14 * eb < r2:
        lossa += 1
    lossb, lossa = max(0, min(lossb, nb)), max(0, min(lossa, na))
    return (ram.b(r + 8), ua, offa, ram.b(ua + offa) if silent else na - lossa), \
           (ram.b(r + 9), ub, offb, nb - lossb)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('entry')
    ap.add_argument('--exit')
    ap.add_argument('--load', default='0077')
    ap.add_argument('-v', action='store_true', help='the values on the way')
    a = ap.parse_args()
    load = int(a.load, 16)
    ram = Ram(a.entry, load)
    say = print if a.v else (lambda s: None)
    ok = True
    try:
        sides = reckon(ram, say)
    except DivideError:
        print('a divisor is 0: the game ends with a divide error here')
        return 1
    r = ram.lin(FAR, RECORD)
    p = percentages(ram, ram.b(r + 8), ram.b(r + 9))
    has = (ram.b(r + 0x24), ram.b(r + 0x25))
    if p is None:
        print('percentages: not reckoned (the attacker is not found beside the target)')
    else:
        print('percentages %d %d: %s' % (p + (('as the record',) if p == has else ('the record has %d %d' % has,))))
        ok = ok and p == has
    out = Ram(a.exit, load) if a.exit else None
    for name, (number, u, off, n) in zip('AB', sides):
        line = '%s: unit %02X count %d -> %d' % (name, number, ram.b(u + off), n)
        if out:
            got = out.b(u + off)
            line += ': as the game' if got == n else ': the game has %d' % got
            ok = ok and got == n
            if off == 3 and n == 0 and out.b(u + 2) != 0:
                line += ' (+2 is not 0)'
                ok = False
        print(line)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
