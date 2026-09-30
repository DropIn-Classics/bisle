#!/usr/bin/env python3
"""Battle Isle's maps (MAP\\NN.FIN, .SHP, .COM, .PMP): read them, write
them back, show them.

    mapfiles.py [MAPDIR ...] [--grid NN]

Without MAPDIR the MAP folder of each game in the game's folder (ISLE,
DESERT, MOON) is taken.  One line per map: its size, the units on it by
player, the buildings its ground has, the shops' records, whether a .COM
and a .PMP are there (and the .PMP's name), and whether each file,
parsed and written back, gives the (unpacked) file's bytes.  --grid NN
prints map NN of each folder as text: four hex digits a square, the
ground and the unit byte (".." for none).  Owners and players are
numbered 0, 1 as the loader counts them, 2 neither (that 0 is the
player of the arrow keys is not checked); "buildings 400h 1/1/2" gives
the squares of that flag by owner 0/1/2.

All four files are packed (see tpwmfiles.py).  BATTLE.EXE's T0708 loads
them when a map starts (seen with -dos: .FIN, .SHP, .COM, .PMP, in that
order), make_path's extension 0, 2, 9, 1; the name is the map's number
in two digits (F27EE:2523).

.FIN, the map (read by T0E9B:02B6):

    4 bytes: 00, the width w, 00, the height h (the loader reads the
    bytes at 1 and 3 only; 0 and 2 are 0 in all files); then w * h
    squares, row by row, 2 bytes each:
      the ground: an index into GROUND.DAT (records of 6 bytes, loaded
        at F27EE:0751; the word at +0 has flags: 400h, 100h and 40h make
        the square a building of one of three kinds (T169E:0244 fills
        the table F27EE:2780, T169E:0324 F27EE:259C, up to 10 each, by
        a counter; T169E:0401 F27EE:24B0, by the owner); the owner is 2
        with flag 2, 1 with flag 1, else 0.  Which kind is a depot, a
        factory or a headquarters is not looked into; 40h, of which each
        game has two ground values, one per player, is the headquarters,
        presumably.
      the unit: F4h or more none (the loader writes FFh there), else
        the type (byte >> 1, an index into UNIT.DAT's records of 44h
        bytes, loaded at F27EE:001F; 1Bh or more is skipped) and the
        player (bit 0; the loader counts 0 in F27EE:2440, 1 in 2457,
        records of 17h bytes per player, presumably).  A type with the flags 40h and 80h (the
        word at +0Ch of its UNIT.DAT record; ISLE's types 15, 17, 19,
        21) takes two squares: the next type (byte + 2) is in the square
        a row below for player 1, a row above for player 0 (the loader
        places the pair from the first square it meets, as read from
        the code).  A unit of flag 2 (ISLE's type 25) is given owner 2
        and not counted.  At most F1h units; the loader then replaces
        each unit byte with the unit's number.

.SHP, what the buildings hold (read by T0E9B:007B):

    27 bytes, one per unit type: bit 0 sets bit 1 of the word at +0Eh
    of the type's UNIT.DAT record (else clears it; what it means is not
    looked into); a count n; n records of 12 bytes:
      +0 the owner (as the ground's: 0, 1, 2), +1 the kind (0: the table
      of ground flag 40h, 1: of 400h, 2: of 100h, else F27EE:0B64, 70
      records), +2 the index in that table, +3 stored at the record's
      +16h and +17h, +4 at +18h, +5..+11 seven unit types (F1h or more:
      none), the building's contents.
    ISLE's 11.SHP ends 4 bytes into its last record: the loader reads
    what its buffer held there (a record's last four types; the file
    has FFh, none, for the three before).

.COM, for the computer (only when F27EE:250C bit 400h is set; seen in a
run against the computer, map 16; ISLE has 16..31, the maps after CONRA,
presumably):

    27 records of 6 bytes, one per unit type (presumably, by the count);
    the loader swaps the bytes at +2 and +3 of each (T0708:0E2A).  What
    they mean is not looked into.

.PMP (loaded by T0708 into a buffer; in a run to the first map and 25 s
on it nothing but the unpacking read that buffer; what uses it is not
known):

    a long n; "INFO"; "ILBM"; a byte (02 ISLE, 03, 1Fh or 73h DESERT, 35h
    in one of ISLE's); 55h; 4 zero bytes; words W and H (2w + 4 and
    2h + 4 but in ISLE's 02.PMP, whose H is 2h + 5); W * H bytes (n is
    22 + W * H); a name of 8 bytes (zero-padded: M00..M15 and 16..33 in
    ISLE, words in DESERT: CLOCK, LOSAG, ...); a long, 4 in all files.
    The W * H bytes (values below 20h) do not follow the .FIN's ground
    square by square; shown as a picture they are blocks of 8x8 with
    lines between (not decoded).  MOON has no .PMP files.

MOON's files are read here as BATTLE.EXE reads them; MOON.EXE's loader
is not compared.  MOON's 16.FIN has a two-square unit whose second
square is not the next type (reported).
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

UNIT_TYPES = 27
UNIT_REC = 0x44
GROUND_REC = 6
NO_UNIT = 0xF4
BUILDING_FLAGS = (0x40, 0x400, 0x100)   # the SHP kinds 0, 1, 2


def unpacked(path):
    data = open(path, 'rb').read()
    return tpwmfiles.unpack(data) if data[:4] == tpwmfiles.MAGIC else data


def read_fin(data):
    if len(data) < 4:
        raise ValueError('.FIN too short')
    w, h = data[1], data[3]
    if len(data) != 4 + 2 * w * h:
        raise ValueError('.FIN of %d bytes, not 4 + 2 * %d * %d' % (len(data), w, h))
    squares = [(data[i], data[i + 1]) for i in range(4, len(data), 2)]
    return {'head': data[:4], 'w': w, 'h': h, 'squares': squares}


def write_fin(m):
    return bytes(m['head']) + bytes(b for sq in m['squares'] for b in sq)


def read_shp(data):
    if len(data) < UNIT_TYPES + 1:
        raise ValueError('.SHP too short')
    avail = data[:UNIT_TYPES]
    n = data[UNIT_TYPES]
    recs = []
    i = UNIT_TYPES + 1
    for _ in range(n):
        r = data[i:i + 12]
        recs.append({'owner': r[0] if len(r) > 0 else None,
                     'kind': r[1] if len(r) > 1 else None,
                     'index': r[2] if len(r) > 2 else None,
                     'b3': r[3] if len(r) > 3 else None,
                     'b4': r[4] if len(r) > 4 else None,
                     'units': list(r[5:12]), 'raw': r})
        i += 12
    if i < len(data):
        raise ValueError('.SHP has %d bytes after its records' % (len(data) - i))
    return {'avail': avail, 'records': recs, 'short': i - len(data)}


def write_shp(s):
    return bytes(s['avail']) + bytes([len(s['records'])]) + b''.join(r['raw'] for r in s['records'])


def read_com(data):
    if len(data) != UNIT_TYPES * 6:
        raise ValueError('.COM of %d bytes, not %d' % (len(data), UNIT_TYPES * 6))
    return [data[i:i + 6] for i in range(0, len(data), 6)]


def read_pmp(data):
    if len(data) < 34 or data[4:12] != b'INFOILBM':
        raise ValueError('.PMP without INFO ILBM')
    n = struct.unpack_from('<I', data, 0)[0]
    W, H = struct.unpack_from('<HH', data, 18)
    if n != 22 + W * H or len(data) != n + 12:
        raise ValueError('.PMP of %d bytes, n %d, W %d, H %d' % (len(data), n, W, H))
    return {'b12': data[12], 'b13': data[13], 'zero': data[14:18], 'W': W, 'H': H,
            'body': data[22:n], 'name': data[n:n + 8], 'tail': struct.unpack_from('<I', data, n + 8)[0]}


def write_pmp(p):
    n = 22 + len(p['body'])
    return (struct.pack('<I', n) + b'INFOILBM' + bytes([p['b12'], p['b13']]) + p['zero']
            + struct.pack('<HH', p['W'], p['H']) + p['body'] + p['name'] + struct.pack('<I', p['tail']))


def tables(game):
    """(ground flags by index, unit flags +0Ch by type) of a game's folder."""
    g = unpacked(os.path.join(game, 'GROUND.DAT'))
    u = unpacked(os.path.join(game, 'UNIT.DAT'))
    ground = [struct.unpack_from('<H', g, i)[0] for i in range(0, len(g) - 1, GROUND_REC)]
    unit = [struct.unpack_from('<H', u, t * UNIT_REC + 0x0C)[0] for t in range(len(u) // UNIT_REC)]
    return ground, unit


def owner(flags):
    return 2 if flags & 2 else 1 if flags & 1 else 0


def check_fin(m, ground, unit):
    """(units per player 0/1/other, buildings per kind and owner, problems)"""
    w, h, sq = m['w'], m['h'], m['squares']
    players = [0, 0, 0]
    build = {}
    problems = []
    for i, (g, u) in enumerate(sq):
        if g >= len(ground):
            problems.append('ground %02X past GROUND.DAT' % g)
        else:
            for k, f in enumerate(BUILDING_FLAGS):
                if ground[g] & f:
                    key = (k, owner(ground[g]))
                    build[key] = build.get(key, 0) + 1
                    break
        if u >= NO_UNIT:
            continue
        t = u >> 1
        if t >= UNIT_TYPES or t >= len(unit):
            problems.append('unit %02X past the types' % u)
            continue
        if unit[t] & 2:
            players[2] += 1
        else:
            players[u & 1] += 1
        if unit[t] & 0xC0 == 0xC0:
            j = i + w if u & 1 else i - w
            if not 0 <= j < w * h or sq[j][1] != u + 2:
                problems.append('two-square unit %02X at %d,%d without %02X %s' % (
                    u, i % w, i // w, u + 2, 'below' if u & 1 else 'above'))
    return players, build, problems


def grid(m):
    rows = []
    for y in range(m['h']):
        row = []
        for x in range(m['w']):
            g, u = m['squares'][y * m['w'] + x]
            row.append('%02X%s' % (g, '..' if u >= NO_UNIT else '%02X' % u))
        rows.append(' '.join(row))
    return '\n'.join(rows)


def map_dirs(root):
    for game in ('ISLE', 'DESERT', 'MOON'):
        d = os.path.join(root, game, 'MAP')
        if os.path.isdir(d):
            yield d


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('dirs', nargs='*', metavar='MAPDIR')
    ap.add_argument('--grid', help='print this map (NN) of each folder as text')
    a = ap.parse_args()
    root = game_dir()
    dirs = a.dirs or list(map_dirs(root))
    total = bad = 0
    for d in dirs:
        ground, unit = tables(os.path.dirname(os.path.abspath(d)))
        files = {f.upper(): os.path.join(d, f) for f in os.listdir(d)}
        maps = sorted({os.path.splitext(f)[0] for f in files if f.endswith('.FIN')})
        rel = os.path.relpath(d, root) if not a.dirs else d
        for nn in maps:
            total += 1
            parts = []
            same = []
            problems = []
            try:
                data = unpacked(files[nn + '.FIN'])
                m = read_fin(data)
                same.append(write_fin(m) == data)
                players, build, problems = check_fin(m, ground, unit)
                parts.append('%2dx%-2d units %d/%d%s' % (
                    m['w'], m['h'], players[0], players[1],
                    '+%d' % players[2] if players[2] else ''))
                parts.append('buildings ' + ' '.join(
                    '%Xh %s' % (f, '/'.join(str(build.get((k, o), 0)) for o in range(3)))
                    for k, f in enumerate(BUILDING_FLAGS)))
                if a.grid == nn:
                    print('%s\\%s.FIN' % (rel, nn))
                    print(grid(m))
                if nn + '.SHP' in files:
                    data = unpacked(files[nn + '.SHP'])
                    s = read_shp(data)
                    same.append(write_shp(s) == data)
                    parts.append('shops %d%s' % (len(s['records']),
                                 ' (%d bytes short)' % s['short'] if s['short'] else ''))
                if nn + '.COM' in files:
                    data = unpacked(files[nn + '.COM'])
                    same.append(b''.join(read_com(data)) == data)
                    parts.append('.COM')
                if nn + '.PMP' in files:
                    data = unpacked(files[nn + '.PMP'])
                    p = read_pmp(data)
                    same.append(write_pmp(p) == data)
                    parts.append('.PMP %s' % p['name'].rstrip(b'\0').decode('latin-1'))
            except (ValueError, KeyError) as e:
                print('%s\\%-8s %s' % (rel, nn, e))
                bad += 1
                continue
            ok = all(same)
            bad += not ok
            print('%s\\%-8s %s; %s%s' % (
                rel, nn, ', '.join(parts),
                'written back identical' if ok else 'WRITTEN BACK OTHERWISE',
                ''.join('; ' + q for q in sorted(set(problems)))))
    print('%d maps, %d not read or not written back' % (total, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
