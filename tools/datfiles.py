#!/usr/bin/env python3
"""Battle Isle's tables of unit types, ground and maps (UNIT.DAT,
GROUND.DAT, CODES.DAT, AMOK.DAT): read them, write them back, show them.

    datfiles.py [GAMEDIR ...] [--ground] [--codes] [--raw]

Without GAMEDIR the folder of each game in the game's folder (ISLE,
DESERT, MOON) is taken.  Per folder: UNIT.DAT's types, one line each,
then one line for GROUND.DAT (its records by building flag and owner),
one for CODES.DAT and one for AMOK.DAT, and whether each file, parsed
and written back, gives the (unpacked) file's bytes.  --ground prints GROUND.DAT's
records, one line each, --codes CODES.DAT's; --raw adds a unit type's
bytes that have no column, in hex.

The files are packed (see tpwmfiles.py; AMOK.DAT only in MOON).  BATTLE.EXE's T0708 loads them
with load_file when a map starts (seen with -dos), UNIT.DAT to
F27EE:001F and GROUND.DAT to F27EE:0751, over the same tables in the
program's own data: BATTLE.EXE holds ISLE's two files byte for byte
there (checked), and DESERT's UNIT.DAT is ISLE's.

What a field is for is read from BATTLE.EXE's code; "seen" means the
routine read that field in a run (a battle of 600 s against the
computer on map 16, the runner's -rwatch over both tables; docs/
HANDOFF.md has the key script).  Nothing here is checked against what
the game shows on the screen.

UNIT.DAT: 27 records of 44h bytes, the unit types (a map's unit byte
>> 1, see mapfiles.py):

    +00 move      copied to the unit's record (+0) when a unit is made
                  (T169E:000E make_unit, seen): the points the unit has
                  for a move, of which each square entered takes its
                  ground's cost (reach, T0BA0:0323; moves.py); the
                  unit's screen shows half of it.  The computer player
                  compares it with distances (T17C0:10E4, T1C04:1041,
                  seen)
    +01 armour    what a unit withstands in a fight: with the count and
                  the unit's +1 it makes a side's defence, and the
                  difference of attack and defence is divided by it
                  (fight_reckon, T2190:000D; fight.py has the formula);
                  T0408:2E7A adds it up over a player's units (seen)
    +02 count     how many the unit has when whole (make_unit copies it
                  to the unit's +2): T0408:2087, run over all units,
                  adds one up to it for a unit with 8000h in its +6 and
                  for the units inside such a unit that holds others;
                  with flag 4 of +0E the unit's +3 and
    +03 count2    this one are taken instead (the ships: 1 and 6)
    +04 ground    AND GROUND.DAT's +2: where the unit can be (T122D:149B
                  gives message 0Eh, UNIT CANNOT SET HERE, when 0;
                  T0BA0:098F fills a map of the squares' costs for a
                  unit with it; both seen)
    +05 targets   a word.  AND 3Ch against the other type's class (+0C):
                  which classes it can fire at (T2190, T0D36:1275, seen);
                  2: at a unit with 2 in its +6 as well; with 40h or
                  80h the unit does not answer an attack (fight.py), and
                  with 40h it cannot fire at the six squares around it
                  (fire_reach, T0BA0:05C8) and gives no flank bonus
    +07 range_air how far it fires at class 10h: T0D36:12F5 marks the
                  targets of (targets AND FFD3h) within it, when above 1
    +08 range     the same for the other classes (targets AND FFEFh)
                  (T0D36:1275; neither marks any: message 1Bh, NO AIMS)
    +09 hit_air   the attacker's value in the fight against a unit of
    +0A hit_land  class 10h (+09), of class 8 (+0B), else +0A (T2190,
    +0B hit_sea   seen for +0A)
    +0C class     a word, copied to the unit's +4 with the player in bits
                  0, 1: 4, 8, 10h, 20h the kinds the targets word names
                  (land, sea, air by the types' names; 20h the two that
                  land on a shore, presumably); 2 a unit of neither
                  player (mapfiles.py: owner 2); 40h a unit of two
                  squares and 80h its first half; 1000h a unit that
                  holds others (make_unit gives it a record of
                  F27EE:0B64)
    +0E flags     a word: 1 a unit of the type that has moved keeps the
                  flag "has moved" (200h in its +4) when the phase
                  changes (change_phase, T0408:2087; turn.py): it does
                  not fire in the phase after, presumably (ISLE's types
                  0 and 7), 2 set or cleared by a map's .SHP
                  (mapfiles.py): T1479:11B4 leaves such a type out of
                  the list of what can be made (seen), 4 the count is
                  count2
    +10 inside    a word, copied to the unit's +6: bits 40h..800h AND a
                  holding unit's cargo word: which of them can take it;
                  4000h a unit that gets 2 in its +0Ah (the pioneers:
                  depots it can build, presumably); 4 a unit that a
                  mover with 2000h takes in, 20h one that a mover with
                  1000h takes in (stop_check; it becomes the mover's
                  player's when the phase changes), and a unit with 4
                  taken into a building becomes energy there, neither
                  20h nor 4 counts as a unit at a map's end (turn.py);
                  8000h a unit whose count rises by one each change of
                  phase; the other low bits are not looked into
    +12 cargo     a word: the flags of the record make_unit gives a unit
                  that holds others (T169E:04A0, ORed with 20h), one bit
                  a type
    +14 w_dist    the path search's (T0BA0:0E2E, seen) weights: a
    +15 w_ground  square's key is its distance to the aim * w_dist + its
                  ground's cost * w_ground
    +16 pace      the passes of the map's loop (18.2 a second) between
                  two squares of a move: move_step (T122D:0713) sets
                  timer 4 with it
    +17           no reader found in the code or the run
    +18 pic_y     added to y when the type's big picture is drawn
    +19 pic       the big picture's number in the list at F27EE:0A8F
                  (T1479:0C93, seen)
    +1A name      17 bytes, a text ending with 0, drawn by T1479:0C93
    +2B group     17 bytes, another name (drawn in the run at 154 s, the
                  first fight; by which routine is not looked into)
    +3C serial    two bytes, one a player: load_fin sets both to 1,
                  make_unit gives the unit the value (its +9) and adds 2.
                  The file's values are never used
    +3E cost      T0708:3965 takes it from the building's +16h when the
                  type is made there; T1479:11B4 lists the types that
                  cost no more than its argument (seen)
    +3F room      of a unit that holds others: T0708:2EC4 shows it less
                  the sizes of what is inside; make_unit puts it in the
                  cargo record's +16h, +17h
    +40 size      what the unit takes of that room
    +41 fight     three bytes for the fight scene's sounds (scene.py;
                  the sounds themselves are not looked at): +41 the
                  sound its units come in with (FFh none), +42 that of
                  its shot, +43 flags: 1 the first sound is not ended
                  when the units have arrived, 2 the shot's sound gets
                  5Fh instead of 7Fh (its loudness, presumably).
                  T0408:2DF7 reads them too (not read)

GROUND.DAT: records of 6 bytes (110 in ISLE and DESERT, all the table
at F27EE:0751 has room for up to the next variable; 150 in MOON), the
ground values of a map's squares:

    +0 flags      a word: 40h, 100h, 400h a building of one of three
                  kinds (the headquarters, a depot, a factory by the
                  status screen's words: AMOK.DAT below) and 1, 2 its
                  owner (mapfiles.py); 4000h: an air
                  unit (class 10h) cannot be set there (T122D:14DD);
                  8000h, 80h, 200h, 2000h, 1000h, 800h, 20h, 10h, 8, 4
                  are tested or stored here and there (not read further)
    +2 units      AND a unit type's +04 (above)
    +3 cost       the square's cost for a unit that can be there, in
                  T0BA0:098F's map and in the path search (seen), and
    +4 cost2      the same for a unit with flag 10h in its +4 (the air
                  units, by the class), whatever the mask; not seen in
                  the run
    +5 scene      1..7: the fight scene's ground (its list of
                  FIGHT.LIB's pieces and the way the units come in:
                  scene.py) and the two signed bytes at F2D65:000E that
                  fight_reckon adds to attack and defence (fight.py)

CODES.DAT: records of 10 bytes, one a map in the maps' order (the menu,
T1090, loads it and takes the length / 10 for their number: 34 in each
game; after_map loads it again):

    +0 code       5 letters, each less 30h (A is 11h).  The menu looks
                  for the code typed among them (T1090:0B2F) and the
                  record's number becomes the map (F27EE:2523);
                  after_map shows the next map's code
    +5            2 in all records; no reader found in the code
    +6 players    2: T1090:110F clears bit 400h of F27EE:250C, else
                  sets it (1 in the files): with that bit T0708 loads
                  the map's .COM (mapfiles.py), so 1 is a map against
                  the computer, 2 one of two players.  In each game the
                  maps with 1 are those that have a .COM file
    +7 step       added to the map's number after a map that was won
                  (T15AC:03EB): 1, and 0 in the last map of a row
    +8 last       bit 0: after_map sets bit 20h of F27EE:250C, which
                  plays the ending (HANDOFF.md)
    +9            0 in all records; no reader found in the code

AMOK.DAT: 24h bytes, loaded to F27EE:24E8 when a map starts: ground
values and colours the program has no constants for:

    +00 factory   three GROUND.DAT indexes, by owner 0, 1, 2: the ground
                  a building's square gets when the building changes its
                  owner (T0408:16B3; of a building record with flag 8 in
                  its +19h).  In all three games they are the records
                  with flag 400h and that owner.  The player's count of
                  them (F27EE:243E +3, 17h bytes a player) is the status
                  screen's FACTORY, its second row (T1479:0DC5
                  draw_status, as read; the screen seen in a run of
                  ISLE's first map, which has none: all 0)
    +03 depot     the same for flag 10h of the building record: the
                  ground records with flag 100h; the count (+4) is the
                  status screen's DEPOT
    +06 hq        the same for flag 4, owners 0 and 1: the records with
                  flag 40h (no count: the headquarters, of which every
                  map has one a player, mapfiles.py)
    +08 steps     the count a square's record (F27EE:2472, 7 bytes, four
                  of them) starts from when T0408:2415 puts a square
                  down for a change; T0408:0C86 counts it down, draws a
                  step (T0408:25C8) while it is below this value and
                  stores the square's new bytes after 0: the steps of
                  an animation on the square (the explosion, presumably)
    +09 colours   15 bytes, colours of texts and boxes: +09, +0A the
                  status screen's counts of player 0 and 1 (seen: red
                  and yellow, as the units); +0B, +0D texts (the last
                  argument of T164D:0140; +0D most of them); +0E, +0F,
                  +10 set before T2482:0004 (a filled box, presumably:
                  T1479:0004's, show_message's, the building screens');
                  +11, +12 T1479:01AF's two colours for T24BA:0008 and
                  T2536:0004 (a box's edges, presumably); +13, +14
                  arguments of T164D:028D in T0708:2DC0.  +0C and
                  +15..+17 have no reader in the code as far as read.
                  Not looked at on the screen but +09, +0A
    +18 site      four ground values T0408:18E8 (a unit's order 0Bh)
                  gives a square and three beside it (F27EE:41A2, set by
                  T0E9B:17D1) when all four have ground of flag 8000h
                  and no unit; the unit's +0Ah is one less then (the
                  pioneers' 2, above): a building site, presumably
    +1C built     four ground values for the same squares at order 0Dh
                  (T0408:19BF; T0B70:00E9 gives that order on a square
                  with the site's first value while there are fewer
                  than 10 depot records): a depot record is made
                  (T169E:0324), the player's depots are one more, and
                  the first square gets +03 or +04 by the player, so
                  +1C itself is never shown
    +20 dots_on   two bytes by player, and
    +22 dots      two bytes by player: the colour of a unit's dot in the
                  map's overview (T0E9B:0931 draw_overview, mapfiles.py;
                  seen: +22).  +20 is taken for a unit with flag 200h in
                  its +4 when T0D36:00F5 says the unit is the window's
                  player's (what the flag is: not read)

MOON's files are read here as BATTLE.EXE reads ISLE's; MOON.EXE is not
compared.
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir  # noqa: E402
import tpwmfiles  # noqa: E402

UNIT_REC = 0x44
GROUND_REC = 6
NAME_LEN = 17
# (name, offset, struct format) of a unit type's fields; what is between
# them stays in 'rest'
UNIT_FIELDS = (
    ('move', 0x00, 'B'), ('armour', 0x01, 'B'), ('count', 0x02, 'B'), ('count2', 0x03, 'B'),
    ('ground', 0x04, 'B'), ('targets', 0x05, '<H'), ('range_air', 0x07, 'B'), ('range', 0x08, 'B'),
    ('hit_air', 0x09, 'B'), ('hit_land', 0x0A, 'B'), ('hit_sea', 0x0B, 'B'),
    ('class', 0x0C, '<H'), ('flags', 0x0E, '<H'), ('inside', 0x10, '<H'), ('cargo', 0x12, '<H'),
    ('w_dist', 0x14, 'b'), ('w_ground', 0x15, 'b'), ('pace', 0x16, 'b'), ('b17', 0x17, 'B'),
    ('pic_y', 0x18, 'B'), ('pic', 0x19, 'B'),
    ('name', 0x1A, '17s'), ('group', 0x2B, '17s'), ('serial', 0x3C, '2s'),
    ('cost', 0x3E, 'B'), ('room', 0x3F, 'B'), ('size', 0x40, 'B'), ('fight', 0x41, '3s'),
)
GROUND_FIELDS = (('flags', 0, '<H'), ('units', 2, 'B'), ('cost', 3, 'b'), ('cost2', 4, 'b'), ('scene', 5, 'B'))
CODE_REC = 10
CODE_FIELDS = (('code', 0, '5s'), ('b5', 5, 'B'), ('players', 6, 'B'), ('step', 7, 'B'),
               ('last', 8, 'B'), ('b9', 9, 'B'))
CODE_LETTER = 0x30                      # a code's bytes are its letters less this
BUILDING_FLAGS = (0x40, 0x400, 0x100)
AMOK_REC = 0x24
AMOK_FIELDS = (('factory', 0x00, '3s'), ('depot', 0x03, '3s'), ('hq', 0x06, '2s'), ('steps', 0x08, 'B'),
               ('colours', 0x09, '15s'), ('site', 0x18, '4s'), ('built', 0x1C, '4s'),
               ('dots_on', 0x20, '2s'), ('dots', 0x22, '2s'))
# the ground flag each of AMOK.DAT's building values should have
AMOK_BUILDINGS = (('factory', 0x400), ('depot', 0x100), ('hq', 0x40))


def unpacked(path):
    data = open(path, 'rb').read()
    return tpwmfiles.unpack(data) if data[:4] == tpwmfiles.MAGIC else data


def read_records(data, size, fields):
    if len(data) % size:
        raise ValueError('%d bytes, not records of %d' % (len(data), size))
    return [{name: struct.unpack_from(fmt, data, i + off)[0] for name, off, fmt in fields}
            for i in range(0, len(data), size)]


def write_records(recs, size, fields):
    out = bytearray(size * len(recs))
    for n, r in enumerate(recs):
        for name, off, fmt in fields:
            struct.pack_into(fmt, out, n * size + off, r[name])
    return bytes(out)


def read_units(data):
    return read_records(data, UNIT_REC, UNIT_FIELDS)


def write_units(types):
    return write_records(types, UNIT_REC, UNIT_FIELDS)


def read_ground(data):
    return read_records(data, GROUND_REC, GROUND_FIELDS)


def write_ground(recs):
    return write_records(recs, GROUND_REC, GROUND_FIELDS)


def read_codes(data):
    return read_records(data, CODE_REC, CODE_FIELDS)


def write_codes(recs):
    return write_records(recs, CODE_REC, CODE_FIELDS)


def read_amok(data):
    if len(data) != AMOK_REC:
        raise ValueError('%d bytes, not %d' % (len(data), AMOK_REC))
    return read_records(data, AMOK_REC, AMOK_FIELDS)


def write_amok(recs):
    return write_records(recs, AMOK_REC, AMOK_FIELDS)


def amok_summary(a, ground):
    """the line for AMOK.DAT: its ground values against GROUND.DAT"""
    parts = []
    for name, flag in AMOK_BUILDINGS:
        ok = all(g < len(ground) and ground[g]['flags'] & flag and owner(ground[g]['flags']) == o
                 for o, g in enumerate(a[name]))
        parts.append('%s %s%s' % (name, '/'.join('%02X' % g for g in a[name]),
                                  '' if ok else ' (NOT ground of flag %Xh by owner)' % flag))
    parts.append('steps %d' % a['steps'])
    parts.append('colours %s' % ' '.join('%02X' % c for c in a['colours']))
    parts.append('site %s' % ' '.join('%02X' % g for g in a['site']))
    parts.append('built %s' % ' '.join('%02X' % g for g in a['built']))
    parts.append('dots %s, %s' % ('/'.join('%02X' % c for c in a['dots_on']),
                                  '/'.join('%02X' % c for c in a['dots'])))
    return ', '.join(parts)


def code_text(code):
    return ''.join(chr(c + CODE_LETTER) for c in code)


def ranges(numbers):
    """'0..15, 32, 33' of sorted numbers"""
    out = []
    for n in numbers:
        if out and out[-1][1] == n - 1:
            out[-1][1] = n
        else:
            out.append([n, n])
    return ', '.join('%d' % a if a == b else '%d..%d' % (a, b) if b > a + 1 else '%d, %d' % (a, b)
                     for a, b in out) or 'none'


def codes_summary(recs):
    return '%d maps, against the computer %s, without a next one %s, the last %s' % (
        len(recs), ranges([n for n, r in enumerate(recs) if r['players'] != 2]),
        ranges([n for n, r in enumerate(recs) if not r['step']]),
        ranges([n for n, r in enumerate(recs) if r['last'] & 1]))


def text(b):
    return b.split(b'\0')[0].decode('latin-1').strip()


def owner(flags):
    return 2 if flags & 2 else 1 if flags & 1 else 0


def unit_line(n, t, raw):
    s = '%2d %-14s %-15s move %2d armour %3d count %d/%d ground %02X' % (
        n, text(t['name']), text(t['group']), t['move'], t['armour'], t['count'], t['count2'], t['ground'])
    s += ' targets %04X range %d/%d hit %3d/%3d/%3d' % (
        t['targets'], t['range_air'], t['range'], t['hit_air'], t['hit_land'], t['hit_sea'])
    s += ' class %04X flags %04X inside %04X cargo %04X' % (t['class'], t['flags'], t['inside'], t['cargo'])
    s += ' path %d/%d cost %3d room %3d size %3d' % (t['w_dist'], t['w_ground'], t['cost'], t['room'], t['size'])
    if raw:
        s += ' pace %d +17 %02X pic %d,%d serial %s fight %s names %s %s' % (
            t['pace'], t['b17'], t['pic'], t['pic_y'], t['serial'].hex(), t['fight'].hex(),
            t['name'].hex(), t['group'].hex())
    return s


def ground_summary(recs):
    used = [r for r in recs if any(r.values())]
    parts = ['%d records, %d not empty' % (len(recs), len(used))]
    for f in BUILDING_FLAGS:
        by = [sum(1 for r in recs if r['flags'] & f and owner(r['flags']) == o) for o in range(3)]
        parts.append('buildings %Xh %s' % (f, '/'.join(map(str, by))))
    return ', '.join(parts)


def game_dirs(root):
    for game in ('ISLE', 'DESERT', 'MOON'):
        d = os.path.join(root, game)
        if os.path.isdir(d):
            yield d


def find(d, name):
    for f in os.listdir(d):
        if f.upper() == name:
            return os.path.join(d, f)
    raise ValueError('no %s' % name)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('dirs', nargs='*', metavar='GAMEDIR')
    ap.add_argument('--ground', action='store_true', help="print GROUND.DAT's records")
    ap.add_argument('--codes', action='store_true', help="print CODES.DAT's records")
    ap.add_argument('--raw', action='store_true', help="add a unit type's other bytes in hex")
    a = ap.parse_args()
    root = game_dir()
    total = bad = 0
    for d in a.dirs or list(game_dirs(root)):
        rel = d if a.dirs else os.path.relpath(d, root)
        ground = []
        for name, read, write in (('UNIT.DAT', read_units, write_units),
                                  ('GROUND.DAT', read_ground, write_ground),
                                  ('CODES.DAT', read_codes, write_codes),
                                  ('AMOK.DAT', read_amok, write_amok)):
            total += 1
            try:
                data = unpacked(find(d, name))
                recs = read(data)
            except (OSError, ValueError) as e:
                print('%s\\%s %s' % (rel, name, e))
                bad += 1
                continue
            same = write(recs) == data
            bad += not same
            if name == 'UNIT.DAT':
                for n, t in enumerate(recs):
                    print(unit_line(n, t, a.raw))
                line = '%d types' % len(recs)
            elif name == 'CODES.DAT':
                if a.codes:
                    for n, r in enumerate(recs):
                        print('%2d %s +5 %d players %d step %d last %d +9 %d' % (
                            n, code_text(r['code']), r['b5'], r['players'], r['step'], r['last'], r['b9']))
                line = codes_summary(recs)
            elif name == 'AMOK.DAT':
                line = amok_summary(recs[0], ground)
            else:
                ground = recs
                if a.ground:
                    for n, r in enumerate(recs):
                        print('%02X flags %04X units %02X cost %3d %3d scene %d' % (
                            n, r['flags'], r['units'], r['cost'], r['cost2'], r['scene']))
                line = ground_summary(recs)
            print('%s\\%s %s; %s' % (rel, name, line,
                                     'written back identical' if same else 'WRITTEN BACK OTHERWISE'))
    print('%d files, %d not read or not written back' % (total, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
