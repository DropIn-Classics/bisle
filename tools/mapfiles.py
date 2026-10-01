#!/usr/bin/env python3
"""Battle Isle's maps (MAP\\NN.FIN, .SHP, .COM, .PMP): read them, write
them back, show them.

    mapfiles.py [MAPDIR ...] [--grid NN] [--png DIR] [--match VRAM NN]
                [--overview VRAM NN]

Without MAPDIR the MAP folder of each game in the game's folder (ISLE,
DESERT, MOON) is taken.  One line per map: its size, the units on it by
player, the buildings its ground has, the shops' records, whether a .COM
and a .PMP are there (and the .PMP's name), and whether each file,
parsed and written back, gives the (unpacked) file's bytes.  --grid NN
prints map NN of each folder as text: four hex digits a square, the
ground and the unit byte (".." for none).  Owners and players are
numbered 0, 1 as the loader counts them, 2 neither (that 0 is the
player of the arrow keys is not checked); "buildings 400h 1/1/2" gives
the squares of that flag by owner 0/1/2.  --png DIR writes each map as
a picture, GAME_NN.png, as the game draws it at its start (below);
and its overview, GAME_NN_overview.png, where it has a .PMP;
--match VRAM NN looks for map NN of each folder in a run's video memory
(run.py -vram) and says where each player's window shows it and how
many of the window's pixels are the picture's; --overview VRAM NN does
the same for the map's overview.

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

.PMP, the map's overview (loaded by T0708 into a buffer of its own):

    a graphics library of one entry (libfiles.py): a long, the
    directory's offset; the entry: the label "INFOILBM", the transparent
    value (02 in ISLE's but one, 03, 1Fh, 73h in DESERT's; pixels have
    it in four files only, ISLE's 02 and 08, DESERT's 01 and 09: not
    drawn), the kind 'U', offsets 0, the width 2w + 4 and the height
    2h + 4 (ISLE's 02.PMP: 2h + 5), a byte a pixel, plane by plane
    (values 0..1Fh, and ISLE's 08's transparent value above them); the
    directory's one record: a name (M00..M15
    and 16..33 in ISLE, words in DESERT: CLOCK, LOSAG, ...) and the
    entry's offset, 4.  MOON has no .PMP files.
    T0E9B:0931 (draw_overview) draws the entry with colour base 70h in
    the middle of a player's window, at x = 50h - (w + 2) - 3 (A0h more
    in the right window) and y = 64h - (h + 2) - 4, then a dot for each
    unit at x + 2 * column + 2, y + 2 * row + 2: four pixels, the colour
    c, c + 1 right of it and below it, c + 2 at the fourth, c from
    AMOK.DAT by the unit's player (datfiles.py: 02h and 12h); no dot for
    a unit of neither player or one with flag 2 in its +6 (the types
    drawn in their own player's window only).  A player gets it with
    fire held on an empty square and right, fire let go first
    (T0D36:1567: the cursor's function 3).
    Checked against a run (ISLE's first map, keys 14 space, 33 enter, 48
    left, 49 left, 50 space+, 50.6 right+, 51.5 space-, 52 right-, -vram
    at 58 s; --overview): the picture with its nine dots was in video
    memory at x 59, y 78, all pixels but 160, a frame of two pixels
    around 24 by 20 of them (what the window shows of the map,
    presumably; who draws it is not read).  --png draws the dots of all
    units as at the map's start.

The map as the game draws it (T0E9B:0A9B, a window, read from the code;
--png): a square is 24x24 pixels, column c at x = 16 * c, row r at
y = 24 * r, 12 further down in the odd columns: hexagons.  The ground is
entry number "ground" of LIB\\PART.LIB in PART.DAT's order (libfiles.py;
the list at F27EE:0A3F), of which T24DF:0002 copies a hexagon, 384 of
the 576 pixels (8 in the first and last row, 24 in the two middle ones;
through the latches, the edges plane by plane), whatever the entry's
transparent value: ISLE's entries and all but three of MOON's have that
value in exactly the 192 pixels outside the hexagon, DESERT's do not.
The unit is entry type * 6 + direction of UNIT.LIB in UNIT.DAT's order
(F27EE:0A2B), the direction the unit's +0Fh or +10h by the window, which
make_unit sets to 3 for player 0 and 0 for the others; colour base 30h
for player 1, else 20h, the ground 0; the palette 00.PAL (palfiles.py,
level 252).  A unit with flag 2 in its +6 (its type's +10h, datfiles.py:
ISLE's types 17 and 18) is drawn only in its own player's window
(T0D36:00F5, as read); here it is drawn like the others.  MOON has no
.DAT files: its libraries are taken in their own order.
Checked against runs (-vram at the first map's start, 60 s; --match):
in the windows (GAME.IFF's pixels of colour 64, 24192 each) ISLE's map
00 had 23952 pixels equal in each window, the other 240 in one square
(the cursor); DESERT's map 00, from DESERT.EX2's run, 23964 in each, the
others in one square; MOON's map 00, from MOON.EXE's run, 24192 and
23808 (384 in one square).

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
import ifffiles  # noqa: E402
import libfiles  # noqa: E402
import palfiles  # noqa: E402
import png  # noqa: E402
import tpwmfiles  # noqa: E402

UNIT_TYPES = 27
UNIT_REC = 0x44
GROUND_REC = 6
NO_UNIT = 0xF4
BUILDING_FLAGS = (0x40, 0x400, 0x100)   # the SHP kinds 0, 1, 2
SQUARE = 24                             # a square's picture, 24x24
COLUMN = 16                             # from one column to the next
WINDOW_COLOUR = 64                      # GAME.IFF's pixels the maps are drawn in
OVERVIEW_BASE = 0x70                    # the colour base draw_overview gives the .PMP's entry
OVERVIEW_DOTS = 0x22                    # AMOK.DAT: a unit's colour there, by player
# the hexagon T24DF:0002 copies of a ground's picture: the first pixel of
# each of its 24 rows (the row is as much shorter on the right)
HEXAGON = (8, 7, 7, 6, 5, 4, 4, 3, 2, 1, 1, 0, 0, 1, 1, 2, 3, 4, 4, 5, 6, 7, 7, 8)


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
    """(name, entry) of a .PMP: a library of one 'U' entry"""
    entries = libfiles.read(data)
    if len(entries) != 1 or entries[0][1]['kind'] != 'U' or entries[0][1]['tail']:
        raise ValueError('.PMP not one entry of kind U')
    return entries[0]


def write_pmp(p):
    return libfiles.write([p])


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


def find(d, name):
    for f in os.listdir(d):
        if f.upper() == name:
            return os.path.join(d, f)
    return None


def sorted_entries(game, lib):
    """a library's entries in its .DAT's order, as load_lib sorts them;
    without a .DAT in the library's own"""
    libdir = find(game, 'LIB')
    entries = libfiles.read(unpacked(find(libdir, lib + '.LIB')))
    dat = find(libdir, lib + '.DAT')
    if dat is None:
        return [e for _, e in entries]
    by_name = dict(entries)
    names = unpacked(dat)
    return [by_name[names[k:k + 8]] for k in range(0, len(names), 8)]


