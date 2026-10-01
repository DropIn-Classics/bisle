#!/usr/bin/env python3
"""Battle Isle's tables of unit types, ground and maps (UNIT.DAT,
GROUND.DAT, CODES.DAT): read them, write them back, show them.

    datfiles.py [GAMEDIR ...] [--ground] [--codes] [--raw]

Without GAMEDIR the folder of each game in the game's folder (ISLE,
DESERT, MOON) is taken.  Per folder: UNIT.DAT's types, one line each,
then one line for GROUND.DAT (its records by building flag and owner)
and one for CODES.DAT, and whether each file, parsed and written back,
gives the (unpacked) file's bytes.  --ground prints GROUND.DAT's
records, one line each, --codes CODES.DAT's; --raw adds a unit type's
bytes that have no column, in hex.

All three files are packed (see tpwmfiles.py).  BATTLE.EXE's T0708 loads them
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
                  (T169E:000E make_unit, seen); the computer player
                  compares it with distances (T17C0:10E4, T1C04:1041,
                  seen): how far the unit moves, presumably
    +01 armour    the fight's reckoning (T2190:000D, seen) takes it of
                  both sides beside the attacker's hit value: what a
                  unit withstands, presumably (the formula is not read);
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
                  bits 40h and 80h are tested in the fight (not read
                  further)
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
    +0E flags     a word: 1 (tested with the unit's +14h in T0408:2087,
                  not read further), 2 set or cleared by a map's .SHP
                  (mapfiles.py): T1479:11B4 leaves such a type out of
                  the list of what can be made (seen), 4 the count is
                  count2
    +10 inside    a word, copied to the unit's +6: bits 40h..800h AND a
                  holding unit's cargo word: which of them can take it;
                  4000h a unit that gets 2 in its +0Ah (the pioneers:
                  depots it can build, presumably); the low bits and
                  1000h, 2000h, 8000h are not looked into
    +12 cargo     a word: the flags of the record make_unit gives a unit
                  that holds others (T169E:04A0, ORed with 20h), one bit
                  a type
    +14 w_dist    the path search's (T0BA0:0E2E, seen) weights: a
    +15 w_ground  square's key is its distance to the aim * w_dist + its
                  ground's cost * w_ground
    +16 sound     passed to T0D36:0165 with kind 4 when the unit has
                  moved (T122D:09E0, seen): an effect's number,
                  presumably
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
    +41 fight     three bytes read by the fight scene (T1F5A, seen) and
                  T0408:2DF7; +43 holds flags 1, 2.  Not read further

GROUND.DAT: records of 6 bytes (110 in ISLE and DESERT, all the table
at F27EE:0751 has room for up to the next variable; 150 in MOON), the
ground values of a map's squares:

    +0 flags      a word: 40h, 100h, 400h a building of one of three
                  kinds and 1, 2 its owner (mapfiles.py); 4000h: an air
                  unit (class 10h) cannot be set there (T122D:14DD);
                  8000h, 80h, 200h, 2000h, 1000h, 800h, 20h, 10h, 8, 4
                  are tested or stored here and there (not read further)
    +2 units      AND a unit type's +04 (above)
    +3 cost       the square's cost for a unit that can be there, in
                  T0BA0:098F's map and in the path search (seen), and
    +4 cost2      the same for a unit with flag 10h in its +4 (the air
                  units, by the class), whatever the mask; not seen in
                  the run
    +5 scene      1..7: the fight scene's choice (a switch in T2112:0000,
                  a table at F2D65:000E; seen): the background,
                  presumably

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
    ('w_dist', 0x14, 'b'), ('w_ground', 0x15, 'b'), ('sound', 0x16, 'b'), ('b17', 0x17, 'B'),
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
        s += ' sound %d +17 %02X pic %d,%d serial %s fight %s names %s %s' % (
            t['sound'], t['b17'], t['pic'], t['pic_y'], t['serial'].hex(), t['fight'].hex(),
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
        for name, read, write in (('UNIT.DAT', read_units, write_units),
                                  ('GROUND.DAT', read_ground, write_ground),
                                  ('CODES.DAT', read_codes, write_codes)):
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
            else:
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
