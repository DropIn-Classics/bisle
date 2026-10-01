#!/usr/bin/env python3
"""Battle Isle's computer player: BATTLE.EXE's routines done again from a
run's memory and compared with what the game made of it.

    computer.py --assess|--plan|--handout|--carry|--script ENTRY [EXIT]
                [--calls N] [--player N] [--load SEG] [-v]

ENTRY and EXIT are a run's memory (run.py -ram) stopped at a routine's
entry and at its end; the tool does on a copy of ENTRY what the routine
does and says for each part of the computer player's state whether EXIT
has the same bytes.  Nothing of the game's files is read: the tables are
the memory's.  --player is the computer's player (default 1), --load the
segment the program was loaded at (0077).  The stages are made of many
small calls, one a pass of the map's loop: with --calls N the tool does
up to N calls one after another, to the one that returns 1 (or before
one that changes nothing: the routine waits for a move to end; or to
the one that ends the stage), and says how many they were (while a move
runs, what the move itself changes is left out of the comparison); EXIT
is then the memory at the end
of the last of them (entry N of a run, the end of call N + calls - 1).
Without EXIT it only says how many calls.

How the computer player works (src/BATTLE.hints at T178C has it in
full).  The map's loop calls T178C:000D for a player who is the computer
(bit 2 of his record's +0) once a pass, and the routine does one small
step of one of five stages, by the word at F2C0A:00A2 + 0Ch * player:

    20h  assess   T17C0:0A0D  the two sides' strength, the units that
                              threaten the headquarters, the list of aims
    40h  plan     T1C04:000D  a task for every unit
    8    hand out T17C0:0004  the tasks become commands in a queue
    2    carry out T1938:012F the commands, as directions and fire for
    1             T1938:000F  the player's own cursor (F2C0A:11EE): the
                              computer moves and fires through the same
                              routines as a player with keys

(a pass goes on from a stage that is through to the next: a command
handed out is begun, and the script's first byte played, in the same
pass.)

Its state, all in the frame F2C0A: the .COM file's 27 records of 6 bytes
at 0000 (a type's worth +0, flags +2); the player's record at 00A2 (+0
the stage, +3, +5, +7 the steps of the stages, +6 the number of aims, +9
a word); a record of 9 bytes for every unit, at 00BA for the move phase
and at 0934 for the attack phase (+0 an aim's square or unit, +2, +4 a
score, +6 how much the unit threatens, +8 the task); the aims in the list
at the far pointer of the player's record +9 (F27EE:2447 + 17h * player),
6 bytes each: the score, the square or unit, two bytes of its kind.

--assess: one call of T17C0:0A0D (`-break LT17C0_0A0D#N`, the end
`-break LT17C0_177C#N`), which does the step in the record's +5:

    0  nothing but +5 = 5 in the attack phase.  Else every unit that is
       alive and somebody's: its strength (T1ABC:0008: three numbers, one
       for each of the type's hit values against land, sea and air:
       hit * (count / 2 + 1) * (experience + 1) * (armour / 8) / 32)
       added to its side's sums.  An enemy's record gets FFFFh in +2 and
       +6.  An own unit keeps the task 0Bh; keeps the task 9 while one
       of the own buildings with more than 2 of energy can be reached
       (T1ABC:027D); a unit that holds a unit with 4 in its +6 gets the
       task 0Ah with the score 190h and the nearest own factory it can
       reach, or the own headquarters, as its aim (T1B01:0E4B); every
       other unit's records are cleared.  The record's +9: 3 when the
       enemy is at least twice as strong, 2 when stronger, else 1.
    1  every enemy unit's threat (its record's +6): the type's worth
       from the .COM, 50 for a type with 40h or 80h in its targets, 25
       for each hit value that is 0, 10 for each step of experience below
       6; a type with flag 1 in the .COM's +2 that is nearer to the own
       headquarters than its move: 300 more, the record's +9 two more,
       and 300 more for the units around it.
    2  with an own unit of a type with flag 1: the aims: the enemy's
       headquarters (10 * its record's +18h), every factory and depot not
       the own (10 * +18h, 20 for each unit in it, for a factory 20 for
       every 10 of energy), kind 1.
    3  with an enemy unit of a type with flag 1: as many aims of kind 3
       as the record's +9 says (6 at most; 1 with fewer than 10 own
       units), squares around the own headquarters in the order of the
       table F2D33:0000, scores 78h + 1Eh * n.
    4  with an own unit of a type with flag 80h or 200h: every unit of
       nobody with 4 in its +6 is an aim of kind 4, score C8h.
    5  +5 = 0 and the routine returns 1: the stage is done.

--plan: T1C04:000D (`-break LT1C04_000D#N`, the end `-break
LT1C04_2C58#N`), by the record's +4 and, within a step, its +7.  A unit's
record: +8 the task, +0 its aim, +4 the score the task has (a unit is
given a task only for more than it has), +2 a distance.  Nothing but
+4 = 0Ch in the attack phase.  In the move phase:

    1..6  the aims, the best first, kind after kind (1, 2, 3, 4):
       kind 1, a building: of the own units with the .COM flag 1 the one
         with the shortest path (can_go, T1ABC:027D) for which the aim's
         score less the path's length is more than it has: task 1; a
         type with flag 20h makes itself an aim of kind 2 (score - 10);
       kind 2, a unit to go with: as many units as the type's .COM +4
         says, of types whose flags meet the unit's class, the nearest
         first: task 7, the aim the unit; the score a third less each;
       kind 3, a square by the headquarters: a unit standing on it with
         flag 40h stays (task 4), else the nearest with flag 40h: task 1;
       kind 4, a unit of nobody: T1ED2:0005 (below) with the task 0Ah.
    7  every own unit that is not whole and no ship, with v = 0Fh, or
       46h + 10 * experience for one with fewer than 5 and experience
       above 1: the own buildings with more than 2 of energy, the last
       of the list first; one whose path (no enemy's square on it) is
       no longer than the type's move: task 1 there with the score 2 v,
       if that is more than the unit has; else, if v is more than it
       has, T1ED2:0005 with the task 9, the score v and the building's
       square (not for the types with flags 80h..200h).
    8  for every enemy unit, in its record's +2: what its type cannot
       fire at (1 a type with 40h or 80h in the targets; 2, 4, 8 a hit
       value of 0 against air, sea, land).
    9  every own unit with flag 2 that has not moved: its targets
       (fire_reach); with one, the unit stays (task 4, score 64h) and
       its attack record gets the task 0Eh and the target that
       threatens most.
    0Ah the enemy unit that threatens most (and is not done): the own
       units nearer than half their move that can fire at it and have
       less than its threat as score are tried on the six squares
       around it (in reach, free, nobody's aim yet, not crowded:
       T1B01:0C27, more than three enemies around or two opposite);
       then at most four are chosen: first units that can fire at what
       the enemy cannot answer, then one opposite a square that is
       taken, pairs on opposite squares, one between two that are
       taken, any.  Each gets the task 3 (4 if it stands there), the
       square, the enemy's threat as score, and in its attack record
       the task 2 with the enemy's square.  Then the next enemy.
    0Bh every unit without a task (not the types with 80h..200h): the
       distances of all others (for an own unit the distance of its
       aim); it goes to the enemy that threatens most which it can fire
       at and reach (task 1, score 0Ah), else to the aim of the nearest
       own unit with a task worth more than 0Ah, else T1ED2:0005 with
       the task 0Bh, the score 46h and the own headquarters.
    0Ch +4 = 0, no aims, and the routine returns 1.

  T1ED2:0005 (player, unit, score, task, square), in steps (+8): a unit
  with 4 in its +6 is not moved itself.  Else its reach (all ground
  flags allowed) and the squares it can stop on: one that holds an own
  unit with the task: the unit goes there (task 0Ch) and that one's
  score rises; one that holds an own unit without a task: the same, and
  that one gets the task, the score and the square.  Else the first
  unit that holds others, has a type with flags 80h..200h, less than
  the score, not the tasks 9, 0Ah, 0Bh, 0Dh, and may take the unit in,
  gets the task 0Dh with the unit's square if it can go there.

--handout: T17C0:0004 (`-break LT17C0_0004#N`, the end `-break
LT17C0_09C2#N`), by the record's +3; it returns 1 after each command it
puts into the queue (T1938:0388), and the stage is 2 until that is
carried out:

    0  the own units' +2 = FFFFh (not handled yet).
    1  the units in the own buildings: in the move phase the task 5
       (score 7), in the attack phase the task 6 for those that are not
       whole, in a building with more than 2 of energy.
    2, 3  the units with a task, in their order: task 3: the command 1
       (the unit to its aim's square); 0Ch: T1B01:0712; 0Eh: the command
       2 (fire at the unit on the aim's square).
    4, 5  then the unit not handled with the highest score: tasks 1 and
       0Dh, and 9, 0Ah, 0Bh (which keep the task): T1B01:0712; 2: the
       command 2; 5: the command 4 (out of the building); 6: the command
       6 (repair); 7: T1B01:0007.
    6  in the attack phase, for each free slot of an own factory with
       more than 9 of energy: the first type that can be made for the
       energy less 9 with the .COM flag 400h and a random number above
       500 of 1000: the command 7 (at most ten).
    7  the command 3 (the change of phase).
    8  the stage is 20h again.

  T1B01:0712, a unit towards its aim: a path over the whole map
  (can_go, first without the squares of enemy units), the unit's reach
  and the squares it can stop on; the square of the path nearest to the
  aim that is one of those (no building's, no unit that holds others,
  unless it is the aim): the command 1.  T1B01:0007, a unit to a unit
  (task 7): the first free square in reach around that unit, tried from
  the way that unit faces: the command 1, and for the attack phase the
  task 2 with an enemy beside that square the type can fire at; no
  square: the task 1 with the unit's square.

--carry: one call of T1938:012F (`-break command_step#N`, the end `-break
LT1938_0374#N`): a step (the queue's +47h) of the command that runs (its
+46h).  A step either waits, or puts keys into the player's script
(F2C0A:11AE + 20h * player, ended by FFh), which T1938:000F then plays,
one byte a pass; computer_step sets the script's place to its start
before.  A script byte: 1 up, 2 down, 3 left, 4 right, 5 fire; 80h..83h
fire with up, down, left, right for one pass and a pass of nothing.
Only one call is done: between two the map's loop moves the cursor.

    go to a square (T1938:1245, its steps in +48h): when the cursor is
       in a building's screen or less than 5 columns and rows away:
       steered there, a step a call (left or right, and up or down,
       both in one call; T1938:157E).  Else by the overview: fire with
       right, the overview's window moved until the square is in its
       middle (T1938:140D: column - 5 and row - 4, held within the map
       and made even), fire, then steered.
    1  a unit to a square: wait while a move runs; go to the unit; fire
       with up; steer to the square; fire (the choice is given up, bit
       8 of the player's record, when the square is not in reach or
       stop_check has a message); a call of nothing; fire.
    2  a unit fires at a unit: go to the unit; fire with up; steer to
       the target; fire, and if the target's square is not marked a
       target: given up, and fire once more.
    3  the change of phase: go to the own headquarters, steer to the
       square a row below (two bytes a square: width * 2 on), fire with
       left when no move runs, then wait for bit 4 of the stage's word.
    4  a unit out of its building: the record that holds it (the
       factories, the depots, the units that hold others, the
       headquarters: T1938:16C1; none: the command is over); go to its
       square; fire with left; up or down to the unit's slot; wait
       while a move runs; fire with up; the squares the unit may stop
       on (list_reach, into the list of aims): steer to the first (none:
       the building's own square, and given up); fire; fire; fire with
       right.
    5  the cursor to a square (no caller hands it out as far as read).
    6  a repair: as 4 to the slot, then fire with down, fire with right.
    7  a type made in a factory: go to the factory; fire with left; to
       the last free slot; fire with left; down to the type's line of
       the list (list_makeable's, as the screen made it); fire with
       left; fire with right.

--script: one call of T1938:000F (`-break script_step#N`, the end
`-break LT1938_0128#N`).

Checked against runs: docs/HANDOFF.md says which.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fight
import moves
import turn

FAR, AI, ORDER = 0x27EE, 0x2C0A, 0x2D33
NONE = turn.NONE
s8 = fight.signed


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


class Mem(turn.Mem):
    def __init__(self, path, load):
        turn.Mem.__init__(self, path, load)
        self.A = (load + AI) * 16

    def type_of(self, n):                       # as moves.Ram
        return self.utype(n)

    def state(self, player):
        return self.A + 0xA2 + 0x0C * player

    def table(self, player):
        """the units' records of the phase the player is in"""
        return self.A + (0x934 if self.b(self.F + turn.CURSORS + 0x31 * player + 0x16) == 2 else 0xBA)

    def com(self, n):
        """the .COM record of unit n's type"""
        return self.A + 6 * self.b(self.unit(n) + 8)

    def aims(self, player):
        return self.ptr(self.F + turn.PLAYERS + 0x17 * player + 9)

    def paths(self, player):
        return self.ptr(self.F + turn.PLAYERS + 0x17 * player + 0x0D)

    def hq_square(self, player):
        return self.w(self.F + turn.HQS + 0x1C * player + 0x0E)


