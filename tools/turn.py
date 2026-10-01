#!/usr/bin/env python3
"""How Battle Isle carries out a move and changes the phase: BATTLE.EXE's
routines done again from a run's memory and compared with what the game
made of it.

    turn.py --step ENTRY EXIT [--load SEG] [-v]
    turn.py --change ENTRY EXIT [--load SEG] [-v]

ENTRY and EXIT are a run's memory (run.py -ram) stopped at a routine's
entry and at its end.  The tool does on a copy of ENTRY what the routine
does and says for each part of the game's state whether EXIT has the same
bytes: the units (F27EE:2898, F1h records of 1Ah bytes), the two players'
maps (the far pointers F27EE:4152 and 4156; two bytes a square, the
ground and the unit, FFh or F4h none), the cargo records (F27EE:0B64, 46h
of 1Ch bytes), the depots (259C), factories (2780) and headquarters
(24B0), the players' records (243E, 17h bytes each), the squares' marks
(1339), the move's record (1326), and after a change the cursors' states
and modes, the counts, the round and the score.  Nothing of the game's
files is read.  --load is the segment the program was loaded at (0077).

Each player has a map of his own, and a unit a square in each (the
words +0Bh and +0Dh, the square's offset plus 1; the offset alone for a
unit inside a building) and a direction in each (+0Fh, +10h).  What a
player does in his phase is done in his map at once; the other's map
gets it when the phase changes.

--step: one call of T122D:0713 (`-break LT122D_0713#N`, the end
`-break LT122D_0AB6#N`), a square of a move.  The player chose the unit
(fire with up on it, or taken out of a building), put the cursor on a
square in reach and pressed fire: T122D:02D8 asks stop_check (below)
whether the unit may stop there, has find_path write the path into the
player's list (tools/moves.py) and marks its squares.  Fire again on the
same square, T122D:05B8: the unit's number goes into the player's list
of orders (the far pointer at the player's +11h, the count at +15h), the
unit's +11h keeps the square it leaves, F27EE:0B62 is the path's length
less 2 (the next square's index; the path is written from the aim back),
and timer 4 is set to run out at once.  When it has, the map's loop
calls T122D:0713:

    the unit F27EE:1326 goes to the path's square [0B62]: its old square
    in the mover's map gets back the byte kept in F27EE:1328 (FFh, or for
    a unit out of a building the record's +1Bh), 1328 keeps the new
    square's unit byte, the unit's number goes there, its direction is
    the way it went (0 up, then clockwise) and its +0Bh/+0Dh the square;
    a unit of two squares (40h in +4) drags its second half, the next
    record, onto the square it left (1329 keeps that one's byte);
    the units inside a unit that holds others (1000h) get its square;
    more squares to go: 0B62 less 1 and timer 4 again, after the type's
    +16h passes of the map's loop (so +16h is how slowly a type moves);
    at the aim: stop_check for the square, then by its kind (T122D:0ABD)
      1  the unit takes the unit there in: that one into the mover's
         cargo slot, the mover onto its square, the other gets 4200h in
         its +4, the mover keeps the other's old +4 in its +17h;
         message 11h or 12h
      2  the unit goes into the unit there (its slot; 4200h; what the
         mover holds itself goes into free slots there too)
      3  a plain move
      4  onto a building of the other side or of nobody: the record's
         flags into the unit's +17h, its number into +15h; the building
         is taken when the phase changes
      5  into a building of the own side: its slot, the unit off the map
         (its square F4h), 4200h
      the unit's +13h is the kind, +14h gets bit 1, +15h a building's or
      cargo record's number, +16h the other unit, +19h the slot; +4 loses
      100h (chosen) and gets 200h (has moved; the square's mark 10h, 20h
      for player 1's map) unless it is inside now; message 6.

stop_check (T122D:0F50; square, side, map) fills the record F27EE:1326
(+0 the mover, +1 the unit on the square, +4 a slot, +5 far a cargo or
building record, +9 its number, +0Ah the square, +0Ch the kind) and
gives 0 or the message that says why not; src/BATTLE.hints has the
rules, the tool does them.

--change: T0408:000B (`-break LT0408_000B#N`, the end `-break
LT0408_23F4#N`), what happens when both players asked for the change of
phase.  D is the player whose mode (the cursor record's +16h) is 2,
attack; M the other, who moved.

    D's orders (his list of units; a unit's +11h is its target's square):
      the fight for each (tools/fight.py: the record F27EE:2716, the
      percentages, the reckoning), then a unit left with 0 is taken off
      D's map and gets 4 in its +14h, the units inside a unit that holds
      others lose what it lost (4 in +14h for one with no more than
      that), and the experience (+1) rises, to 6 at most.
    every unit with 4 in its +14h: the end of its move, if it has one
      (bit 1), is taken back (T122D:15DF: its slot emptied, a unit it
      took in put back), and it is dead (T169E:0585: 8000h alone in +4,
      the counts of units less; the units inside it too, its cargo
      record 8000h).
    every unit by the kind in its +13h:
      1  the unit taken in becomes M's: +4's bits 1 and 2 are the
         player, +9 the type's serial for him, the counts of units
      4  the building is taken: its slots as D sees them become M's
         view, the units inside become M's, the counts of factories or
         depots change, its square in D's map gets the ground of AMOK.DAT
         for the new owner, the taker goes into a free slot (when there
         is none, the unit of slot 0 is dead).  A headquarters: the
         routine returns M, the map is won.
      5  a unit that holds others, gone into a building: each unit there
         with 4 in its +6 becomes 8 of energy a piece of its count (FAh
         at most) and is gone
      0Bh, 0Dh the pioneers' building site and depot (read, not run)
    the cargo and building records: slots and energy of M's side from
      M's view into D's, the others from D's into M's (T0408:2730);
    D's map: all units off, M's units get their squares and directions
      of M's view for D's, a unit with a count (+2) of 0 is dead
      (T0408:2FAF), every unit not inside something is put on its square;
      then M's map becomes a copy of D's.
    the cursors: state 0, the modes exchanged; no orders; the round
      (F27EE:251D) one more when player 0 is to move.
    every unit: +4 loses 100h..800h; one that moved, of a type with
      flag 1 in +0Eh and not inside, gets 200h again with its mark (it
      cannot fire in the phase after, presumably); with 8000h in its +6
      its count rises by one up to the type's (and its cargo's);
      +11h, +13h .. +16h are 0.
    the end: the units with neither 20h nor 4 in +6 counted for each
      player; none of either: both results 15h; none of player 0: results
      12h and 11h; none of player 1: 11h and 12h; with a result both
      cursors get state 7 (the map's loop then shows the messages and
      ends the map; for a headquarters it sets 0Fh for the taker and 10h).
    the score (F27EE:2597): the armour of all units that count, plus 100
      with HIDE SHOP, times 4, 3 or 2 for a limit of 4, 8 or 16 turns, at
      most 7EF4h.

Checked against runs: docs/HANDOFF.md says which.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fight

FAR = 0x27EE
UNITS, TYPES, GROUND = 0x2898, 0x001F, 0x0751
CARGO, DEPOTS, FACTORIES, HQS = 0x0B64, 0x259C, 0x2780, 0x24B0
PLAYERS, CURSORS, MAPS, MARKS = 0x243E, 0x26B4, 0x4152, 0x1339
REC, AMOK = 0x1326, 0x24E8
NONE = 0xF0                             # a unit byte above this: no unit


class Mem(fight.Ram):
    """a run's memory that can be written"""

    def __init__(self, path, load):
        with open(path, 'rb') as f:
            self.m = bytearray(f.read())
        self.load = load
        self.F = (load + FAR) * 16

    def sb(self, a, v):
        self.m[a] = v & 0xFF

    def sw(self, a, v):
        struct.pack_into('<H', self.m, a, v & 0xFFFF)

    def ptr(self, a):
        return self.far(a)[0]

    def sfar(self, a, lin):
        """a far pointer into the frame of the game's state"""
        struct.pack_into('<HH', self.m, a, lin - self.F, self.load + FAR)

    def unit(self, n):
        return self.F + UNITS + 0x1A * n

    def utype(self, n):
        return self.F + TYPES + 0x44 * self.b(self.unit(n) + 8)

    def cargo(self, n):
        return self.F + CARGO + 0x1C * (n - 256 if n & 0x80 else n)

    def squares(self, player):
        return self.ptr(self.F + MAPS + 4 * player)

    def size(self):
        return self.w(self.F + 0x246E), self.w(self.F + 0x2470)

    def ground(self, squares, off):
        return self.F + GROUND + 6 * self.b(squares + off)

    def mark(self, off):
        """the mark of the square at offset off (two bytes a square)"""
        w, h = self.size()
        return self.F + MARKS + (off >> 1) % w + 64 * ((off >> 1) // w)

    def other_half(self, n):
        return n + 1 if self.w(self.unit(n) + 4) & 0x80 else n - 1


def same_side(a, b):
    return fight.same_side(a, b)


def first_free(mem, rec, side):
    for i in range(7):
        if mem.b(rec + 7 * side + i) > NONE:
            return i
    return 7


def count_slots(mem, side, rec, used):
    """T122D:18A9"""
    return sum(1 for i in range(7) if (mem.b(rec + 7 * side + i) <= NONE) == bool(used))


def cargo_size(mem, rec, side):
    """T1479:12B5"""
    return sum(mem.b(mem.utype(mem.b(rec + 7 * side + i)) + 0x40)
               for i in range(7) if mem.b(rec + 7 * side + i) <= NONE)


def find_building(mem, sq, table):
    """T0E9B:1587"""
    for n in range(10):
        if sq in (mem.w(table + 0x1C * n + 0x0E + 2 * i) for i in range(4)):
            return n
    return 0xFFFF


def stop_check(mem, sq, side, squares):
    """T122D:0F50 -> the message, 0 when the unit may stop there"""
    F, rec = mem.F, mem.F + REC
    U = mem.unit(mem.b(rec))
    msg = 0
    mem.sb(rec + 0x0C, 3)
    mem.sw(rec + 0x0A, sq)
    there = mem.b(squares + sq + 1)
    if there <= NONE:
        mem.sb(rec + 1, there)
        u = mem.unit(there)
        if mem.w(u + 4) & 0x40 and not mem.w(u + 4) & 0x80:
            there -= 1
            u = mem.unit(there)
        u6, U6 = mem.w(u + 6), mem.w(U + 6)
        if u6 & 0x20 and U6 & 0x1000 or u6 & 4 and U6 & 0x2000:
            c = mem.cargo(mem.b(U + 0x0A))
            mem.sfar(rec + 5, c)
            i = first_free(mem, c, side)
            if i == 7 or cargo_size(mem, c, side) + mem.b(mem.utype(there) + 0x40) > mem.b(mem.utype(mem.b(rec)) + 0x3F):
                msg = 9
            else:
                mem.sb(rec + 4, i)
                mem.sb(rec + 0x0C, 1)
        elif not same_side(mem.w(u + 4), side):
            msg = 5
        elif not mem.w(u + 4) & 0x1000:
            msg = 7
        else:
            c = mem.cargo(mem.b(u + 0x0A))
            mem.sfar(rec + 5, c)
            mem.sb(rec + 9, mem.b(u + 0x0A))
            if not U6 & mem.w(c + 0x19) & 0x0FC0 or mem.w(u + 4) & 0x200:
                msg = 8
            else:
                i = first_free(mem, c, side)
                if i == 7:
                    msg = 9
                else:
                    fits = True
                    size = mem.b(mem.utype(mem.b(rec)) + 0x40)
                    if mem.w(U + 4) & 0x1000:
                        mine = mem.cargo(mem.b(U + 0x0A))
                        size += cargo_size(mem, mine, side)
                        if count_slots(mem, side, c, 0) < count_slots(mem, side, mine, 1) + 1:
                            fits = False
                    size += cargo_size(mem, c, side)
                    if size > mem.b(mem.utype(there) + 0x3F) or not fits:
                        msg = 9
                    else:
                        mem.sb(rec + 4, i)
                        mem.sb(rec + 0x0C, 2)
        return msg
    g = mem.w(mem.ground(squares, sq))
    B = None
    if g & 0x400:
        n = find_building(mem, sq, F + FACTORIES)
        B = F + FACTORIES + 0x1C * n
    elif g & 0x100:
        n = find_building(mem, sq, F + DEPOTS)
        B = F + DEPOTS + 0x1C * n
    elif g & 0x40:
        n = g & 1
        B = F + HQS + 0x1C * n
    if B is not None:
        mem.sb(rec + 9, n)
        i = first_free(mem, B, side)
        if not same_side(mem.w(B + 0x19), side) or mem.w(B + 0x19) & 2:
            mem.sb(rec + 0x0C, 4)
            mem.sfar(rec + 5, B)
        else:
            if mem.w(U + 4) & 0x1000:
                mine = mem.cargo(mem.b(U + 0x0A))
                if count_slots(mem, side, B, 0) < count_slots(mem, side, mine, 1) + 1:
                    i = 7
            if i != 7:
                mem.sb(rec + 4, i)
                mem.sb(rec + 0x0C, 5)
                mem.sfar(rec + 5, B)
            else:
                msg = 9
        return msg
    if not mem.b(mem.utype(mem.b(rec)) + 4) & mem.b(mem.ground(squares, sq) + 2):
        msg = 0x0E
    if mem.w(U + 4) & 0x10 and g & 0x4000:
        msg = 0x0E
    return msg


def direction(mem, a, b):
    """T122D:1516: the way from the square at offset a to the one at b"""
    w, h = mem.size()
    x1, y1, x2, y2 = (a >> 1) % w, (a >> 1) // w, (b >> 1) % w, (b >> 1) // w
    if x2 == x1:
        return 0 if y2 < y1 else 3
    if x1 & 1:
        if x2 < x1:
            return 4 if y2 > y1 else 5
        return 2 if y2 > y1 else 1
    if x2 < x1:
        return 5 if y2 < y1 else 4
    return 1 if y2 < y1 else 2


def cargo_follows(mem, n, side):
    """T169E:07EA: the units inside unit n get its square"""
    c = mem.cargo(mem.b(mem.unit(n) + 0x0A))
    pos = mem.w(mem.unit(n) + 0x0B + 2 * side)
    for i in range(7):
        v = mem.b(c + 7 * side + i)
        if v <= NONE:
            mem.sw(mem.unit(v) + 0x0B + 2 * side, pos)
    mem.sw(c + 0x0E, pos)


def cargo_moves(mem, side, src, dst):
    """T122D:1910: the units of one record into the free slots of another"""
    for i in range(7):
        v = mem.b(src + 7 * side + i)
        if v > NONE:
            continue
        for j in range(7):
            if mem.b(dst + 7 * side + j) > NONE:
                mem.sb(dst + 7 * side + j, v)
                mem.sw(mem.unit(v) + 0x0B + 2 * side, mem.w(dst + 0x0E))
                mem.sb(src + 7 * side + i, 0xFF)
                break


def set_flag(mem, n, flag, side, on):
    """T0E9B:19ED: a flag of a unit's +4 with the mark on its square
    (100h: 40h, 200h: 10h; twice that in player 1's map), both halves"""
    bit = (0x40 if flag & 0x100 else 0x10 if flag & 0x200 else 0) << (side & 1)
    for k in [n] + ([mem.other_half(n)] if mem.w(mem.unit(n) + 4) & 0x40 else []):
        u = mem.unit(k)
        m = mem.mark(mem.w(u + 0x0B + 2 * side) - 1)
        if on:
            mem.sw(u + 4, mem.w(u + 4) | flag)
            mem.sb(m, mem.b(m) | bit)
        else:
            mem.sw(u + 4, mem.w(u + 4) & ~flag)
            mem.sb(m, mem.b(m) & ~bit)


def arrive(mem, side, squares):
    """T122D:0ABD -> the message"""
    rec = mem.F + REC
    n = mem.b(rec)
    U = mem.unit(n)
    kind = mem.b(rec + 0x0C)
    pos = U + 0x0B + 2 * side
    holds = mem.w(U + 4) & 0x1000
    msg = 0
    if kind == 1:
        other = mem.b(rec + 1)
        u = mem.unit(other)
        mem.sb(mem.ptr(rec + 5) + 7 * side + mem.b(rec + 4), other)
        mem.sw(pos, mem.w(rec + 0x0A) + 1)
        mem.sb(squares + mem.w(pos), n)
        cargo_follows(mem, n, side)
        mem.sw(U + 0x17, mem.w(u + 4))
        mem.sw(u + 4, mem.w(u + 4) | 0x4200)
        mem.sw(U + 4, mem.w(U + 4) & 0xBEFF | 0x200)
        msg = 0x11 if mem.w(u + 6) & 0x20 else 0x12
        set_flag(mem, n, 0x200, side, 1)
        mem.sb(U + 0x13, kind)
        mem.sb(U + 0x14, mem.b(U + 0x14) | 1)
        mem.sb(U + 0x19, mem.b(rec + 4))
        mem.sb(U + 0x16, other)
    elif kind == 2:
        mem.sb(mem.ptr(rec + 5) + 7 * side + mem.b(rec + 4), n)
        mem.sb(squares + mem.w(pos), mem.b(rec + 1))
        mem.sw(U + 4, mem.w(U + 4) & 0xFEFF | 0x4200)
        if holds:
            cargo_moves(mem, side, mem.cargo(mem.b(U + 0x0A)), mem.ptr(rec + 5))
        mem.sb(U + 0x13, kind)
        mem.sb(U + 0x19, mem.b(rec + 4))
        mem.sb(U + 0x15, mem.b(rec + 9))
        mem.sb(U + 0x16, mem.b(rec + 1))
        mem.sb(U + 0x14, mem.b(U + 0x14) | 1)
        msg = 6
    elif kind in (3, 4):
        msg = 6
        mem.sw(U + 4, mem.w(U + 4) & 0xBEFF | 0x200)
        set_flag(mem, n, 0x200, side, 1)
        mem.sb(U + 0x13, kind)
        mem.sb(U + 0x14, mem.b(U + 0x14) | 1)
        if holds:
            cargo_follows(mem, n, side)
        if kind == 3:
            mem.sb(U + 0x15, 0)
            mem.sb(U + 0x19, 0)
            mem.sw(U + 0x17, 0)
        else:
            mem.sw(U + 0x17, mem.w(mem.ptr(rec + 5) + 0x19))
            mem.sb(U + 0x15, mem.b(rec + 9))
        mem.sb(U + 0x16, 0)
    elif kind == 5:
        B = mem.ptr(rec + 5)
        mem.sb(U + 0x13, kind)
        mem.sb(U + 0x14, mem.b(U + 0x14) | 1)
        mem.sb(U + 0x19, mem.b(rec + 4))
        mem.sb(U + 0x15, mem.b(rec + 9))
        mem.sw(U + 0x17, mem.w(B + 0x19))
        mem.sb(B + 7 * side + mem.b(rec + 4), n)
        if holds:
            cargo_moves(mem, side, mem.cargo(mem.b(U + 0x0A)), B)
        mem.sb(squares + mem.w(pos), 0xF4)
        mem.sw(pos, mem.w(B + 0x0E))
        mem.sw(U + 4, mem.w(U + 4) & 0xFEFF | 0x4200)
        msg = 6
    return msg


def step(mem, say):
    """T122D:0713"""
    F = mem.F
    n = mem.b(F + REC)
    U = mem.unit(n)
    side = mem.w(U + 4) & 1
    squares = mem.squares(side)
    left = mem.w(F + 0x0B62)
    to = mem.w(mem.ptr(F + PLAYERS + 0x17 * side + 0x0D) + 0x1F40 + 2 * left)
    w, h = mem.size()
    say('unit %02X (type %d) of player %d to column %d row %d, %d more' % (
        n, mem.b(U + 8), side, (to >> 1) % w, (to >> 1) // w, left))
    if left == 0:
        say('stop_check: message %X, kind %d' % (stop_check(mem, to, side, squares), mem.b(F + REC + 0x0C)))
    pos = U + 0x0B + 2 * side
    if mem.w(U + 4) & 0x40:
        H = mem.unit(n + 1)
        old = mem.w(pos)
        mem.sb(squares + mem.w(H + 0x0B + 2 * side), mem.b(F + 0x1329))
        mem.sb(F + 0x1329, mem.b(F + 0x1328))
        mem.sb(F + 0x1328, mem.b(squares + to + 1))
        mem.sb(squares + to + 1, n)
        mem.sb(U + 0x0F + side, direction(mem, old - 1, to))
        mem.sw(pos, to + 1)
        mem.sb(squares + old, n + 1)
        mem.sb(H + 0x0F + side, mem.b(U + 0x0F + side))
        mem.sw(H + 0x0B + 2 * side, old)
    else:
        mem.sb(squares + mem.w(pos), mem.b(F + 0x1328))
        mem.sb(F + 0x1328, mem.b(squares + to + 1))
        mem.sb(squares + to + 1, n)
        mem.sb(U + 0x0F + side, direction(mem, mem.w(pos) - 1, to))
        mem.sw(pos, to + 1)
    if mem.w(U + 4) & 0x1000:
        cargo_follows(mem, n, side)
    if left > 0:
        mem.sw(F + 0x0B62, left - 1)
        return
    msg = arrive(mem, side, squares)
    say('arrived: message %X' % msg)
    mem.sw(F + 0x250C, mem.w(F + 0x250C) | 4 << side)
    mem.sw(F + 0x2510, (mem.w(F + 0x2510) | 8 << side) & 0xFF7F)
    mem.sw(U + 4, mem.w(U + 4) & 0xFEFF)
    if mem.w(U + 4) & 0x40:
        o = mem.unit(mem.other_half(n))
        mem.sw(o + 4, mem.w(o + 4) & 0xFEFF)


# ---- the change of phase

def remove(mem, n, player, depth=1):
    """T169E:0585: a unit is dead"""
    F = mem.F
    if depth > 5 or mem.w(mem.unit(n) + 4) & 0x8000:
        return
    count = 1
    U = mem.unit(n)
    if mem.w(U + 4) & 0x40:
        if mem.w(U + 4) & 0x80:
            half = n + 1
        else:
            half, n = n, n - 1
        count = 2
        mem.sw(mem.unit(half) + 4, 0x8000)
        U = mem.unit(n)
    if mem.w(U + 4) & 0x1000:
        c = mem.cargo(mem.b(U + 0x0A))
        side = player
        for i in range(7):
            v = mem.b(c + 7 * side + i)
            if v <= NONE:
                remove(mem, v, side, depth + 1)
                mem.sb(c + 7 * side + i, 0xFF)
        f = mem.w(c + 0x19)
        if f & 1:
            side = 1
        elif not f & 2:
            side = 0
        if f & 0x10:
            a = F + PLAYERS + 0x17 * side + 4
            mem.sb(a, mem.b(a) - 1)
            mem.sb(F + 0x2516, mem.b(F + 0x2516) - 1)
        elif f & 8:
            a = F + PLAYERS + 0x17 * side + 3
            mem.sb(a, mem.b(a) - 1)
            mem.sb(F + 0x2515, mem.b(F + 0x2515) - 1)
        elif f & 0x20:
            mem.sb(F + 0x2517, mem.b(F + 0x2517) - 1)
        mem.sw(c + 0x19, 0x8000)
    mem.sb(F + 0x2514, mem.b(F + 0x2514) - count)
    if mem.w(U + 4) & 1:
        mem.sb(F + PLAYERS + 0x17 + 2, mem.b(F + PLAYERS + 0x17 + 2) - count)
    elif not mem.w(U + 4) & 2:
        mem.sb(F + PLAYERS + 2, mem.b(F + PLAYERS + 2) - count)
    mem.sw(U + 4, 0x8000)


def take_back(mem, n, squares, side):
    """T122D:15DF: the end of a dead unit's move undone"""
    U = mem.unit(n)
    kind = mem.b(U + 0x13)
    slot = 7 * side + mem.b(U + 0x19)

    def clear(k):
        u = mem.unit(k)
        mem.sb(u + 0x14, mem.b(u + 0x14) & 0xFE)
        mem.sw(u + 4, mem.w(u + 4) & 0xBDFF)
    if kind == 2:
        mem.sb(mem.cargo(mem.b(U + 0x15)) + slot, 0xFF)
        clear(n)
    elif kind in (3, 4):
        mem.sb(squares + mem.w(U + 0x0B + 2 * side), 0xFF)
        if mem.w(U + 4) & 0x40:
            o = mem.other_half(n) & 0xFF
            mem.sb(squares + mem.w(mem.unit(o) + 0x0B + 2 * side), 0xFF)
            clear(o)
        clear(n)
    elif kind == 1:
        other = mem.b(U + 0x16)
        u = mem.unit(other)
        mem.sb(squares + mem.w(U + 0x0B + 2 * side), other)
        mem.sw(u + 0x0B + 2 * side, mem.w(U + 0x0B + 2 * side))
        mem.sw(u + 4, mem.w(U + 0x17))
        mem.sb(u + 0x14, 0)
        mem.sb(mem.cargo(mem.b(U + 0x0A)) + slot, 0xFF)
        clear(n)
    elif kind == 5:
        mem.sb(building(mem, U) + slot, 0xFF)
        clear(n)
    if 1 <= kind <= 5:
        mem.sb(U + 0x13, 0)


def building(mem, U):
    f = mem.w(U + 0x17)
    table = HQS if f & 4 else FACTORIES if f & 8 else DEPOTS
    return mem.F + table + 0x1C * mem.b(U + 0x15)


def explode(mem, n, d, squares):
    """T0408:2415 and the animation's end: a unit off D's map"""
    for k in [n] + ([mem.other_half(n)] if mem.w(mem.unit(n) + 4) & 0x40 else []):
        u = mem.unit(k)
        mem.sb(squares + mem.w(u + 0x0B + 2 * d), 0xFF)
        mem.sb(u + 0x14, mem.b(u + 0x14) | 4)


def cargo_loses(mem, d, n, loss):
    """T0408:28EA"""
    c = mem.cargo(mem.b(mem.unit(n) + 0x0A))
    for i in range(7):
        v = mem.b(c + 7 * d + i)
        if v > NONE:
            continue
        u = mem.unit(v)
        if mem.b(u + 2) <= loss:
            mem.sb(u + 0x14, mem.b(u + 0x14) | 4)
            mem.sb(c + 7 * d + i, 0xFF)
        else:
            mem.sb(u + 2, mem.b(u + 2) - loss)


def fights(mem, d, say):
    F = mem.F
    squares = mem.squares(d)
    P = F + PLAYERS + 0x17 * d
    orders = mem.ptr(P + 0x11)
    rec = F + fight.RECORD
    w, h = mem.size()
    for i in range(mem.b(P + 0x15)):
        a = mem.b(orders + i)
        target = mem.w(mem.unit(a) + 0x11)
        if mem.w(mem.unit(a) + 4) & 0x40 and not mem.w(mem.unit(a) + 4) & 0x80:
            a = (a - 1) & 0xFF
        own = mem.w(mem.unit(a) + 0x0B + 2 * d) - 1
        b = mem.b(squares + target + 1)
        if b > NONE:
            continue
        if mem.w(mem.unit(b) + 4) & 0x40 and not mem.w(mem.unit(b) + 4) & 0x80:
            b -= 1
        A, B = mem.unit(a), mem.unit(b)
        mem.sw(rec + 0x22, 1 if target in fight.neighbours(mem, own) else 0)
        # fight.percentages takes the map as it is during the fight, the
        # two pointers exchanged
        p0, p1 = mem.m[F + MAPS:F + MAPS + 4], mem.m[F + MAPS + 4:F + MAPS + 8]
        mem.m[F + MAPS:F + MAPS + 8] = p1 + p0
        pct = fight.percentages(mem, a, b)
        mem.m[F + MAPS:F + MAPS + 8] = p0 + p1
        if pct is None:
            raise SystemExit('turn.py: the attacker %02X is not found beside its target' % a)
        mem.sb(rec + 0x24, pct[0])
        mem.sb(rec + 0x25, pct[1])
        mem.sfar(rec, A)
        mem.sfar(rec + 4, B)
        mem.sb(rec + 8, a)
        mem.sb(rec + 9, b)
        mem.sfar(rec + 0x0A, mem.utype(a))
        mem.sfar(rec + 0x0E, mem.utype(b))
        mem.sfar(rec + 0x12, mem.ground(squares, own))
        mem.sfar(rec + 0x16, mem.ground(squares, target))
        for off, s in ((0x1A, own), (0x1E, target)):
            mem.sw(rec + off, (s >> 1) % w)
            mem.sw(rec + off + 2, (s >> 1) // w)
        mem.m[rec + 0x2E:rec + 0x32] =mem.m[F + 0x251F:F + 0x2523]
        offa = 3 if mem.w(mem.utype(a) + 0x0E) & 4 else 2
        offb = 3 if mem.w(mem.utype(b) + 0x0E) & 4 else 2
        had_a, had_b = mem.b(A + offa), mem.b(B + offb)
        (_, _, _, na), (_, _, _, nb) = fight.reckon(mem, lambda s: None)
        for u, off, v in ((A, offa, na), (B, offb, nb)):
            mem.sb(u + off, v)
            if off == 3 and v == 0:
                mem.sb(u + 2, 0)
        say('fight: unit %02X %d -> %d, unit %02X %d -> %d' % (a, had_a, na, b, had_b, nb))
        gain_a = gain_b = 0
        if na == 0:
            explode(mem, a, d, squares)
            gain_b += 1
        if nb == 0:
            explode(mem, b, d, squares)
            gain_a += 1
        loss_a, loss_b = (had_a - na) & 0xFF, (had_b - nb) & 0xFF
        if mem.w(A + 4) & 0x1000:
            cargo_loses(mem, d, a, loss_a)
        if mem.w(B + 4) & 0x1000:
            cargo_loses(mem, d, b, loss_b)
        if loss_b and not mem.w(B + 6) & 0x20:
            gain_a += 1
        if loss_a and not mem.w(A + 6) & 0x20:
            gain_b += 1
        mem.sb(A + 1, min(6, mem.b(A + 1) + gain_a))
        mem.sb(B + 1, min(6, mem.b(B + 1) + gain_b))


def becomes(mem, n, m):
    """a unit gets player m: +4, the serial"""
    u = mem.unit(n)
    mem.sw(u + 4, mem.w(u + 4) & 0xFFFC | m)
    serial = mem.utype(n) + 0x3C + m
    mem.sb(u + 9, mem.b(serial))
    mem.sb(serial, mem.b(serial) + 2)


def count(mem, player, off, by):
    a = mem.F + PLAYERS + 0x17 * player + off
    mem.sb(a, mem.b(a) + by)


def four_squares(mem, sq):
    """T0E9B:18EF"""
    w, h = mem.size()
    x, y = (sq >> 1) % w, (sq >> 1) // w
    t = mem.F + (0x0A15 if x & 1 else 0x0A0D)
    s8 = fight.signed
    return [2 * (x + s8(mem.b(t + i))) + 2 * w * (y + s8(mem.b(t + 4 + i))) for i in range(4)]


def change(mem, say):
    """T0408:000B -> what it returns: FFh, or the player who took the
    other's headquarters"""
    F = mem.F
    ret = 0xFF
    mem.m[F + MARKS:F + MARKS + 0x1104] = bytes(0x1104)
    mem.sw(F + 0x250C, mem.w(F + 0x250C) & 0xFFF3)
    d = 0 if mem.b(F + CURSORS + 0x16) == 2 else 1
    m = 1 - d
    sq_d = mem.squares(d)
    fights(mem, d, say)
    for n in range(0xF1):
        U = mem.unit(n)
        if mem.w(U + 4) & 0x8000 or not mem.b(U + 0x14) & 4:
            continue
        holds = mem.w(U + 4) & 0x1000
        if mem.b(U + 0x14) & 1:
            take_back(mem, n, sq_d, m)
        remove(mem, n, m)
        if holds:
            c = mem.cargo(mem.b(U + 0x0A))
            for i in range(7):
                v = mem.b(c + 7 * d + i)
                if v <= NONE:
                    if mem.b(mem.unit(v) + 0x14) & 1:
                        take_back(mem, v, sq_d, m)
                    remove(mem, v, m)
        say('unit %02X is dead' % n)
    for n in range(0xF1):
        U = mem.unit(n)
        if mem.w(U + 4) & 0x8000:
            continue
        kind = mem.b(U + 0x13)
        if kind == 1:
            other = mem.b(U + 0x16)
            u = mem.unit(other)
            if mem.w(u + 4) & 0x8000:
                mem.sb(mem.cargo(mem.b(U + 0x0A)) + 7 * m + mem.b(U + 0x19), 0xFF)
                continue
            same = same_side(mem.w(U + 4), mem.w(u + 4))
            neutral = mem.w(u + 4) & 2
            if not same and not neutral:
                count(mem, 1 - m, 2, -1)
            if not same or neutral:
                count(mem, m, 2, 1)
            becomes(mem, other, m)
            say('unit %02X took unit %02X in: it is player %d\'s' % (n, other, m))
        elif kind == 5:
            if not mem.w(U + 4) & 0x1000:
                continue
            B = building(mem, U)
            for i in range(7):
                v = mem.b(B + 7 * m + i)
                if v <= NONE and mem.w(mem.unit(v) + 6) & 4:
                    e = (mem.b(mem.unit(v) + 2) << 3) & 0xFF
                    mem.sb(B + 0x16 + m, min(0xFA, mem.b(B + 0x16 + m) + e))
                    remove(mem, v, m)
                    mem.sb(B + 7 * m + i, 0xFF)
                    say('unit %02X became %d of energy' % (v, e))
        elif kind == 4:
            B = building(mem, U)
            for i in range(7):
                mem.sb(B + 7 * m + i, mem.b(B + 7 * d + i))
            slot = first_free(mem, B, m)
            if slot == 7:
                slot = 0
                remove(mem, mem.b(B + 7 * m), m)
            mem.sb(U + 0x19, slot)
            for k in range(0xF1):
                u = mem.unit(k)
                if mem.w(u + 0x0B + 2 * m) != mem.w(B + 0x0E) or mem.w(u + 4) & 0x8000:
                    continue
                if not mem.w(u + 4) & 2:
                    count(mem, 1 - m, 2, -1)
                becomes(mem, k, m)
                count(mem, m, 2, 1)
                if mem.w(u + 4) & 0x1000:
                    c = mem.cargo(mem.b(u + 0x0A))
                    mem.sw(c + 0x19, mem.w(c + 0x19) & 0xFFFC | m)
            f = mem.w(B + 0x19)
            amok = F + AMOK
            if f & 8:
                if not f & 2:
                    count(mem, 1 - m, 3, -1)
                count(mem, m, 3, 1)
                mem.sb(sq_d + mem.w(B + 0x0E), mem.b(amok + m))
            elif f & 0x10:
                if not f & 2:
                    count(mem, 1 - m, 4, -1)
                count(mem, m, 4, 1)
                mem.sb(sq_d + mem.w(B + 0x0E), mem.b(amok + 3 + m))
            elif f & 4:
                ret = m
                mem.sb(sq_d + mem.w(B + 0x0E), mem.b(amok + 6 + m))
            mem.sw(B + 0x19, f & 0xFFFC | m)
            mem.sb(B + 0x18, 5)
            mem.sb(B + 7 * m + slot, n)
            mem.sb(B + 0x16 + m, mem.b(B + 0x16 + d))
            mem.sb(sq_d + mem.w(U + 0x0B + 2 * m), 0xF4)
            mem.sw(U + 0x0B + 2 * m, mem.w(B + 0x0E))
            mem.sw(U + 4, mem.w(U + 4) & 0xFEFF | 0x4200)
            if mem.w(U + 4) & 0x1000:
                cargo_follows(mem, n, m)
            say('unit %02X took the building at offset %X (flags %04X)' % (n, mem.w(B + 0x0E), f))
        elif kind == 0x0B:
            sq_m = mem.squares(m)
            four = four_squares(mem, mem.w(U + 0x11))
            if all(mem.w(mem.ground(sq_m, s)) & 0x8000 and mem.b(sq_m + s + 1) > NONE for s in four):
                for i, s in enumerate(four):
                    mem.sb(sq_d + s, mem.b(F + AMOK + 0x18 + i))
                mem.sb(U + 0x0A, mem.b(U + 0x0A) - 1)
        elif kind == 0x0D:
            raise SystemExit('turn.py: a depot built (kind 0Dh) is not done by the tool')
    for table, n in ((CARGO, 0x46), (DEPOTS, 10), (FACTORIES, 10), (HQS, 2)):
        for k in range(n):
            B = F + table + 0x1C * k
            if mem.w(B + 0x19) & 0x8002:
                continue
            for i in range(7):
                if same_side(mem.w(B + 0x19), m):
                    v = mem.b(B + 7 * m + i)
                    if mem.w(mem.unit(v) + 4) & 0x8000:
                        v = 0xFF
                        mem.sb(B + 7 * m + i, v)
                    mem.sb(B + 7 * d + i, v)
                    mem.sb(B + 0x16 + d, mem.b(B + 0x16 + m))
                else:
                    mem.sb(B + 7 * m + i, mem.b(B + 7 * d + i))
                    mem.sb(B + 0x16 + m, mem.b(B + 0x16 + d))
    size = mem.w(F + 0x246C)
    for i in range(1, size, 2):
        mem.sb(sq_d + i, 0xFF)
    for n in range(0xF1):
        U = mem.unit(n)
        f = mem.w(U + 4)
        if f & 0x8000 or not (f & 1 if m else not f & 3):
            continue
        mem.sw(U + 0x0B + 2 * d, mem.w(U + 0x0B + 2 * m))
        mem.sb(U + 0x0F + d, mem.b(U + 0x0F + m))
    for n in range(0xF1):
        U = mem.unit(n)
        if not mem.w(U + 4) & 0x8000 and mem.b(U + 2) == 0:
            mem.sw(U + 4, mem.w(U + 4) | 0x8000)
    for n in range(0xF1):
        U = mem.unit(n)
        if not mem.w(U + 4) & 0xC000:
            mem.sb(sq_d + mem.w(U + 0x0B), n)
    sq_m = mem.squares(m)
    mem.m[sq_m:sq_m + size] = mem.m[sq_d:sq_d + size]
    c0, c1 = F + CURSORS, F + CURSORS + 0x31
    for c in (c0, c1):
        mem.sb(c + 0x17, 0)
        mem.sb(c + 0x18, 0)
        mem.sb(c + 0x19, 0)
        mem.sb(c + 0x1B, 5)
    mode = 2 if mem.b(c0 + 0x16) == 1 else 1
    mem.sb(c0 + 0x16, mode)
    mem.sb(c1 + 0x16, 3 - mode)
    mem.sb(F + PLAYERS + 0x15, 0)
    mem.sb(F + PLAYERS + 0x17 + 0x15, 0)
    mem.sw(F + 0x250C, mem.w(F + 0x250C) | 0x0C)        # the two messages
    mem.sw(F + 0x2510, mem.w(F + 0x2510) | 0x18)
    if mode == 1:
        mem.sw(F + 0x251D, mem.w(F + 0x251D) + 1)
    mem.m[F + MARKS:F + MARKS + 0x1104] = bytes(0x1104)
    for n in range(0xF1):
        U = mem.unit(n)
        mem.sw(U + 4, mem.w(U + 4) & 0xF0FF)
        f = mem.w(U + 4)
        t = mem.utype(n)
        if not f & 0x8000 and mem.b(U + 0x14) & 1 and mem.w(t + 0x0E) & 1 and not f & 0x4000:
            set_flag(mem, n, 0x200, f & 1, 1)
        if mem.w(U + 6) & 0x8000:
            def repair(k):
                u, tk = mem.unit(k), mem.utype(k)
                off = 3 if mem.w(tk + 0x0E) & 4 else 2
                if mem.b(tk + off) > mem.b(u + off):
                    mem.sb(u + off, mem.b(u + off) + 1)
            repair(n)
            if f & 0x1000:
                c = mem.cargo(mem.b(U + 0x0A))
                for i in range(7):
                    if mem.b(c + i) <= NONE:
                        repair(mem.b(c + i))
        mem.sb(U + 0x14, 0)
        mem.sw(U + 0x11, 0)
        mem.sb(U + 0x16, 0)
        mem.sb(U + 0x15, 0)
        mem.sb(U + 0x13, 0)
    n0 = n1 = 0
    for n in range(0xF1):
        U = mem.unit(n)
        if mem.w(U + 4) & 0x8000 or mem.w(U + 6) & 0x24:
            continue
        if mem.w(U + 4) & 1:
            n1 += 1
        elif not mem.w(U + 4) & 2:
            n0 += 1
    result = None
    if n0 == 0 and n1 == 0:
        result = (0x15, 0x15)
    elif n0 == 0:
        result = (0x12, 0x11)
    elif n1 == 0:
        result = (0x11, 0x12)
    if result:
        for c, r in zip((c0, c1), result):
            mem.sb(c + 0x17, 7)
            mem.sw(c + 0x22, r)
    score = 0
    for n in range(0xF1):
        U = mem.unit(n)
        f = mem.w(U + 4)
        if f & 0x8000 or f & 0x40 and not f & 0x80 or mem.w(U + 6) & 4:
            continue
        score += mem.b(mem.utype(n) + 1)
    if mem.w(F + 0x250E) & 8:
        score += 100
    score *= {4: 4, 8: 3, 0x10: 2}.get(mem.b(F + PLAYERS + 0x16), 1)
    struct.pack_into('<L', mem.m, F + 0x2597, min(score, 0x7EF4))
    say('units that count: %d and %d; the score %d; returns %02X' % (n0, n1, min(score, 0x7EF4), ret))
    return ret


def regions(mem, changed):
    F = mem.F
    size = mem.w(F + 0x246C)
    r = [('the units', F + UNITS, 0xF1 * 0x1A),
         ('player 0\'s map', mem.squares(0), size),
         ('player 1\'s map', mem.squares(1), size),
         ('the cargo records', F + CARGO, 0x46 * 0x1C),
         ('the depots', F + DEPOTS, 10 * 0x1C),
         ('the factories', F + FACTORIES, 10 * 0x1C),
         ('the headquarters', F + HQS, 2 * 0x1C),
         ('the players\' counts', F + PLAYERS, 7),
         ('', F + PLAYERS + 0x17, 7),
         ('the orders\' counts', F + PLAYERS + 0x15, 1),
         ('', F + PLAYERS + 0x17 + 0x15, 1),
         ('the marks', F + MARKS, 0x1104),
         ('the types\' serials', F + TYPES, 0x1B * 0x44),
         ('the counts of units and buildings', F + 0x2514, 4)]
    if changed:
        r += [('the cursors\' modes and states', F + CURSORS + 0x16, 4),
              ('', F + CURSORS + 0x31 + 0x16, 4),
              ('the results', F + CURSORS + 0x22, 2),
              ('', F + CURSORS + 0x31 + 0x22, 2),
              ('the round', F + 0x251D, 2),
              ('the score', F + 0x2597, 4),
              ('the last fight\'s record', F + fight.RECORD, 0x26)]
    else:
        r += [('the move\'s record', F + REC, 0x0D),
              ('the squares to go', F + 0x0B62, 2),
              ('the bytes under the unit', F + 0x1328, 2),
              ('the flags 2510', F + 0x2510, 2)]
    return r


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('entry')
    ap.add_argument('exit')
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument('--step', action='store_true')
    g.add_argument('--change', action='store_true')
    ap.add_argument('--load', default='0077')
    ap.add_argument('-v', action='store_true', help='the bytes that differ')
    a = ap.parse_args()
    load = int(a.load, 16)
    mem, out = Mem(a.entry, load), Mem(a.exit, load)
    if a.step:
        step(mem, print)
    else:
        change(mem, print)
    bad = 0
    name = ''
    for label, at, n in regions(mem, a.change):
        name = label or name
        diff = [i for i in range(n) if mem.m[at + i] != out.m[at + i]]
        was = sum(1 for i in range(n) if Mem.b(out, at + i) != open_entry(a.entry)[at + i])
        if label:
            print('%s: %d bytes, %d changed by the game, %s' % (
                name, n, was, 'all as the game\'s' if not diff else '%d differ' % len(diff)))
        elif diff:
            print('%s (player 1): %d differ' % (name, len(diff)))
        bad += len(diff)
        if a.v:
            for i in diff[:40]:
                print('   +%04X (F27EE:%04X): the tool %02X, the game %02X' % (
                    i, at + i - mem.F, mem.m[at + i], out.m[at + i]))
    return 0 if bad == 0 else 1


_entry = {}


def open_entry(path):
    if path not in _entry:
        with open(path, 'rb') as f:
            _entry[path] = f.read()
    return _entry[path]


if __name__ == '__main__':
    sys.exit(main())