def ground_images(game):
    """PART.LIB's pictures as the map shows them: the hexagon of each,
    its pixels whatever the entry's transparent value"""
    out = []
    for e in sorted_entries(game, 'PART'):
        w, h, px = libfiles.image(e)
        if (w, h) != (SQUARE, SQUARE):
            raise ValueError('a ground picture of %dx%d' % (w, h))
        px = [e['key'] if v < 0 else v for v in px]
        for y, first in enumerate(HEXAGON):
            for x in list(range(first)) + list(range(SQUARE - first, SQUARE)):
                px[y * w + x] = -1
        out.append((w, h, px))
    return out


def unit_images(game):
    return [libfiles.image(e) for e in sorted_entries(game, 'UNIT')]


def draw(m, parts, units):
    """(width, height, colour numbers with -1 where nothing is drawn) of
    a map as the game draws it at its start"""
    w, h = m['w'], m['h']
    width, height = COLUMN * w + SQUARE - COLUMN, SQUARE * h + SQUARE // 2
    out = [-1] * (width * height)

    def put(img, x0, y0, base):
        iw, ih, px = img
        for j in range(ih):
            o = (y0 + j) * width + x0
            for i in range(iw):
                if px[j * iw + i] >= 0:
                    out[o + i] = (px[j * iw + i] + base) & 0xFF

    for units_now in (False, True):
        for r in range(h):
            for c in range(w):
                g, u = m['squares'][r * w + c]
                x, y = COLUMN * c, SQUARE * r + (SQUARE // 2 if c & 1 else 0)
                if not units_now:
                    if g < len(parts):
                        put(parts[g], x, y, 0)
                elif u < NO_UNIT:
                    e = (u >> 1) * 6 + (0 if u & 1 else 3)
                    if e < len(units):
                        put(units[e], x, y, 0x30 if u & 1 else 0x20)
    return width, height, out


def overview(m, entry, game):
    """(width, height, colour numbers with -1 where nothing is drawn) of
    a map's overview as draw_overview draws it: the .PMP's entry and the
    units' dots"""
    width, height, px = libfiles.image(entry)
    out = [v if v < 0 else v + OVERVIEW_BASE for v in px]
    u = unpacked(find(game, 'UNIT.DAT'))
    amok = unpacked(find(game, 'AMOK.DAT'))
    for i, (_, unit) in enumerate(m['squares']):
        t = unit >> 1
        if unit >= NO_UNIT or (t + 1) * UNIT_REC > len(u):
            continue
        cls, inside = struct.unpack_from('<H2xH', u, t * UNIT_REC + 0x0C)
        if cls & 2 or inside & 2:
            continue
        x, y = 2 * (i % m['w']) + 2, 2 * (i // m['w']) + 2
        c = amok[OVERVIEW_DOTS + (unit & 1)]
        for dx, dy, d in ((0, 0, 0), (1, 0, 1), (1, 1, 2), (0, 1, 1)):
            if x + dx < width and y + dy < height:
                out[(y + dy) * width + x + dx] = c + d
    return width, height, out


def match_overview(pic, vram):
    """(pixels drawn, those equal, x, y, the box of the others or None)
    where the overview is in video memory"""
    width, height, out = pic
    screen = [vram[4 * (y * 80 + x // 4) + (x & 3)] for y in range(200) for x in range(320)]
    some = [(x, y) for y in range(0, height, 3) for x in range(0, width, 3)]
    _, ox, oy = max((sum(out[y * width + x] == screen[(oy + y) * 320 + ox + x] for x, y in some), ox, oy)
                    for oy in range(200 - height + 1) for ox in range(320 - width + 1))
    drawn = [(x, y) for y in range(height) for x in range(width) if out[y * width + x] >= 0]
    bad = [(x, y) for x, y in drawn if out[y * width + x] != screen[(oy + y) * 320 + ox + x]]
    box = (min(x for x, _ in bad), min(y for _, y in bad),
           max(x for x, _ in bad), max(y for _, y in bad)) if bad else None
    return len(drawn), len(drawn) - len(bad), ox, oy, box


def to_png(path, game, pic):
    width, height, out = pic
    pal = palfiles.read(open(find(game, '00.PAL'), 'rb').read())[0]
    rgb = [tuple(png.dac_to_rgb(v) for v in c) for c in palfiles.dac(pal, palfiles.kind(pal))]
    used = set(out)
    clear = next((i for i in range(256) if i not in used), None) if -1 in used else None
    png.write_indexed(path, width, height, bytes((clear or 0) if p < 0 else p for p in out), rgb, clear)


def match(pic, game, vram):
    """per window (left, right): (pixels of the window, the most that
    equal the picture's, the map's column and half row at the screen's
    corner, the box of the pixels that differ or None)"""
    width, height, out = pic
    frame = ifffiles.read(unpacked(find(game, 'GAME.IFF')))
    fw, fh, fp = frame['width'], frame['height'], frame['pixels']
    screen = [vram[4 * (y * 80 + x // 4) + (x & 3)] for y in range(fh) for x in range(fw)]
    res = []
    for xs in (range(0, fw // 2), range(fw // 2, fw)):
        win = [(x, y) for y in range(fh) for x in xs if fp[y * fw + x] == WINDOW_COLOUR]

        def equal(points, ox, oy):
            n = 0
            for x, y in points:
                mx, my = x + ox, y + oy
                if 0 <= mx < width and 0 <= my < height and out[my * width + mx] == screen[y * fw + x]:
                    n += 1
            return n

        some = win[::13]
        best = max((equal(some, COLUMN * c, SQUARE // 2 * k), c, k)
                   for c in range(-fw // COLUMN, width // COLUMN + 1)
                   for k in range(-fh // (SQUARE // 2), height // (SQUARE // 2) + 1))
        _, c, k = best
        ox, oy = COLUMN * c, SQUARE // 2 * k
        bad = [(x, y) for x, y in win if not (0 <= x + ox < width and 0 <= y + oy < height
                                              and out[(y + oy) * width + x + ox] == screen[y * fw + x])]
        box = (min(x for x, _ in bad), min(y for _, y in bad),
               max(x for x, _ in bad), max(y for _, y in bad)) if bad else None
        res.append((len(win), len(win) - len(bad), c, k, box))
    return res


def map_dirs(root):
    for game in ('ISLE', 'DESERT', 'MOON'):
        d = os.path.join(root, game, 'MAP')
        if os.path.isdir(d):
            yield d


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('dirs', nargs='*', metavar='MAPDIR')
    ap.add_argument('--grid', help='print this map (NN) of each folder as text')
    ap.add_argument('--png', metavar='DIR', help='write each map as a picture here')
    ap.add_argument('--match', nargs=2, metavar=('VRAM', 'NN'),
                    help="where map NN is in a run's video memory, and how much of it")
    ap.add_argument('--overview', nargs=2, metavar=('VRAM', 'NN'),
                    help="where map NN's overview is in a run's video memory, and how much of it")
    a = ap.parse_args()
    root = game_dir()
    dirs = a.dirs or list(map_dirs(root))
    total = bad = 0
    for d in dirs:
        game = os.path.dirname(os.path.abspath(d))
        ground, unit = tables(game)
        tiles = sprites = None
        if a.png or a.match:
            tiles, sprites = ground_images(game), unit_images(game)
        if a.match:
            m = read_fin(unpacked(os.path.join(d, a.match[1] + '.FIN')))
            vram = open(a.match[0], 'rb').read()
            for side, (n, same, c, k, box) in zip(('left', 'right'), match(draw(m, tiles, sprites), game, vram)):
                print("%s\\%s %s window: column %d, row %s of the map at the screen's corner; "
                      "%d of %d pixels equal%s" % (
                          os.path.basename(game), a.match[1], side, c, '%g' % (k / 2), same, n,
                          '' if box is None else ', the others within x %d..%d, y %d..%d' % (
                              box[0], box[2], box[1], box[3])))
            continue
        if a.overview:
            path = find(d, a.overview[1] + '.PMP')
            if path is None:
                print('%s\\%s no .PMP' % (os.path.basename(game), a.overview[1]))
                continue
            m = read_fin(unpacked(find(d, a.overview[1] + '.FIN')))
            pic = overview(m, read_pmp(unpacked(path))[1], game)
            n, same, x, y, box = match_overview(pic, open(a.overview[0], 'rb').read())
            print('%s\\%s overview at x %d, y %d: %d of %d pixels equal%s' % (
                os.path.basename(game), a.overview[1], x, y, same, n,
                '' if box is None else ', the others within x %d..%d, y %d..%d of it' % (
                    box[0], box[2], box[1], box[3])))
            continue
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
                if a.png:
                    os.makedirs(a.png, exist_ok=True)
                    to_png(os.path.join(a.png, '%s_%s.png' % (os.path.basename(game), nn)),
                           game, draw(m, tiles, sprites))
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
                    parts.append('.PMP %s' % p[0].rstrip(b'\0').decode('latin-1'))
                    if (p[1]['w'], p[1]['h']) != (2 * m['w'] + 4, 2 * m['h'] + 4):
                        problems.append('.PMP of %dx%d' % (p[1]['w'], p[1]['h']))
                    if a.png:
                        to_png(os.path.join(a.png, '%s_%s_overview.png' % (os.path.basename(game), nn)),
                               game, overview(m, p[1], game))
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
    if not a.match and not a.overview:
        print('%d maps, %d not read or not written back' % (total, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