def same_side(a, b):
    return fight.same_side(a, b)


def strength(mem, n):
    """T1ABC:0008 -> the three numbers, written to F27EE:418C as the game"""
    U, t = mem.unit(n), mem.utype(n)
    count = mem.b(U + (3 if mem.w(t + 0x0E) & 4 else 2))
    a, e, arm = (count >> 1) + 1, mem.b(U + 1) + 1, mem.b(t + 1) >> 3
    out = []
    for i, off in enumerate((0x0A, 0x0B, 9)):
        v = fight.s32(mem.b(t + off) * a * e * arm) >> 5
        mem.sw(mem.F + 0x418C + 2 * i, v)
        out.append(s16(v & 0xFFFF))
    return out


def find_kind(mem, mask, side):
    """T1ABC:01D2 -> the first unit of the side whose type has one of the
    .COM flags, FFh none"""
    for n in range(0xF1):
        f = mem.w(mem.unit(n) + 4)
        if same_side(f, side) and not f & 0x8000 and mem.w(mem.com(n) + 2) & mask:
            return n
    return 0xFF


def can_go(mem, sq, n, player, flags):
    """T1ABC:027D: a path for unit n to the square over the whole map:
    every square whose ground the type can be on and that holds no unit
    (with flag 1: no unit of the other side), or an own unit that holds
    others.  The path is left in the player's list, its length in
    F27EE:0B62."""
    F = mem.F
    squares = mem.squares(player)
    bit = 2 if player else 1
    w, h = mem.size()
    mask = mem.b(mem.utype(n) + 4)
    for s in range(0, mem.w(F + 0x246C), 2):
        ok = bool(mem.b(mem.ground(squares, s) + 2) & mask)
        there = mem.b(squares + s + 1)
        if there <= NONE:
            f = mem.w(mem.unit(there) + 4)
            if flags & 1 and not same_side(player, f):
                ok = False
            if same_side(f, player) and f & 0x1000:
                ok = True
        if ok:
            m = mem.mark(s)
            mem.sb(m, mem.b(m) | bit)
    start = mem.w(mem.unit(n) + 0x0B + 2 * player) - 1
    path = moves.find_path(mem, n, start, sq, player, squares)
    lst = mem.paths(player) + 0x1F40
    for i, s in enumerate(path or []):
        mem.sw(lst + 2 * i, s)
    mem.sw(F + 0x0B62, len(path or []))
    for i in range(0x1104):
        mem.m[F + turn.MARKS + i] &= ~bit & 0xFF
    return bool(path)


def buildings_list(mem, table, player, count):
    """T1B01:0B4F: the squares of the side's buildings with more than 2
    of energy, onto the list of aims"""
    F = mem.F
    lst = mem.aims(player)
    for k in range(count):
        B = F + table + 0x1C * k
        f = mem.w(B + 0x19)
        if f & 0x8002 or not same_side(f, player) or mem.b(B + 0x16 + player) <= 2:
            continue
        n = mem.w(F + 0x0B62)
        mem.sw(lst + 2 * n, mem.w(B + 0x0E))
        mem.sw(F + 0x0B62, n + 1)


def take_home(mem, player, n):
    """T1B01:0E4B: task 0Ah, the aim the nearest own factory in reach or
    the own headquarters"""
    F = mem.F
    w, h = mem.size()
    best, which = 1000, -1
    for k in range(10):
        B = F + turn.FACTORIES + 0x1C * k
        f = mem.w(B + 0x19)
        if f & 0x8000 or not same_side(f, player):
            continue
        sq = mem.w(B + 0x0E)
        if not can_go(mem, sq, n, player, 1):
            continue
        # as the code: the offsets are not halved here
        p = mem.w(mem.unit(n) + 0x0B + 2 * player) - 1
        d = moves.square_distance(s16(sq) % w, s16(sq) // w, s16(p) % w, s16(p) // w)
        if d < best:
            best, which = d, k
    sq = mem.w(F + turn.FACTORIES + 0x1C * which + 0x0E) if which >= 0 else mem.hq_square(player)
    r = mem.table(player) + 9 * n
    mem.sb(r + 8, 0x0A)
    mem.sw(r + 4, 0x190)
    mem.sw(r, sq)
    mem.sb(mem.A + 0x93C + 9 * n, 0)
    mem.sw(mem.A + 0x938 + 9 * n, 0xFFFF)


def add_aim(mem, player, score, where, kind):
    S = mem.state(player)
    e = mem.aims(player) + 6 * mem.b(S + 6)
    mem.sw(e, score)
    mem.sb(e + 4, kind)
    mem.sb(e + 5, kind)
    mem.sw(e + 2, where)
    mem.sb(S + 6, mem.b(S + 6) + 1)


def assess(mem, player, say):
    """T17C0:0A0D -> what it returns"""
    F, A = mem.F, mem.A
    S = mem.state(player)
    T = mem.table(player)
    squares = mem.squares(player)
    other = 1 - player
    w, h = mem.size()
    step = mem.b(S + 5)
    say('step %d' % step)
    if step == 0:
        mem.sb(S + 7, 0)
        if mem.b(F + turn.CURSORS + 0x31 * player + 0x16) == 2:
            mem.sb(S + 5, 5)
            return 0
        own = enemy = 0
        for n in range(0xF1):
            f = mem.w(mem.unit(n) + 4)
            if f & 0x8002:
                continue
            total = sum(strength(mem, n))
            r = T + 9 * n
            if not same_side(player, f):
                mem.sw(r + 6, 0xFFFF)
                mem.sw(r + 2, 0xFFFF)
                enemy += total
                continue
            own += total
            task = mem.b(r + 8)
            if task == 0x0B:
                continue
            if task == 9:
                mem.sw(F + 0x0B62, 0)
                buildings_list(mem, turn.HQS, player, 2)
                buildings_list(mem, turn.FACTORIES, player, 10)
                buildings_list(mem, turn.DEPOTS, player, 10)
                lst = mem.aims(player)
                k, found = mem.w(F + 0x0B62), False
                while k > 0 and not found:
                    k -= 1
                    found = can_go(mem, mem.w(lst + 2 * k), n, player, 0)
                if found:
                    continue
            if f & 0x1000:
                c = mem.cargo(mem.b(mem.unit(n) + 0x0A))
                if any(mem.b(c + 7 * player + i) <= NONE and mem.w(mem.unit(mem.b(c + 7 * player + i)) + 6) & 4
                       for i in range(7)):
                    take_home(mem, player, n)
                    continue
            mem.sw(r + 4, 0xFFFF)
            mem.sb(r + 8, 0)
            mem.sw(r, 0)
            mem.sw(A + 0x938 + 9 * n, 0xFFFF)
            mem.sb(A + 0x93C + 9 * n, 0)
            mem.sw(A + 0x934 + 9 * n, 0)
        mem.sw(S + 9, 3 if 2 * own <= enemy else 2 if own < enemy else 1)
        say('strength %d against %d: %d' % (own, enemy, mem.w(S + 9)))
    elif step == 1:
        hq = mem.hq_square(player) >> 1
        hx, hy = hq % w, hq // w
        for n in range(0xF1):
            U = mem.unit(n)
            f = mem.w(U + 4)
            if f & 0x40 and not f & 0x80 or not same_side(other, f) or f & 0x8000:
                continue
            t = mem.utype(n)
            v = s8(mem.b(mem.com(n)))
            if mem.w(t + 5) & 0xC0:
                v += 50
            v += 25 * sum(1 for off in (9, 0x0B, 0x0A) if mem.b(t + off) == 0)
            v += (6 - mem.b(U + 1)) * 10
            if mem.w(mem.com(n) + 2) & 1:
                p = s16(mem.w(U + 0x0B + 2 * player) - 1) >> 1
                if f & 0x4000:
                    holder = mem.b(mem.cargo(mem.b(U + 0x0A)) + 0x1B)
                    move = mem.b(mem.utype(holder))
                else:
                    move = mem.b(t)
                if moves.square_distance(hx, hy, p % w, p // w) < move:
                    v += 300
                    mem.sw(S + 9, mem.w(S + 9) + 2)
                    # as the code: the square's number, not its offset
                    for q in fight.neighbours(mem, p & 0xFFFF):
                        if q & 0x8000:
                            continue
                        there = mem.b(squares + q + 1)
                        if there <= NONE and same_side(other, mem.w(mem.unit(there) + 4)):
                            r = T + 9 * there + 6
                            mem.sw(r, mem.w(r) + 300)
            mem.sw(T + 9 * n + 6, mem.w(T + 9 * n + 6) + v)
    elif step == 2:
        if find_kind(mem, 1, player) <= NONE:
            add_aim(mem, player, mem.b(F + turn.HQS + 0x1C * other + 0x18) * 10, mem.hq_square(other), 1)
            for table in (turn.FACTORIES, turn.DEPOTS):
                for k in range(10):
                    B = F + table + 0x1C * k
                    f = mem.w(B + 0x19)
                    if f & 0x8000 or same_side(f, player):
                        continue
                    v = mem.b(B + 0x18) * 10
                    v += 20 * sum(1 for i in range(7) if mem.b(B + 7 * player + i) <= NONE)
                    if table == turn.FACTORIES and mem.b(B + 0x16 + player) >= 10:
                        v += (mem.b(B + 0x16 + player) // 10) * 20
                    add_aim(mem, player, v, mem.w(B + 0x0E), 1)
    elif step == 3:
        if find_kind(mem, 1, other) <= NONE:
            around = fight.neighbours(mem, mem.hq_square(player))
            for i, q in enumerate(around):
                mem.sw(F + 0x41A2 + 2 * i, q)
            if s16(mem.w(S + 9)) > 6:
                mem.sw(S + 9, 6)
            if mem.b(F + turn.PLAYERS + 0x17 * player + 2) < 10 and s16(mem.w(S + 9)) > 1:
                mem.sw(S + 9, 1)
            order = (mem.load + ORDER) * 16
            k, i = mem.b(S + 9), 0
            while k > 0:
                add_aim(mem, player, 0x78 + 0x1E * k, around[s8(mem.b(order + i))], 3)
                i += 1
                k -= 1
    elif step == 4:
        if find_kind(mem, 0x280, player) <= NONE:
            for n in range(0xF1):
                U = mem.unit(n)
                if not mem.w(U + 4) & 0x8000 and mem.w(U + 6) & 4 and mem.w(U + 4) & 2:
                    add_aim(mem, player, 0xC8, n, 4)
    else:
        mem.sb(S + 5, 0)
        return 1
    mem.sb(S + 5, mem.b(S + 5) + 1)
    return 0


def set_marks(mem, buf, bit):
    for i in range(moves.SIZE):
        if s8(buf[i]) >= 0:
            mem.m[mem.F + turn.MARKS + i] |= bit


def clear_marks(mem, bit):
    for i in range(moves.SIZE):
        mem.m[mem.F + turn.MARKS + i] &= ~bit & 0xFF


def unit_reach(mem, n, player, plain=False):
    """reach for unit n with all its points, as T1B01 calls it"""
    U = mem.unit(n)
    f = mem.w(U + 4)
    flags = 0xFFFF if mem.w(U + 6) & 1 and not plain else 6 if f & 1 else 3
    buf = moves.reach(mem, mem.w(U + 0x0B + 2 * player) - 1, n, mem.b(U), player, flags, mem.squares(player))
    set_marks(mem, buf, 2 if player else 1)


def list_reach(mem, player, n, lst=None):
    """T0BA0:1391: the squares in reach on which the unit may stop, into
    the player's list (or the list given), counted in F27EE:0B62"""
    F = mem.F
    w, h = mem.size()
    squares = mem.squares(player)
    bit = 2 if player else 1
    if lst is None:
        lst = mem.paths(player)
    count = 0
    mem.sb(F + turn.REC, n)
    for x in range(w):
        for y in range(h):
            if not mem.b(F + turn.MARKS + x + 64 * y) & bit:
                continue
            sq = 2 * (x + w * y)
            if turn.stop_check(mem, sq, player, squares) == 0:
                mem.sw(lst + 2 * count, sq)
                count += 1
                if count >= 0x316:
                    mem.sw(F + 0x0B62, count)
                    return
    mem.sw(F + 0x0B62, count)


def command_add(mem, player, kind, b1=None, w3=None, b3=None):
    """T1938:0388 with the record F2C0A:11F6 as the callers fill it"""
    A = mem.A
    c = A + 0x11F6
    mem.sb(c, kind)
    if b1 is not None:
        mem.sb(c + 1, b1)
    if w3 is not None:
        mem.sw(c + 3, w3)
    if b3 is not None:
        mem.sb(c + 3, b3)
    Q = A + 0x11FE + 0x49 * player
    n = mem.b(Q + 0x46)
    if n >= 10:
        return
    if n == 0:
        slot, end = 0, 1
        mem.sb(Q + 0x46, 1)
    else:
        slot, end = n, n + 1
        mem.sb(Q + 0x46, n + 1)
    mem.m[Q + 7 * slot:Q + 7 * slot + 7] = mem.m[c:c + 7]
    mem.sb(Q + 7 * end, 0xFF)


def first_half(mem, n):
    f = mem.w(mem.unit(n) + 4)
    return (n - 1) & 0xFF if f & 0x40 and not f & 0x80 else n


def move_toward(mem, n, player):
    """T1B01:0712 -> 1 when it is through (a command to move may be added)"""
    F = mem.F
    S, T = mem.state(player), mem.table(player)
    squares = mem.squares(player)
    bit = 2 if player else 1
    rec = T + 9 * n                              # the record of the unit as given
    n = first_half(mem, n)
    step = mem.b(S + 7)
    if step == 0:
        if not mem.w(F + 0x2510) & 0x80:
            mem.sb(S + 7, 1)
    elif step == 1:
        mem.sb(S + 7, 3 if can_go(mem, mem.w(rec), n, player, 1) else 2)
    elif step == 2:
        can_go(mem, mem.w(rec), n, player, 0)
        mem.sb(S + 7, 3)
    elif step == 3:
        count = mem.w(F + 0x0B62)
        src, dst = mem.paths(player) + 0x1F40, mem.aims(player)
        mem.sw((mem.load + ORDER) * 16 + 0x14, count)
        mem.m[dst:dst + 2 * count] = mem.m[src:src + 2 * count]
        mem.sb(S + 7, 4)
    elif step == 4:
        unit_reach(mem, n, player)
        mem.sb(S + 7, 5)
    elif step == 5:
        list_reach(mem, player, n)
        clear_marks(mem, bit)
        mem.sb(S + 7, 6)
    elif step == 6:
        mem.sb(S + 7, 0)
        path, stops = mem.aims(player), mem.paths(player)
        for d in range(s16(mem.w((mem.load + ORDER) * 16 + 0x14))):
            for i in range(s16(mem.w(F + 0x0B62))):
                sq = mem.w(stops + 2 * i)
                if sq != mem.w(path + 2 * d):
                    continue
                ok = True
                if mem.w(rec) != sq:
                    if mem.w(mem.ground(squares, sq)) & 0x540:
                        ok = False
                    there = mem.b(squares + sq + 1)
                    if there <= NONE and mem.w(mem.unit(there) + 4) & 0x1000:
                        ok = False
                if ok:
                    command_add(mem, player, 1, n, sq)
                    return 1
        return 1
    return 0


def approach(mem, n, player):
    """T1B01:0007 -> 1 when it is through: to a square beside the unit
    that is the aim"""
    F, A = mem.F, mem.A
    S, T = mem.state(player), mem.table(player)
    squares = mem.squares(player)
    bit = 2 if player else 1
    rec = T + 9 * n
    n = first_half(mem, n)
    step = mem.b(S + 7)
    if step == 0:
        if not mem.w(F + 0x2510) & 0x80:
            mem.sb(S + 7, 1)
        return 0
    if step == 1:
        unit_reach(mem, n, player, plain=True)
        mem.sb(S + 7, 2)
        return 0
    if step != 2:
        return 0
    aim = mem.w(rec)
    pos = mem.w(mem.unit(aim) + 0x0B + 2 * player) - 1
    way = mem.b(mem.unit(aim) + 0x0F + player)
    turns = (mem.load + 0x2D35) * 16 + 0x0E
    around = fight.neighbours(mem, pos & 0xFFFF)
    found = None
    for i in range(6):
        d = (way + mem.b(turns + i)) % 6
        sq = around[d]
        if sq == 0xFFFF or not mem.b(mem.mark(sq)) & bit:
            continue
        free = mem.b(squares + sq + 1) > NONE
        if mem.w(mem.unit(n) + 4) & 0x10 and mem.w(mem.ground(squares, sq)) & 0x4000:
            free = False
        if free:
            found = sq
            break
    if found is not None:
        sq = found
        command_add(mem, player, 1, n, sq)
        mem.sb(rec + 8, 0)
        mem.sw(rec + 4, 0xFFFF)
        mem.sw(rec + 2, 1)
        around = fight.neighbours(mem, sq)
        targets = mem.w(mem.utype(n) + 5) & 0x3C
        for i in range(6):
            sq2 = around[(way + mem.b(turns + i)) % 6]
            if sq2 == 0xFFFF:
                continue
            there = mem.b(squares + sq2 + 1)
            if there > NONE:
                continue
            f2 = mem.w(mem.unit(there) + 4)
            if not same_side(player, f2) and targets & f2:
                mem.sb(A + 0x93C + 9 * n, 2)
                mem.sw(A + 0x934 + 9 * n, sq2)
                mem.sw(A + 0x938 + 9 * n, mem.w(T + 9 * there + 6))
    else:
        mem.sb(rec + 8, 1)
        mem.sw(rec, pos)
    clear_marks(mem, bit)
    mem.sb(S + 7, 0)
    return 1


def rand(mem):
    a = (mem.load + 0x2D8D) * 16 + 0x1938
    state = (struct.unpack_from('<L', mem.m, a)[0] * 0x015A4E35 + 1) & 0xFFFFFFFF
    struct.pack_into('<L', mem.m, a, state)
    return (state >> 16) & 0x7FFF


def list_makeable(mem, energy):
    """T1479:11B4 -> the count (F27EE:1338), the types at F27EE:4136"""
    F = mem.F
    count = 0
    for t in range(0x1B):
        mem.sb(F + 0x4136 + t, 0xFF)
    mem.sb(F + 0x1338, 0)
    if not any(mem.w(mem.unit(n) + 4) & 0x8000 for n in range(0xF1)):
        return 0                                # no unit record is free
    for t in range(0x1B):
        r = F + turn.TYPES + 0x44 * t
        if mem.b(r + 0x3E) > energy or mem.w(r + 0x0E) & 2:
            continue
        if mem.w(r + 0x0C) & 0x1000 and not any(
                mem.w(F + turn.CARGO + 0x1C * k + 0x19) & 0x8000 for k in range(0x46)):
            continue
        mem.sb(F + 0x4136 + count, t)
        count += 1
    mem.sb(F + 0x1338, count)
    return count


def hand_out(mem, player, say):
    """T17C0:0004 -> what it returns: 1 when a command was added"""
    F, A = mem.F, mem.A
    S, T = mem.state(player), mem.table(player)
    squares = mem.squares(player)
    mode = mem.b(F + turn.CURSORS + 0x31 * player + 0x16)
    step = mem.b(S + 3)
    say('step %d' % step)

    def own(n):
        f = mem.w(mem.unit(n) + 4)
        return not f & 0x8000 and same_side(f, player)

    def done(n, keep_score=False):
        mem.sb(T + 9 * n + 8, 0)
        if not keep_score:
            mem.sw(T + 9 * n + 4, 0xFFFF)
        mem.sw(T + 9 * n + 2, 1)

    def fire(n, back):
        there = mem.b(squares + mem.w(T + 9 * n) + 1)
        done(n)
        mem.sb(S + 3, back)
        if there <= NONE and not mem.w(mem.unit(there) + 4) & 0x8000:
            command_add(mem, player, 2, n & 0xFF, b3=there)
            return 1
        return 0

    if step == 0:
        for n in range(0xF1):
            if own(n):
                mem.sw(T + 9 * n + 2, 0xFFFF)
        mem.sb(S + 7, 0)
        mem.sw(S + 9, 0)
        mem.sb(S + 3, 1)
    elif step == 1:
        for table, count in ((turn.DEPOTS, 10), (turn.FACTORIES, 10), (turn.HQS, 2)):
            for k in range(count):
                B = F + table + 0x1C * k
                f = mem.w(B + 0x19)
                if f & 0x8002 or not same_side(f, player):
                    continue
                if mode != 1 and mem.b(B + 0x16 + player) <= 2:
                    continue
                for i in range(7):
                    v = mem.b(B + 7 * player + i)
                    if v > NONE or mem.w(mem.unit(v) + 6) & 0x24:
                        continue
                    if mode == 1:
                        mem.sb(A + 0xC2 + 9 * v, 5)
                        mem.sw(A + 0xBE + 9 * v, 7)
                    elif mem.b(mem.unit(v) + 2) < mem.b(mem.utype(v) + 2):
                        mem.sb(A + 0x93C + 9 * v, 6)
                        mem.sw(A + 0x938 + 9 * v, 7)
        mem.sb(S + 3, 2)
    elif step == 2:
        n = mem.b(S + 9)
        while n <= NONE:
            if own(n) and mem.b(T + 9 * n + 8) != 0:
                mem.sw(S + 9, n)
                break
            n += 1
        mem.sb(S + 3, 3 if n <= NONE else 4)
    elif step == 3:
        n = mem.w(S + 9)
        task = mem.b(T + 9 * n + 8)
        if task == 3:
            command_add(mem, player, 1, n & 0xFF, mem.w(T + 9 * n))
            done(n, keep_score=True)
            mem.sb(S + 3, 2)
            return 1
        if task == 4:
            done(n, keep_score=True)
            mem.sb(S + 3, 2)
        elif task == 0x0C:
            if move_toward(mem, n & 0xFF, player):
                done(n)
                mem.sb(S + 3, 2)
                return 1
        elif task == 0x0E:
            return fire(n, 2)
        else:
            mem.sb(S + 3, 2)
            mem.sw(S + 9, n + 1)
    elif step == 4:
        best = -1
        mem.sw(S + 9, 0xFFFF)
        for n in range(0xF1):
            r = T + 9 * n
            if s16(mem.w(r + 4)) > best and not mem.w(mem.unit(n) + 4) & 0x8200 and own(n) \
                    and mem.b(r + 8) != 0 and s16(mem.w(r + 2)) < 0:
                best = s16(mem.w(r + 4))
                mem.sw(S + 9, n)
        mem.sb(S + 3, 5 if s16(mem.w(S + 9)) >= 0 else 6)
    elif step == 5:
        n = mem.w(S + 9)
        task = mem.b(T + 9 * n + 8)
        if task in (1, 0x0D):
            if move_toward(mem, n & 0xFF, player):
                done(n)
                mem.sb(S + 3, 4)
                return 1
        elif task == 2:
            return fire(n, 4)
        elif task in (5, 6):
            command_add(mem, player, 4 if task == 5 else 6, n & 0xFF)
            done(n)
            mem.sb(S + 3, 4)
            return 1
        elif task == 7:
            if approach(mem, n & 0xFF, player):
                mem.sb(S + 3, 4)
                return 1
        elif task in (9, 0x0A, 0x0B):
            if move_toward(mem, n & 0xFF, player):
                mem.sw(T + 9 * n + 2, 1)
                mem.sb(S + 3, 4)
                return 1
        else:
            done(n)
            mem.sb(S + 3, 4)
    elif step == 6:
        if mode != 1:
            made = 0
            for k in range(10):
                B = F + turn.FACTORIES + 0x1C * k
                f = mem.w(B + 0x19)
                energy = mem.b(B + 0x16 + player)
                if f & 0x8002 or not same_side(f, player) or energy <= 9:
                    continue
                for i in range(7):
                    if mem.b(B + 7 * player + i) <= NONE:
                        continue
                    if energy - 9 <= 0:
                        break
                    count = list_makeable(mem, (energy - 9) & 0xFF)
                    if count == 0 or made >= 10:
                        break
                    pick = 0xFF
                    for j in range(count):
                        t = mem.b(F + 0x4136 + j)
                        if mem.w(A + 6 * t + 2) & 0x400 and rand(mem) % 1001 > 500:
                            pick = j
                            break
                    if pick != 0xFF:
                        # as the code: the cost of the type numbered as the list's place
                        energy -= mem.b(F + turn.TYPES + 0x44 * pick + 0x3E)
                        made += 1
                        command_add(mem, player, 7, pick, b3=k)
        mem.sb(S + 3, 7)
    elif step == 7:
        command_add(mem, player, 3)
        mem.sb(S + 3, 8)
        return 1
    else:
        mem.sw(S, mem.w(S) & 0xFFF7 | 0x20)
        mem.sb(S + 3, 0)
    return 0


def fire_marks(mem, n, player):
    """the unit's targets marked (4, or 8 for player 1), as T0D36 and
    T1C04 call fire_reach for the type's two ranges"""
    t = mem.utype(n)
    pos = mem.w(mem.unit(n) + 0x0B + 2 * player) - 1
    bit = 8 if player else 4
    for rng, mask in ((mem.b(t + 8), 0xFFEF), (mem.b(t + 7), 0xFFD3)):
        if rng > 1:
            buf = moves.fire_reach(mem, pos, rng, player, n, mem.w(t + 5) & mask, mem.squares(player))
            for i in range(moves.SIZE):
                if buf[i] == 1:
                    mem.m[mem.F + turn.MARKS + i] |= bit


def crowded(mem, sq, player):
    """T1B01:0C27 -> 1 when more than three of the six squares around
    hold units of the other side, or two opposite ones do"""
    squares = mem.squares(player)
    other = 1 - player
    held = []
    for q in fight.neighbours(mem, sq & 0xFFFF):
        there = mem.b(squares + q + 1) if q != 0xFFFF else 0xFF
        held.append(q != 0xFFFF and there <= NONE and same_side(other, mem.w(mem.unit(there) + 4)))
    return int(sum(held) > 3 or any(held[i] and held[(i + 3) % 6] for i in range(6)))


def give(mem, n, score, aim, task):
    """a unit's record of the phase, and its attack record cleared"""
    r = mem.table_now + 9 * n
    mem.sw(r + 4, score)
    mem.sw(r, aim)
    mem.sb(r + 8, task)
    mem.sb(mem.A + 0x93C + 9 * n, 0)
    mem.sw(mem.A + 0x938 + 9 * n, 0xFFFF)


def helper(mem, player, n, score, task, square):
    """T1ED2:0005 -> 1 when it is through: a unit that cannot go by
    itself (one with 4 in its +6 is taken at once) goes to a unit with
    the task beside its reach, or one without a task there gets the
    task, or a unit that holds others comes for it"""
    F, A = mem.F, mem.A
    S, T = mem.state(player), mem.table(player)
    mem.table_now = T
    squares = mem.squares(player)
    bit = 2 if player else 1
    G = (mem.load + 0x2D37) * 16
    step = mem.b(S + 8)
    U = mem.unit(n)
    if step == 0:
        mem.sw(G + 6, 0)
        mem.sb(S + 8, 5 if mem.w(U + 6) & 4 else 1)
    elif step == 1:
        buf = moves.reach(mem, mem.w(U + 0x0B + 2 * player) - 1, n, mem.b(U), player, 0, squares)
        set_marks(mem, buf, bit)
        mem.sb(S + 8, 2)
    elif step == 2:
        list_reach(mem, player, n)
        clear_marks(mem, bit)
        mem.sb(S + 8, 3)
    elif step in (3, 4):
        lst = mem.paths(player)
        for i in range(s16(mem.w(F + 0x0B62))):
            sq = mem.w(lst + 2 * i)
            there = mem.b(squares + sq + 1)
            if there > NONE or mem.b(T + 9 * there + 8) != (task if step == 3 else 0):
                continue
            give(mem, n, score, sq, 0x0C)
            if step == 3:
                mem.sw(T + 9 * there + 4, mem.w(T + 9 * there + 4) + score)
            else:
                give(mem, there, score, square, task)
            mem.sb(S + 8, 0)
            return 1
        mem.sb(S + 8, step + 1)
    elif step == 5:
        w, h = mem.size()
        k = mem.b(G + 6)
        while k <= NONE:
            f = mem.w(mem.unit(k) + 4)
            r = T + 9 * k
            if not f & 0xC000 and f & 0x1000 and s16(mem.w(r + 4)) < s16(score) and same_side(player, f) \
                    and mem.w(mem.com(k) + 2) & 0x380 and mem.b(r + 8) not in (9, 0x0A, 0x0D, 0x0B) \
                    and mem.w(U + 6) & mem.w(mem.cargo(mem.b(mem.unit(k) + 0x0A)) + 0x19) & 0x0FC0:
                a = s16(mem.w(U + 0x0B + 2 * player) - 1) >> 1
                b = s16(mem.w(mem.unit(k) + 0x0B + 2 * player) - 1) >> 1
                if moves.square_distance(a % w, a // w, b % w, b // w) < 0x3E8:
                    mem.sw(G + 6, k)
                    break
            k += 1
        mem.sb(S + 8, 6 if k <= NONE else 7)
    elif step == 6:
        sq = mem.w(U + 0x0B + 2 * player) - 1
        k = mem.w(G + 6)
        if can_go(mem, sq, k & 0xFF, player, 0):
            give(mem, k, score, sq, 0x0D)
            mem.sb(S + 8, 7)
        else:
            mem.sb(S + 8, 5)
            mem.sw(G + 6, k + 1)
    else:
        mem.sb(S + 8, 0)
        return 1
    return 0


def plan(mem, player, say):
    """T1C04:000D -> what it returns: 1 when the stage is through"""
    F, A = mem.F, mem.A
    S, T = mem.state(player), mem.table(player)
    mem.table_now = T
    L = mem.aims(player)
    squares = mem.squares(player)
    other = 1 - player
    w, h = mem.size()
    G = (mem.load + 0x2D36) * 16
    step, sub = mem.b(S + 4), mem.b(S + 7)
    say('step %X.%X' % (step, sub))
    idx = s16(mem.w(G + 0x0A))
    aim = L + 6 * idx                   # the aim worked on: score, +2 where, +5 kind

    def f(n):
        return mem.w(mem.unit(n) + 4)

    def half(n):
        return f(n) & 0x40 and not f(n) & 0x80

    def com(n):
        return mem.w(mem.com(n) + 2)

    def rec(n):
        return T + 9 * n

    def score(n):
        return s16(mem.w(rec(n) + 4))

    def pos(n):
        return (mem.w(mem.unit(n) + 0x0B + 2 * player) - 1) & 0xFFFF

    def xy(sq):
        q = s16(sq) >> 1
        return q % w, q // w

    def dist(a, b):
        return moves.square_distance(*(xy(a) + xy(b)))

    def cur():
        return mem.b(G + 7)

    def next_unit(test, on_none, start=None):
        """the loop over the units from F2D36:0007 on"""
        n = cur() if start is None else start
        while n <= NONE:
            if test(n):
                break
            n += 1
        mem.sb(G + 7, n & 0xFF)
        mem.sb(S + 7, sub + 1 if n <= NONE else on_none)

    def path_length(sq):
        n = cur()
        mem.sw(rec(n) + 2, mem.w(F + 0x0B62) if can_go(mem, sq, n, player, 0) else 0x3E8)
        mem.sb(G + 7, n + 1)

    def back():
        mem.sb(S + 7, 0)
        mem.sb(S + 4, 1)
        mem.sb(aim + 5, 0)

    if step == 0:
        if mem.b(F + turn.CURSORS + 0x31 * player + 0x16) == 2:
            mem.sb(S + 4, 0x0C)
        else:
            mem.sb(S + 4, 1)
            mem.sw(G + 8, 1)
    elif step == 1:
        if mem.b(S + 6) == 0 or s16(mem.w(G + 8)) >= 6:
            mem.sb(S + 4, 7)
        else:
            best = 0
            mem.sw(G + 0x0A, 0xFFFF)
            for i in range(mem.b(S + 6)):
                if s16(mem.w(L + 6 * i)) > best and mem.b(L + 6 * i + 5) == mem.w(G + 8):
                    best = s16(mem.w(L + 6 * i))
                    mem.sw(G + 0x0A, i)
            i = s16(mem.w(G + 0x0A))
            if i >= 0:
                mem.sb(S + 4, {1: 3, 2: 4, 3: 5, 4: 6}.get(mem.b(L + 6 * i + 5), 2))
            else:
                mem.sw(G + 8, mem.w(G + 8) + 1)
    elif step == 2:
        mem.sb(S + 4, 1)
        mem.sb(aim + 5, 0)
    elif step == 3:                     # an aim of kind 1: a building to take
        want = s16(mem.w(aim))

        def can(n):
            return com(n) & 1 and not f(n) & 0xC040 and same_side(f(n), player) and score(n) < want
        if sub == 0:
            mem.sb(G + 7, 0)
            mem.sb(S + 7, 1)
        elif sub == 1:
            next_unit(can, 3)
        elif sub == 2:
            path_length(mem.w(aim + 2))
            mem.sb(S + 7, 1)
        elif sub == 3:
            best, who = 0x3E8, -1
            for n in range(0xF1):
                far = s16(mem.w(rec(n) + 2))
                if can(n) and far < best and want - far > score(n):
                    best, who = far, n
            if who >= 0:
                left = (want - best) & 0xFFFF
                give(mem, who, left, mem.w(aim + 2), 1)
                if com(who) & 0x20:
                    add_aim(mem, player, left - 10, who, 2)
            mem.sb(S + 7, 4)
        else:
            back()
    elif step == 4:                     # kind 2: units to go with a unit
        want = s16(mem.w(aim))
        whom = mem.w(aim + 2)

        def can(n):
            return not half(n) and not f(n) & 0xC000 and same_side(f(n), player) and score(n) < want \
                and com(n) & f(whom & 0xFF if False else whom) & 0x1C
        if sub == 0:
            mem.sb(G + 7, 0)
            mem.sb(S + 7, 1)
        elif sub == 1:
            next_unit(can, 3)
        elif sub == 2:
            path_length(pos(whom))
            mem.sb(S + 7, 1)
        elif sub == 3:
            whom &= 0xFF
            for k in range(mem.b(mem.com(whom) + 4)):
                best, who = 0x3E8, -1
                for n in range(0xF1):
                    if not half(n) and not f(n) & 0xC000 and same_side(f(n), player) \
                            and score(n) < s16(mem.w(aim)) and com(n) & f(whom) & 0x1C \
                            and s16(mem.w(rec(n) + 2)) < best:
                        best, who = s16(mem.w(rec(n) + 2)), n
                if who >= 0:
                    now = s16(mem.w(aim))
                    give(mem, who, now, whom, 7)
                    mem.sw(aim, now - fight.div(now, 3))
            mem.sb(S + 7, 4)
        else:
            back()
    elif step == 5:                     # kind 3: a square by the own headquarters
        want = s16(mem.w(aim))
        sq = mem.w(aim + 2)

        def can(n):
            return not half(n) and not f(n) & 0xC000 and same_side(f(n), player) and score(n) < want \
                and com(n) & 0x40
        if sub == 0:
            mem.sb(G + 7, 0)
            mem.sb(S + 7, 1)
        elif sub == 1:
            there = mem.b(squares + sq + 1)
            if there <= NONE and same_side(player, f(there)) and score(there) < want and com(there) & 0x40:
                give(mem, there, want, sq, 4)
                mem.sb(S + 7, 5)
            else:
                mem.sb(S + 7, 2)
        elif sub == 2:
            next_unit(can, 4)
        elif sub == 3:
            path_length(sq)
            mem.sb(S + 7, 2)
        elif sub == 4:
            best, who = 0x3E8, -1
            for n in range(0xF1):
                if can(n) and s16(mem.w(rec(n) + 2)) < best:
                    best, who = s16(mem.w(rec(n) + 2)), n
            if who >= 0:
                give(mem, who, want, sq, 1)
            mem.sb(S + 7, 5)
        else:
            back()
    elif step == 6:                     # kind 4: a unit of nobody to fetch
        whom = mem.w(aim + 2)
        if helper(mem, player, whom & 0xFF, mem.w(aim), 0x0A, mem.w(mem.unit(whom) + 0x0B + 2 * player)):
            mem.sb(S + 4, 1)
            mem.sb(S + 7, 0)
            mem.sb(aim + 5, 0)
    elif step == 7:                     # the weakened units go to a building
        if sub == 0:
            mem.sb(G + 7, 0)
            mem.sb(S + 7, 1)
        elif sub == 1:
            next_unit(lambda n: not f(n) & 0xC048 and same_side(f(n), player)
                      and mem.b(mem.unit(n) + 2) < mem.b(mem.utype(n) + 2), 5)
        elif sub == 2:
            mem.sw(F + 0x0B62, 0)
            buildings_list(mem, turn.HQS, player, 2)
            buildings_list(mem, turn.FACTORIES, player, 10)
            buildings_list(mem, turn.DEPOTS, player, 10)
            if mem.w(F + 0x0B62) == 0:
                mem.sb(S + 7, 1)
                mem.sb(G + 7, cur() + 1)
            else:
                mem.sw(G + 0x0C, mem.w(F + 0x0B62))
                mem.sb(S + 7, 3)
        elif sub == 3:
            if s16(mem.w(G + 0x0C)) > 0:
                mem.sw(G + 0x0C, mem.w(G + 0x0C) - 1)
                sq = mem.w(L + 2 * mem.w(G + 0x0C))
                n = cur()
                U = mem.unit(n)
                v = 0x46 + 10 * mem.b(U + 1) if mem.b(U + 2) < 5 and mem.b(U + 1) > 1 else 0x0F
                near = can_go(mem, sq, n, player, 1) and s16(mem.w(F + 0x0B62)) <= mem.b(mem.utype(n))
                if near:
                    v *= 2
                    if score(n) < v:
                        give(mem, n, v, sq, 1)
                    mem.sb(S + 7, 1)
                    mem.sb(G + 7, n + 1)
                elif score(n) < v:
                    mem.sw(G + 0x0E, v)
                    mem.sw(G + 0x10, sq)
                    mem.sb(S + 7, 4)
                else:
                    mem.sb(S + 7, 1)
                    mem.sb(G + 7, n + 1)
        elif sub == 4:
            n = cur()
            if com(n) & 0x380 or helper(mem, player, n, mem.w(G + 0x0E), 9, mem.w(G + 0x10)):
                mem.sb(S + 7, 1)
                mem.sb(G + 7, n + 1)
        else:
            mem.sb(S + 7, 0)
            mem.sb(S + 4, 8)
    elif step == 8:                     # what each enemy unit cannot fire at
        for n in range(0xF1):
            if half(n) or not same_side(other, f(n)) or f(n) & 0xC000:
                continue
            t = mem.utype(n)
            v = (1 if mem.w(t + 5) & 0xC0 else 0) | (2 if mem.b(t + 9) == 0 else 0) \
                | (4 if mem.b(t + 0x0B) == 0 else 0) | (8 if mem.b(t + 0x0A) == 0 else 0)
            mem.sw(rec(n) + 2, v)
        mem.sb(S + 4, 9)
    elif step == 9:                     # the units that fire from afar
        if sub == 0:
            mem.sb(G + 7, 0)
            mem.sb(S + 7, 1)
        elif sub == 1:
            next_unit(lambda n: not half(n) and same_side(player, f(n)) and not f(n) & 0xC200 and com(n) & 2, 4)
        elif sub == 2:
            clear_marks(mem, 8 if player else 4)
            fire_marks(mem, cur(), player)
            mem.sb(S + 7, 3)
        elif sub == 3:
            bit = 8 if player else 4
            best, where = -1, -1
            for x in range(w):
                for y in range(h):
                    if not mem.b(F + turn.MARKS + x + 64 * y) & bit:
                        continue
                    sq = 2 * (x + w * y)
                    threat = s16(mem.w(rec(mem.b(squares + sq + 1)) + 6))
                    if threat > best:
                        best, where = threat, sq
            n = cur()
            if where >= 0:
                mem.sb(rec(n) + 8, 4)
                mem.sw(rec(n), pos(n))
                mem.sw(rec(n) + 4, 0x64)
                mem.sb(A + 0x93C + 9 * n, 0x0E)
                mem.sw(A + 0x934 + 9 * n, where)
                mem.sw(A + 0x938 + 9 * n, 0x64)
            clear_marks(mem, bit)
            mem.sb(G + 7, n + 1)
            mem.sb(S + 7, 1)
        else:
            mem.sb(S + 7, 0)
            mem.sb(S + 4, 0x0A)
    elif step == 0x0A:                  # units around the enemy that threatens most
        foe = mem.b(G + 6)
        P = mem.paths(player)
        if sub == 0:
            mem.sb(G + 6, 0)
            mem.sb(S + 7, 1)
        elif sub == 1:
            best = -1
            mem.sb(G + 7, 0)
            for n in range(0xF1):
                if not half(n) and same_side(other, f(n)) and not f(n) & 0xC002 \
                        and s16(mem.w(rec(n) + 6)) > best and s16(mem.w(rec(n) + 2)) >= 0:
                    best = s16(mem.w(rec(n) + 6))
                    mem.sb(G + 6, n)
            if best >= 0:
                mem.sb(S + 7, 2)
                for i in range(6):
                    mem.sb(L + 0x78 + i, 0)
            else:
                mem.sb(S + 7, 0x0B)
        elif sub == 2:
            mem.sb(S + 7, 3)
        elif sub == 3:
            def can(n):
                t = mem.utype(n)
                return not half(n) and same_side(player, f(n)) and not f(n) & 0xC000 \
                    and score(n) < s16(mem.w(rec(foe) + 6)) and dist(pos(n), pos(foe)) < mem.b(t) >> 1 \
                    and mem.w(t + 5) & f(foe) & 0x3C and not mem.w(t + 5) & 0x40
            n = cur()
            while n <= NONE and not can(n):
                n += 1
            if n <= NONE:
                mem.sb(G + 7, n)
                mem.sb(S + 7, 4)
            else:
                mem.sb(S + 7, 6)
        elif sub == 4:
            unit_reach(mem, cur(), player)
            mem.sb(S + 7, 5)
        elif sub == 5:
            bit = 2 if player else 1
            n = cur()
            around = fight.neighbours(mem, pos(foe))
            for i in range(6):
                sq = around[i]
                if mem.b(L + 0x78 + i) >= 0x14 or sq & 0x8000 or crowded(mem, sq, player):
                    continue
                if pos(n) != sq:
                    if not mem.b(mem.mark(sq)) & bit or mem.b(squares + sq + 1) <= NONE:
                        continue
                    g = mem.w(mem.ground(squares, sq))
                    if g & 0x540 or f(n) & 0x10 and g & 0x4000:
                        continue
                    if any(mem.w(rec(k)) == sq for k in range(0xF1)):
                        continue
                mem.sb(L + 0x14 * i + mem.b(L + 0x78 + i), n)
                mem.sb(L + 0x78 + i, mem.b(L + 0x78 + i) + 1)
            clear_marks(mem, bit)
            mem.sb(G + 7, n + 1)
            mem.sb(S + 7, 2)
        elif sub == 6:
            free = 6
            around = fight.neighbours(mem, pos(foe))
            for i in range(6):
                sq = around[i]
                mem.sb(P + 0x28 + i, 0xFF)
                if sq & 0x8000:
                    mem.sw(P + 2 * i, 0xFFFF)
                    free -= 1
                    continue
                mem.sw(P + 0x14 + 2 * i, sq)
                mem.sw(P + 2 * i, 0)
                if crowded(mem, sq, player):
                    mem.sw(P + 2 * i, 0xFFFF)
                    free -= 1
                    continue
                for k in range(0xF1):
                    if not f(k) & 0xC000 and same_side(f(k), player) and mem.b(rec(k) + 8) != 0 \
                            and mem.w(rec(k)) == sq:
                        mem.sw(P + 2 * i, 1)
                        free -= 1
                        break
            if free > 0:
                mem.sb(S + 7, 7)
                mem.sw(G + 0x12, 0)
            else:
                mem.sw(G + 0x12, free)
                mem.sb(S + 7, 0x0A)
        elif sub in (7, 8):
            def chosen(u):                      # T1B01:0DE3
                return u in (mem.b(P + 0x28 + k) for k in range(6))

            def count(i):
                return mem.b(L + 0x78 + i)

            def got():
                return s16(mem.w(G + 0x12))

            def take(i, u, by=1):
                mem.sb(P + 0x28 + i, u)
                mem.sw(P + 2 * i, 1)
                mem.sb(L + 0x78 + i, 0)
                mem.sw(G + 0x12, mem.w(G + 0x12) + by)
            if sub == 7:
                weak = mem.w(rec(foe) + 2)
                for i in range(6):
                    if mem.w(P + 2 * i) != 0:
                        continue
                    j = 0
                    while count(i) > j and got() < 4:
                        u = mem.b(L + 0x14 * i + j)
                        fu = f(u)
                        if s16(mem.w(rec(foe) + 6)) > score(u) and not chosen(u) and (
                                weak & 1 or weak & 2 and fu & 0x10 or weak & 8 and fu & 4 or weak & 4 and fu & 8):
                            take(i, u)
                        j += 1
            else:
                for i in range(6):
                    o = (i + 3) % 6
                    if got() >= 4:
                        break
                    if mem.w(P + 2 * i) == 1 and mem.w(P + 2 * o) == 0:
                        j = 0
                        while count(o) > j:
                            u = mem.b(L + 0x14 * o + j)
                            if not chosen(u):
                                j = count(o)
                                take(o, u)
                            j += 1
                    elif mem.w(P + 2 * i) == 0 and mem.w(P + 2 * o) == 0 and count(i) > 0 and count(o) > 0:
                        a = b = 0xFF
                        for j in range(count(i)):
                            u = mem.b(L + 0x14 * i + j)
                            if not chosen(u):
                                a = u
                        for j in range(count(o)):
                            u = mem.b(L + 0x14 * o + j)
                            if not chosen(u) and u != a:
                                b = u
                        if a != 0xFF and b != 0xFF:
                            take(i, a, 0)
                            take(o, b, 2)
                for i in range(6):
                    if mem.w(P + 2 * i) == 0 and mem.w(P + 2 * ((i + 1) % 6)) == 1 \
                            and mem.w(P + 2 * ((i + 5) % 6)) == 1 and got() < 4:
                        j = 0
                        while count(i) > j:
                            u = mem.b(L + 0x14 * i + j)
                            if not chosen(u):
                                take(i, u)
                            j += 1
                for i in range(6):
                    if got() < 4:
                        j = 0
                        while count(i) > j:
                            u = mem.b(L + 0x14 * i + j)
                            if not chosen(u):
                                take(i, u)
                            j += 1
            mem.sb(S + 7, sub + 1)
        elif sub == 9:
            for i in range(6):
                u = mem.b(P + 0x28 + i)
                if u == 0xFF:
                    continue
                sq = mem.w(P + 0x14 + 2 * i)
                mem.sb(rec(u) + 8, 4 if pos(u) == sq else 3)
                mem.sw(rec(u), sq)
                mem.sw(rec(u) + 4, mem.w(rec(foe) + 6))
                mem.sb(A + 0x93C + 9 * u, 2)
                mem.sw(A + 0x934 + 9 * u, pos(foe))
                mem.sw(A + 0x938 + 9 * u, mem.w(rec(foe) + 6))
            mem.sb(S + 7, 0x0A)
        elif sub == 0x0A:
            mem.sw(rec(foe) + 2, 0xFFFF)
            mem.sb(G + 6, foe + 1)
            mem.sb(S + 7, 1)
        else:
            mem.sb(S + 7, 0)
            mem.sb(S + 4, 0x0B)
    elif step == 0x0B:                  # the units without a task
        if sub == 0:
            mem.sb(G + 7, 0)
            mem.sb(S + 7, 1)
            for n in range(0xF1):
                mem.sw(rec(n) + 2, 0x3E8)
        elif sub == 1:
            next_unit(lambda n: not half(n) and not f(n) & 0xC000 and not mem.w(mem.unit(n) + 6) & 0x20
                      and not com(n) & 0x380 and same_side(f(n), player) and mem.b(rec(n) + 8) == 0, 0x0A)
        elif sub == 2:
            n = cur()
            for u in range(0xF1):
                if f(u) & 0xC002 or mem.w(mem.unit(u) + 6) & 0x20 or u == n:
                    continue
                to = mem.w(rec(u)) if same_side(f(u), player) else pos(u)
                mem.sw(rec(u) + 2, dist(pos(n), to))
            mem.sb(S + 7, 5)
        elif sub == 5:
            best = -1
            targets = mem.w(mem.utype(cur()) + 5) & 0x3C
            mem.sb(G + 0x14, 0xFF)
            for u in range(0xF1):
                if not f(u) & 0xC000 and same_side(f(u), other) and s16(mem.w(rec(u) + 6)) > best \
                        and s16(mem.w(rec(u) + 2)) < 0x3E8 and targets & f(u):
                    best = s16(mem.w(rec(u) + 6))
                    mem.sb(G + 0x14, u)
            mem.sb(S + 7, 6 if mem.b(G + 0x14) <= NONE else 7)
        elif sub in (6, 8):
            n, u = cur(), mem.b(G + 0x14)
            sq = pos(u) if sub == 6 else mem.w(rec(u))
            if can_go(mem, sq, n, player, 0):
                give(mem, n, 0x0A, sq, 1)
                mem.sb(G + 7, n + 1)
                mem.sb(S + 7, 1)
            else:
                mem.sw(rec(u) + 2, 0x3E8)
                mem.sb(S + 7, sub - 1)
        elif sub == 7:
            best = 0x3E8
            mem.sb(G + 0x14, 0xFF)
            for u in range(0xF1):
                if mem.b(rec(u) + 8) in (0, 8, 0x0C, 6) or f(u) & 0xC000 or com(u) & 0x380 \
                        or not same_side(f(u), player) or score(u) <= 0x0A or s16(mem.w(rec(u) + 2)) >= best:
                    continue
                best = s16(mem.w(rec(u) + 2))
                mem.sb(G + 0x14, u)
            mem.sb(S + 7, 8 if mem.b(G + 0x14) <= NONE else 9)
        elif sub == 9:
            n = cur()
            if com(n) & 0x380 or helper(mem, player, n, 0x46, 0x0B, mem.hq_square(player)):
                mem.sb(S + 7, 1)
                mem.sb(G + 7, n + 1)
        else:
            mem.sb(S + 7, 0)
            mem.sb(S + 4, 0x0C)
    else:
        mem.sb(S + 4, 0)
        mem.sb(S + 6, 0)
        return 1
    return 0


def script_put(mem, player, *keys):
    """bytes onto the keys' script and FFh after them"""
    S = mem.A + 0x11AE + 0x20 * player
    for k in keys:
        i = mem.b(S + 0x1E)
        mem.sb(S + 0x1E, i + 1)
        mem.sb(S + i, k)
    mem.sb(S + mem.b(S + 0x1E), 0xFF)


def toward(mem, player, x, y, ax, ay):
    """the directions from x, y to ax, ay into the script -> 1 when there"""
    if x == ax and y == ay:
        return 1
    keys = []
    if x != ax:
        keys.append(3 if x > ax else 4)
    if y != ay:
        keys.append(1 if y > ay else 2)
    script_put(mem, player, *keys)
    return 0


def steer(mem, player, cur, aim):
    """T1938:157E: a step of the cursor towards the square -> 1 when it is
    there (or the square is none of the map's, or in its last column or
    row)"""
    F = mem.F
    w, h = mem.size()
    if s16(aim) < 0 or s16(mem.w(F + 0x246C)) < s16(aim):
        return 1
    cur, aim = s16(cur) >> 1, s16(aim) >> 1
    ax, ay = aim % w, aim // w
    if w - 1 <= ax or h - 1 <= ay:
        return 1
    return toward(mem, player, cur % w, cur // w, ax, ay)


def scroll(mem, player, x, y):
    """T1938:140D: a step of the overview's window towards the one with
    the square x, y in its middle -> 1 when it is there"""
    C = mem.F + turn.CURSORS + 0x31 * player
    w, h = mem.size()
    if w <= x or h <= y:
        return 1
    x = 0 if x < 5 else min(x - 5, w - 10)
    y = 0 if y < 4 else min(y - 4, h - 8)
    x &= ~1
    y &= ~1
    return toward(mem, player, s16(mem.w(C + 0x0C)), s16(mem.w(C + 0x0E)), x, y)


def go_to(mem, player, cur, aim, Q):
    """T1938:1245: the cursor to a square, in steps (the commands' +48h):
    a square five or more away by the overview (fire with right, its
    window moved, fire), then step by step -> 1 when it is there"""
    F = mem.F
    C = F + turn.CURSORS + 0x31 * player
    w, h = mem.size()
    if s16(aim) < 0 or s16(mem.w(F + 0x246C)) < s16(aim):
        return 1
    c, a = s16(cur) >> 1, s16(aim) >> 1
    step = mem.b(Q + 0x48)
    if step == 0:
        near = abs(c % w - a % w) < 5 and abs(c // w - a // w) < 5
        mem.sb(Q + 0x48, 4 if mem.b(C + 0x18) or near else 1)
    elif step == 1:
        script_put(mem, player, 0x83)
        mem.sb(Q + 0x48, 2)
    elif step == 2:
        if scroll(mem, player, a % w, a // w):
            mem.sb(Q + 0x48, 3)
    elif step == 3:
        script_put(mem, player, 5)
        mem.sb(Q + 0x48, 4)
    elif step == 4:
        if steer(mem, player, cur, aim):
            mem.sb(Q + 0x48, 5)
    else:
        mem.sb(Q + 0x48, 0)
        return 1
    return 0


def find_holder(mem, player, n):
    """T1938:16C1 -> the record of the building or unit that holds unit n
    in one of its slots: the factories, the depots, the units that hold
    others, the headquarters; None when none does"""
    for table, count in ((turn.FACTORIES, 10), (turn.DEPOTS, 10), (turn.CARGO, 0x46), (turn.HQS, 2)):
        for k in range(count):
            B = mem.F + table + 0x1C * k
            if mem.w(B + 0x19) & 0x8002:
                continue
            if n in mem.m[B + 7 * player:B + 7 * player + 7]:
                return B
    return None


def to_slot(mem, player, slot):
    """up or down in a building's screen -> 1 when the cursor is on the slot"""
    at = s16(mem.w(mem.F + turn.CURSORS + 0x31 * player + 0x0E))
    if at == slot:
        return 1
    script_put(mem, player, 2 if at < slot else 1)
    return 0


def carry_out(mem, player, say):
    """T1938:012F: a step of the command that runs -> what it returns"""
    F, A = mem.F, mem.A
    Q = A + 0x11FE + 0x49 * player
    C = F + turn.CURSORS + 0x31 * player
    P = F + turn.PLAYERS + 0x17 * player
    D = (mem.load + ORDER) * 16
    w, h = mem.size()
    cmd = Q + 7 * mem.b(Q + 0x46)
    kind = mem.b(cmd)
    if kind == 0xFF or not 1 <= kind <= 7:
        mem.sb(Q, 0xFF)
        mem.sb(Q + 0x46, 0)
        return 0xFFFF if kind == 0xFF else 0
    cur = mem.w(C)
    step = mem.b(Q + 0x47)
    moving = mem.w(F + 0x2510) & 0x80
    say('command %d (%02X %04X %04X), step %d, go_to\'s %d' % (
        kind, mem.b(cmd + 1), mem.w(cmd + 3), mem.w(cmd + 5), step, mem.b(Q + 0x48)))

    def pos(n):
        return (mem.w(mem.unit(n) + 0x0B + 2 * player) - 1) & 0xFFFF

    def mark(sq):
        sq = s16(sq) >> 1
        return mem.b(F + turn.MARKS + sq % w + 64 * (sq // w))

    def give_up():
        mem.sw(P, mem.w(P) | 8)

    def nxt(key=None):
        if key is not None:
            script_put(mem, player, key)
        mem.sb(Q + 0x47, step + 1)

    def done():
        mem.sb(Q + 0x47, 0)
        return 1

    def holder(at):
        B = find_holder(mem, player, mem.b(cmd + 1))
        if B is None:
            mem.sw(D + at, 0xFFFF)
            mem.sw(D + at + 2, 0xFFFF)
        else:
            mem.sfar(D + at, B)
        return B

    def slot_of(B, n):
        slot = 0
        for i in range(7):
            if mem.b(B + 7 * player + i) == n:
                slot = i
        return slot

    r = 0
    if kind == 1:                               # a unit to a square
        n, aim = mem.b(cmd + 1), mem.w(cmd + 3)
        if step == 0:
            if not moving:
                nxt()
            mem.sb(Q + 0x48, 0)
        elif step == 1:
            if go_to(mem, player, cur, pos(n), Q):
                nxt()
        elif step == 2:
            nxt(0x80)
        elif step == 3:
            if steer(mem, player, cur, aim):
                nxt()
        elif step == 4:
            mem.sb(F + turn.REC, n)
            if not mark(cur) & (2 if player else 1):
                give_up()
            elif turn.stop_check(mem, cur, player, mem.squares(player)):
                give_up()
            nxt(5)
        elif step == 5:
            nxt()
        elif step == 6:
            nxt(5)
        else:
            r = done()
    elif kind == 2:                             # a unit fires at a unit
        n, target = mem.b(cmd + 1), mem.b(cmd + 3)
        if step == 0:
            if go_to(mem, player, cur, pos(n), Q):
                nxt()
        elif step == 1:
            nxt(0x80)
        elif step == 2:
            if steer(mem, player, cur, pos(target)):
                nxt()
        elif step == 3:
            script_put(mem, player, 5)
            if mark(pos(target)) & (8 if player else 4):
                mem.sb(Q + 0x47, 5)
            else:
                nxt()
                give_up()
        elif step == 4:
            nxt(5)
        else:
            r = done()
    elif kind == 3:                             # the change of phase
        hq = mem.hq_square(player)
        if step == 0:
            if go_to(mem, player, cur, hq, Q):
                nxt()
        elif step == 1:
            if steer(mem, player, cur, (hq + 2 * w) & 0xFFFF):
                nxt()
        elif step == 2:
            if not moving:
                nxt(0x82)
        elif step == 3:
            S = mem.state(player)
            if mem.w(S) & 4:
                mem.sw(S, mem.w(S) & 0xFFFB)
                nxt()
        else:
            r = done()
    elif kind == 4:                             # a unit out of its building
        n = mem.b(cmd + 1)
        B = mem.ptr(D + 0x1F) if step else None
        if step == 0:
            if holder(0x1F) is None:
                r = done()
            else:
                nxt()
        elif step == 1:
            if go_to(mem, player, cur, mem.w(B + 0x0E), Q):
                nxt()
        elif step == 2:
            nxt(0x82)
        elif step == 3:
            if to_slot(mem, player, slot_of(B, n)):
                nxt()
        elif step == 4:
            if not moving:
                nxt()
        elif step == 5:
            nxt(0x80)
        elif step == 6:
            nxt()
        elif step == 7:
            lst = mem.aims(player)
            list_reach(mem, player, n, lst)
            if mem.w(F + 0x0B62) == 0:
                mem.sw(D + 0x23, mem.w(B + 0x0E))
                give_up()
            else:
                mem.sw(D + 0x23, mem.w(lst))
            mem.sw(F + 0x0B62, 0)
            nxt()
        elif step == 8:
            if steer(mem, player, cur, mem.w(D + 0x23)):
                nxt()
        elif step in (9, 10):
            nxt(5)
        else:
            if step == 11:
                script_put(mem, player, 0x83)
            r = done()
    elif kind == 5:                             # the cursor to a square
        if go_to(mem, player, cur, mem.w(cmd + 1), Q):
            r = 1
    elif kind == 6:                             # a repair
        n = mem.b(cmd + 1)
        B = mem.ptr(D + 0x25) if step else None
        if step == 0:
            if holder(0x25) is None:
                r = done()
            else:
                nxt()
        elif step == 1:
            if go_to(mem, player, cur, mem.w(B + 0x0E), Q):
                nxt()
        elif step == 2:
            nxt(0x82)
        elif step == 3:
            if to_slot(mem, player, slot_of(B, n)):
                nxt()
        elif step == 4:
            nxt(0x81)
        else:
            if step == 5:
                script_put(mem, player, 0x83)
            r = done()
    else:                                       # a type made in a factory
        t = mem.b(cmd + 1)
        B = F + turn.FACTORIES + 0x1C * mem.b(cmd + 3)
        if step == 0:
            mem.sfar(D + 0x29, B)
            if go_to(mem, player, cur, mem.w(B + 0x0E), Q):
                nxt()
        elif step in (1, 3, 5):
            nxt(0x82)
        elif step == 2:
            if to_slot(mem, player, slot_of(B, 0xFF)):
                nxt()
        elif step == 4:
            line = 0
            for i in range(mem.b(F + 0x1338)):
                if mem.b(F + 0x4136 + i) == t:
                    line = i
            if s16((mem.w(C + 0x0E) + mem.w(C + 0x20)) & 0xFFFF) < line:
                script_put(mem, player, 2)
            else:
                nxt()
        elif step == 6:
            nxt(0x83)
        else:
            r = done()
    if r:
        mem.sb(Q + 0x46, mem.b(Q + 0x46) + 1)
    return 0


def play_script(mem, player, say):
    """T1938:000F: the next byte of the keys' script into the player's
    directions and fire -> 1 at the script's end"""
    A = mem.A
    S = A + 0x11AE + 0x20 * player
    D = (mem.load + ORDER) * 16
    keys = A + 0x11EE + 4 * player
    mem.sw(keys, 0)
    k = mem.b(S + mem.b(S + 0x1E))
    if k != 0xFF:
        if k & 0x80:
            v = mem.b(D + 0x10 + 3 * (k & 0x7F) + mem.b(S + 0x1F))
            if v != 0xFF:
                mem.sw(keys, v)
                mem.sb(S + 0x1F, mem.b(S + 0x1F) + 1)
            else:
                mem.sb(S + 0x1F, 0)
                mem.sb(S + 0x1E, mem.b(S + 0x1E) + 1)
        else:
            mem.sw(keys, mem.b(D + 6 + k))
            mem.sb(S + 0x1E, mem.b(S + 0x1E) + 1)
    say('script byte %02X: keys %04X' % (k, mem.w(keys)))
    if mem.b(S + mem.b(S + 0x1E)) == 0xFF:
        mem.sb(S, 0xFF)
        mem.sb(S + 0x1E, 0)
        mem.sb(S + 0x1F, 0)
        return 1
    return 0


def regions(mem, player):
    F, A = mem.F, mem.A
    return [('the keys\' scripts and the keys (F2C0A:11AE, 11EE)', A + 0x11AE, 0x48),
            ('the players (F27EE:243E)', F + turn.PLAYERS, 0x2E),
            ('the cursors (F27EE:26B4)', F + turn.CURSORS, 0x62),
            ('the commands\' own values (F2D33:001F)', (mem.load + ORDER) * 16 + 0x1F, 0x0E),
            ('the players\' records (F2C0A:00A2)', A + 0xA2, 0x18),
            ('the units\' records of the move phase (00BA)', A + 0xBA, 0xF1 * 9),
            ('the units\' records of the attack phase (0934)', A + 0x934, 0xF1 * 9),
            ('the aims', mem.aims(player), 6 * 0x40),
            ('the marks', F + turn.MARKS, 0x1104),
            ('the units', F + turn.UNITS, 0xF1 * 0x1A),
            ('the commands (11F6, 11FE)', A + 0x11F6, 8 + 2 * 0x49),
            ('the length of a path (F27EE:0B62)', F + 0x0B62, 2),
            ('the move\'s record (F27EE:1326)', F + turn.REC, 0x0D),
            ('the random number\'s state', (mem.load + 0x2D8D) * 16 + 0x1938, 4),
            ('the plan\'s own values (F2D36:0006, F2D37:0006)', (mem.load + 0x2D36) * 16 + 6, 0x18)]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('entry')
    ap.add_argument('exit', nargs='?')
    ap.add_argument('--calls', type=int, default=1,
                    help='so many calls one after another, up to the one that returns 1')
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument('--assess', action='store_true')
    g.add_argument('--handout', action='store_true')
    g.add_argument('--plan', action='store_true')
    g.add_argument('--carry', action='store_true')
    g.add_argument('--script', action='store_true')
    ap.add_argument('--player', type=int, default=1)
    ap.add_argument('--load', default='0077')
    ap.add_argument('-v', action='store_true', help='the bytes that differ')
    a = ap.parse_args()
    load = int(a.load, 16)
    mem, was = Mem(a.entry, load), Mem(a.entry, load)
    moving = bool(mem.w(mem.F + 0x2510) & 0x80)
    stage = (assess if a.assess else plan if a.plan else carry_out if a.carry
             else play_script if a.script else hand_out)
    if a.carry or a.script:
        a.calls, moving = 1, False              # one call: the map's loop moves the cursor between two
    say = print if a.calls == 1 or a.v else (lambda t: None)
    calls = 0
    while calls < a.calls:
        if calls and a.plan:
            # computer_step changes the message (the record's +0Bh) before each call
            mem.sb(mem.state(a.player) + 0x0B, 0 if mem.b(mem.state(a.player) + 0x0B) else 1)
        calls += 1
        before = bytes(mem.m)
        r = stage(mem, a.player, say)
        if r:
            break
        if mem.m == before:
            calls -= 1
            print('the next call changes nothing: it waits')
            break
        if a.handout and mem.w(mem.state(a.player)) & 0x20:
            break                               # the stage is the first again
    print('calls %d, the last returns %d' % (calls, r))
    if not a.exit:
        return 0
    out = Mem(a.exit, load)
    bad = 0
    if moving:
        print('a move runs meanwhile: the units, the marks, the path and the move record are its, not compared')
    for label, at, n in regions(mem, a.player):
        if moving and at in (mem.F + turn.MARKS, mem.F + turn.UNITS, mem.F + 0x0B62, mem.F + turn.REC):
            continue
        diff = [i for i in range(n) if mem.m[at + i] != out.m[at + i]]
        changed = sum(1 for i in range(n) if was.m[at + i] != out.m[at + i])
        print('%s: %d bytes, %d changed by the game, %s' % (
            label, n, changed, 'all as the game\'s' if not diff else '%d differ' % len(diff)))
        bad += len(diff)
        if a.v:
            for i in diff[:40]:
                print('   +%04X: the tool %02X, the game %02X' % (i, mem.m[at + i], out.m[at + i]))
    return 0 if bad == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
