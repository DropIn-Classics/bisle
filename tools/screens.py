#!/usr/bin/env python3
"""Battle Isle's screens drawn from the game's files and a run's memory,
as BATTLE.EXE draws them, and compared with the run's video memory.

    screens.py --status RAM [--vram VRAM] [--png DIR] [--game DIR] [--load SEG]
    screens.py --unit RAM [--vram VRAM] [--png DIR] [--game DIR] [--load SEG]

RAM is a run's memory (run.py -ram), VRAM its video memory (run.py
-vram), both of the same moment.  --status draws the status screen of
each player in that player's window and says how many of the drawn
pixels the video memory has; --png writes the pictures
(DIR/status<player>.png, in 00.PAL's colours, what is not drawn clear).
--game names the game's folder (default ISLE), --load the segment the
runner loaded the program at (its report's "load ... at"; default 0077).

--unit draws the unit's screen (below) of each player whose cursor has
the state 4 (the record's +17h) in the run's memory.

The status screen (draw_status, T1479:0DC5, one argument: the player),
read from the code; x0 is 0 for player 0 and 160 for player 1:

  the window (T1479:0004): SHOP.LIB's entries 0 and 3 at (x0, 0), 1 at
    (x0 + 142, 0), 2 at (x0, 166), colour base 40h (the far pointer
    F27EE:0A7B is to the library's entries), then the rectangle x0 + 8 ..
    x0 + 142, 12 .. 166 filled with AMOK.DAT's +0Eh.
  a box (draw_box, T1479:0130: x, y, w, h, window, colour): the rectangle
    of w by h filled with the colour (not when it is negative), then a
    frame around it: the row above and the column left of it in AMOK's
    +11h, the column right of it and the row below in AMOK's +12h (the
    right column first, so the lower right corner is +12h's and the lower
    left +11h's).  The status screen's three are filled with AMOK's +10h.
  box (x0 + 29, 18, 96, 43): text 0Ah of GAME.TXT (ROUND, LEVEL, MODE,
    HIGH, ACTUAL) in AMOK's +0Dh, and 54 pixels right of it the words
    F27EE:250C +11h, +17h, (the mode), +87h, +8Bh in decimal, a line
    each; the mode is text 0Dh (MOVE) when bit 1 of the player's record
    (F27EE:26B4, 31h bytes a player) +16h is set, else text 0Ch (ATTACK).
  box (x0 + 12, 72, 128, 56): text 0Bh (ONE TWO MAP; UNIT, FACTORY,
    DEPOT); the bytes +2, +3, +4 of F27EE:243E at x0 + 66 in AMOK's +9,
    those of F27EE:2455 at x0 + 90 in AMOK's +0Ah, +8, +9, +0Ah of
    F27EE:250C at x0 + 114 in AMOK's +0Dh; y 90, 102, 114.
  box (x0 + 12, 138, 100, 24): text 0Fh (TURN, LIMIT); 48 pixels right
    of it the byte F27EE:2453 + 17h * player and below it the byte
    F27EE:2454 + 17h * player, or text 10h when that is FFh.
  SHOP.LIB's entry 5 at (x0 + 116, 138), colour base 0.

A number is drawn by draw_number (T164D:01DB) as its decimal digits from
its place on, no leading blanks; the texts and the font are txtfiles.py's.

draw_entry clips when the drawing record's +0Eh (DATA:00E4) is not 0: to
the columns from +12h up to but not +16h and the rows from +14h up to
but not +18h (T23FD:0111).  The record is taken from the run's memory;
in the run below it was 0, 0, 320, 179, so the last row of SHOP.LIB's
entry 2 (row 179) was not drawn and the map's pixels stayed there.  The
filling and the lines do not clip.

Before it calls draw_status, T0708 puts the cursor's place (its record's
+10h, +12h) to (x0 + 117, 139), in the hexagon of entry 5.  In the run
CURSOR.LIB's entry 1 was there with colour base 0; which routine draws
it is not read, the tool draws it last.

The unit's screen was checked the same way (BATTLE.EXE, ISLE's first
map, the cursor one up from each player's start onto a unit of the
player's own, fire with down held, -ram and -vram at 55 s): player 0's
T-3 SCORPION (ground 3) and player 1's SC-T PROVIDER (ground 64), all
24160 pixels drawn for each in the video memory, on both pages.  Not run:
a unit of the other player (the numbers' place then holds text 1 of
GAME.TXT, T1479:030A, or nothing when the type's word +6 has bit 4), a
unit that holds others (the word +4 with bit 40h, T1479:070B), the
message line (T11FD:0103) at the bottom.

Checked against a run (BATTLE.EXE, ISLE's first map, fire and down on an
empty square with each player's keys, -ram and -vram at 61 s): all 24160
pixels drawn for each player were in the video memory, on both pages
(the second 4000h bytes into a plane).  The values were those of a map
just begun (round 0, level 0, MOVE for player 0 and ATTACK for player 1,
6 units a player, no factory or depot, no limit); other values and a
limit were not seen in a run.
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir  # noqa: E402
import libfiles  # noqa: E402
import mapfiles  # noqa: E402
import palfiles  # noqa: E402
import png  # noqa: E402
import tpwmfiles  # noqa: E402
import txtfiles  # noqa: E402

WIDTH, HEIGHT = 320, 200
WINDOW = 160                            # a player's window, from one to the next
FAR = 0x27EE                            # the frame of the far data the game's state is in
DATA = 0x2D8D                           # the frame of DATA
RECORD = 0x00D6                         # DATA: the drawing record
STATE = 0x250C                          # F27EE: the game's state
PLAYERS = 0x26B4                        # F27EE: the players' records
PLAYER_REC = 0x31
COUNTS = 0x243E                         # F27EE: the players' counts
COUNTS_REC = 0x17
# AMOK.DAT's colours
C_ONE, C_TWO, C_TEXT, C_WINDOW, C_BOX, C_LIGHT, C_DARK = 0x09, 0x0A, 0x0D, 0x0E, 0x10, 0x11, 0x12
FRAME_BASE = 0x40
UNITS, UNIT_REC = 0x2898, 0x1A          # F27EE: the units' records
TYPES, TYPE_REC = 0x001F, 0x44          # F27EE: the unit types' records
MAP_PTR = 0x1334                        # F27EE: far pointer to the map's squares


def unpacked(path):
    data = open(path, 'rb').read()
    return tpwmfiles.unpack(data) if data[:4] == tpwmfiles.MAGIC else data


def find(d, name):
    for f in os.listdir(d):
        if f.upper() == name:
            return os.path.join(d, f)
    raise ValueError('no %s in %s' % (name, d))


class Files:
    """what the screens are drawn with, from the game's folder"""

    def __init__(self, game):
        self.game = game
        self.amok = unpacked(find(game, 'AMOK.DAT'))
        self.font = txtfiles.read_font(unpacked(find(game, 'CHAR6.DAT')))
        self.texts = txtfiles.read_texts(unpacked(find(game, 'GAME.TXT')))
        self.shop = [e for _, e in libfiles.read(unpacked(find(find(game, 'LIB'), 'SHOP.LIB')))]
        self.cursor = [e for _, e in libfiles.read(unpacked(find(find(game, 'LIB'), 'CURSOR.LIB')))]
        self.bigunit = mapfiles.sorted_entries(game, 'BIGUNIT')
        self.unit = mapfiles.sorted_entries(game, 'UNIT')
        self.ground = mapfiles.ground_images(game)


class Screen:
    """320x200 colour numbers, -1 where nothing was drawn, with the
    drawing routines of BATTLE.EXE the screens use"""

    def __init__(self, files, clip=None):
        self.f = files
        self.clip = clip                # None or (x1, y1, x2, y2) for the entries
        self.pix = [-1] * (WIDTH * HEIGHT)
        self.colour = 0

    def put(self, x, y, c):
        if 0 <= x < WIDTH and 0 <= y < HEIGHT:
            self.pix[y * WIDTH + x] = c & 0xFF

    def fill(self, x1, y1, x2, y2, c):
        """T2482:0004: the rectangle, both corners in it"""
        for y in range(min(y1, y2), max(y1, y2) + 1):
            for x in range(min(x1, x2), max(x1, x2) + 1):
                self.put(x, y, c)

    def entry(self, x, y, e, base):
        """draw_entry (T2470:0008)"""
        w, h, vals = libfiles.image(e)
        x1, y1, x2, y2 = self.clip or (0, 0, WIDTH, HEIGHT)
        for j in range(h):
            for i in range(w):
                px, py = x + e['dx'] + i, y + e['dy'] + j
                if vals[j * w + i] >= 0 and x1 <= px < x2 and y1 <= py < y2:
                    self.put(px, py, vals[j * w + i] + base)

    def hexagon(self, x, y, img):
        """draw_hexagon (T24DF:0002): a ground's picture as
        mapfiles.ground_images has it, at x, y, colour base 0"""
        w, h, px = img
        for j in range(h):
            for i in range(w):
                if px[j * w + i] >= 0:
                    self.put(x + i, y + j, px[j * w + i])

    def chars(self, x, y, lines):
        """draw_chars (T2525:000A) for each line, 6 rows a line, in the
        colour set"""
        points, _ = txtfiles.text_pixels(self.f.font, lines)
        for px, py in points:
            self.put(x + px, y + py, self.colour)

    def text(self, x, y, n, colour):
        """draw_text (T164D:0140)"""
        self.colour = colour
        self.chars(x, y, self.f.texts[n][0])

    def number(self, n, x, y):
        """draw_number (T164D:01DB)"""
        self.chars(x, y, [b'%d' % n])

    def box(self, x, y, w, h, colour):
        """draw_box (T1479:0130)"""
        a = self.f.amok
        if colour >= 0:
            self.fill(x, y, x + w - 1, y + h - 1, colour)
        self.fill(x - 1, y - 1, x + w, y - 1, a[C_LIGHT])
        self.fill(x + w, y, x + w, y + h, a[C_DARK])
        self.fill(x, y + h, x + w, y + h, a[C_DARK])
        self.fill(x - 1, y - 1, x - 1, y + h, a[C_LIGHT])
        self.colour = a[C_LIGHT]


class Ram:
    def __init__(self, data, load):
        self.data = data
        self.load = load
        self.base = (load + FAR) * 16

    def byte(self, off):
        return self.data[self.base + off]

    def word(self, off):
        return struct.unpack_from('<h', self.data, self.base + off)[0]

    def bytes(self, off, n):
        return self.data[self.base + off:self.base + off + n]

    def far(self, seg, off):
        """a byte at a segment of the run (as stored in the memory, the
        load segment in it) and an offset"""
        return self.data[seg * 16 + off]

    def clip(self):
        """the drawing record's clipping, None when it is off"""
        on, _, x1, y1, x2, y2 = struct.unpack_from('<6h', self.data, (self.load + DATA) * 16 + RECORD + 0x0E)
        return (x1, y1, x2, y2) if on else None


def status_values(ram, player):
    """what draw_status shows for a player, from the run's memory"""
    return {
        'round': ram.word(STATE + 0x11), 'level': ram.word(STATE + 0x17),
        'high': ram.word(STATE + 0x87), 'actual': ram.word(STATE + 0x8B),
        'move': bool(ram.byte(PLAYERS + PLAYER_REC * player + 0x16) & 1),
        'one': [ram.byte(COUNTS + k) for k in (2, 3, 4)],
        'two': [ram.byte(COUNTS + COUNTS_REC + k) for k in (2, 3, 4)],
        'map': [ram.byte(STATE + k) for k in (8, 9, 10)],
        'turn': ram.byte(COUNTS + COUNTS_REC * player + 0x15),
        'limit': ram.byte(COUNTS + COUNTS_REC * player + 0x16),
    }


def window(s, files, x0):
    """draw_shop_window (T1479:0004)"""
    a = files.amok
    s.entry(x0, 0, files.shop[0], FRAME_BASE)
    s.entry(x0, 0, files.shop[3], FRAME_BASE)
    s.entry(x0 + 0x8E, 0, files.shop[1], FRAME_BASE)
    s.entry(x0, 0xA6, files.shop[2], FRAME_BASE)
    s.fill(x0 + 8, 0x0C, x0 + 0x8E, 0xA6, a[C_WINDOW])


def draw_status(files, player, v, clip=None):
    """the status screen of a player as draw_status draws it, and the
    cursor in its place there"""
    s = Screen(files, clip)
    a = files.amok
    x0 = WINDOW * player
    window(s, files, x0)
    x, y = x0 + 0x1D, 0x12
    s.box(x, y, 0x60, 0x2B, a[C_BOX])
    s.text(x, y, 0x0A, a[C_TEXT])
    s.number(v['round'], x + 0x36, y + 6)
    s.number(v['level'], x + 0x36, y + 0x0C)
    s.number(v['high'], x + 0x36, y + 0x18)
    s.number(v['actual'], x + 0x36, y + 0x1E)
    s.text(x + 0x36, y + 0x12, 0x0D if v['move'] else 0x0C, a[C_TEXT])
    x, y = x0 + 0x0C, 0x48
    s.box(x, y, 0x80, 0x38, a[C_BOX])
    s.text(x, y, 0x0B, a[C_TEXT])
    for column, colour, counts in ((0x42, C_ONE, v['one']), (0x5A, C_TWO, v['two']), (0x72, C_TEXT, v['map'])):
        s.colour = a[colour]
        for row, n in zip((0x5A, 0x66, 0x72), counts):
            s.number(n, x0 + column, row)
    x, y = x0 + 0x0C, 0x8A
    s.box(x, y, 0x64, 0x18, a[C_BOX])
    s.text(x, y, 0x0F, a[C_TEXT])
    s.number(v['turn'], x + 0x30, y + 6)
    if v['limit'] < 0xFF:
        s.number(v['limit'], x + 0x30, y + 0x0C)
    else:
        s.text(x + 0x30, y + 0x0C, 0x10, a[C_TEXT])
    s.entry(x0 + 0x74, 0x8A, files.shop[5], 0)
    s.entry(x0 + 0x75, 0x8B, files.cursor[1], 0)
    return s.pix


def unit_values(ram, player):
    """what draw_unit_info shows for a player, from the run's memory: the
    unit's record (1Ah bytes at F27EE:2898), its type's (44h bytes at
    F27EE:001F) and the ground under the cursor"""
    rec = PLAYERS + PLAYER_REC * player
    pos, unit = struct.unpack_from('<H', ram.bytes(rec, 2))[0], ram.byte(rec + 0x1E)
    u = ram.bytes(UNITS + UNIT_REC * unit, UNIT_REC)
    t = ram.bytes(TYPES + TYPE_REC * u[8], TYPE_REC)
    seg, off = struct.unpack_from('<HH', ram.bytes(MAP_PTR, 4))[::-1]
    return {'state': ram.byte(rec + 0x17), 'cursor': ram.byte(rec + 0x1B), 'unit': unit, 'rec': u, 'type': t,
            'ground': ram.far(seg, off + pos), 'player': player}


def draw_unit_info(files, v, clip=None):
    """the unit's screen (T1479:05AF) for a unit that holds no others
    (the record's +4 without bit 40h; the other case is not read), as
    its routines draw it:

      the window, a box (30, 20, 96, 96) for the picture and a box
      (43, 124, 70, 40) for the numbers (draw_box, filled with AMOK's
      +10h).  The numbers (T1479:030A), for a unit of the viewer's: the
      rectangle (43, 124) to (43 + 45h, 124 + 27h) filled again, SHOP.LIB's
      entry 6 at (49, 127), text 0 of GAME.TXT at (91, 126), the numbers in
      AMOK's +0Dh at x 73 and y 126, 132, 138, 147, 153: the type's +0Ah,
      +9, +0Bh (the hit values against land, air and sea: datfiles.py),
      the record's +0 halved (the move), the type's +1 (the armour); and
      at x 97: y 126
      the type's +8 less 1 when its word +5 has bit 4, else 0, y 138 the
      same with bit 8, y 132 the type's +7 less 1.
      The picture (T1479:0C93): BIGUNIT.LIB's entry (the type's +19h, in
      the .DAT's order) at (30, 20 + the type's +18h), colour base 30h when
      the unit's word +4 has bit 1 or 2 set (bit 1: player 1's), else
      20h; the type's name (+1Ah) centred on x 78, 3 pixels a character
      a side (6 wide), at y 106, in AMOK's +0Dh.
      SHOP.LIB's entry 5 at (116, 134) and at (15, 134) (the second is
      the frame of the unit's square), in it at (16, 135) the ground's
      hexagon (PART.LIB, by the map's byte) and over it the unit's entry
      (UNIT.LIB's, type * 6 + the record's +0Fh + player), base 30h when
      the record's word +4 has bit 1, else 20h."""
    s = Screen(files, clip)
    a = files.amok
    x0 = WINDOW * v['player']
    u, t = v['rec'], v['type']
    word4 = u[4] | u[5] << 8
    window(s, files, x0)
    s.box(x0 + 30, 20, 96, 96, a[C_BOX])
    s.box(x0 + 43, 124, 70, 40, a[C_BOX])
    # T1479:030A
    x, y = x0 + 43, 124
    s.fill(x, y, x + 0x45, y + 0x27, a[C_BOX])
    x, y = x + 6, y + 2
    s.entry(x, y + 1, files.shop[6], 0)
    s.text(x + 0x2A, y, 0, a[C_TEXT])
    s.colour = a[C_TEXT]
    for dy, n in ((0, t[0x0A]), (6, t[9]), (0x0C, t[0x0B]), (0x15, u[0] >> 1), (0x1B, t[1])):
        s.number(n, x + 0x18, y + dy)
    t5 = t[5] | t[6] << 8
    s.number(t[8] - 1 if t5 & 4 else 0, x + 0x30, y)
    s.number(t[8] - 1 if t5 & 8 else 0, x + 0x30, y + 0x0C)
    s.number(t[7] - 1, x + 0x30, y + 6)
    # T1479:0C93
    x, y = x0 + 30, 20
    base = 0x30 if word4 & 3 else 0x20
    s.entry(x, y + t[0x18], files.bigunit[t[0x19]], base)
    name = t[0x1A:0x2B].split(b'\0')[0]
    s.colour = a[C_TEXT]
    s.chars(x + 0x30 - 3 * len(name), y + 0x56, [name])
    # T1479:05AF
    s.entry(x0 + 0x74, 0x86, files.shop[5], 0)
    s.entry(x0 + 15, 0x86, files.shop[5], 0)
    s.hexagon(x0 + 16, 0x87, files.ground[v['ground']])
    s.entry(x0 + 16, 0x87, files.unit[u[8] * 6 + u[0x0F + v['player']]], 0x30 if word4 & 1 else 0x20)
    s.entry(x0 + 0x75, 0x87, files.cursor[v['cursor']], 0)
    return s.pix


def compare(pix, vram, page=0):
    """(pixels drawn, those the video memory has, the box of the others
    or None)"""
    base = page * txtfiles.PAGE
    drawn = [(x, y) for y in range(HEIGHT) for x in range(WIDTH) if pix[y * WIDTH + x] >= 0]
    bad = [(x, y) for x, y in drawn if vram[4 * (base + y * 80 + x // 4) + (x & 3)] != pix[y * WIDTH + x]]
    box = (min(x for x, _ in bad), min(y for _, y in bad),
           max(x for x, _ in bad), max(y for _, y in bad)) if bad else None
    return len(drawn), len(drawn) - len(bad), box


def to_png(path, game, pix):
    pal = palfiles.read(open(find(game, '00.PAL'), 'rb').read())[0]
    rgb = [tuple(png.dac_to_rgb(c) for c in col) for col in palfiles.dac(pal, palfiles.kind(pal))]
    used = set(pix)
    clear = next((i for i in range(256) if i not in used), None) if -1 in used else None
    png.write_indexed(path, WIDTH, HEIGHT, bytes((clear or 0) if p < 0 else p for p in pix), rgb, clear)


def report(pix, vram):
    for page in (0, 1):
        n, same, box = compare(pix, vram, page)
        print('  page %d: %d pixels drawn, %d as in the video memory%s' % (
            page, n, same, '' if box is None else ', the others within x %d..%d, y %d..%d' % (
                box[0], box[2], box[1], box[3])))


def unit_main(a, files, ram, vram, game):
    shown = 0
    for player in (0, 1):
        v = unit_values(ram, player)
        if v['state'] != 4:
            continue
        shown += 1
        t = v['type']
        print('player %d: unit %d, type %d %s, ground %d' % (
            player, v['unit'], v['rec'][8], t[0x1A:0x2B].split(b'\0')[0].decode('latin-1'), v['ground']))
        pix = draw_unit_info(files, v, ram.clip())
        if vram:
            report(pix, vram)
        if a.png:
            os.makedirs(a.png, exist_ok=True)
            to_png(os.path.join(a.png, 'unit%d.png' % player), game, pix)
    if not shown:
        print('no player has the state 4')
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--status', metavar='RAM', help="draw the status screens from a run's memory")
    ap.add_argument('--unit', metavar='RAM', help="draw the unit's screens from a run's memory")
    ap.add_argument('--vram', help="compare with the run's video memory")
    ap.add_argument('--png', metavar='DIR', help='write the pictures here')
    ap.add_argument('--game', help="the game's folder (default ISLE)")
    ap.add_argument('--load', default='0077', help='the segment the program was loaded at (hex)')
    a = ap.parse_args()
    game = a.game or os.path.join(game_dir(), 'ISLE')
    files = Files(game)
    if bool(a.status) == bool(a.unit):
        ap.error('one of --status and --unit')
    ram = Ram(open(a.status or a.unit, 'rb').read(), int(a.load, 16))
    vram = open(a.vram, 'rb').read() if a.vram else None
    if a.unit:
        return unit_main(a, files, ram, vram, game)
    for player in (0, 1):
        v = status_values(ram, player)
        pix = draw_status(files, player, v, ram.clip())
        print('player %d: round %d, level %d, %s, high %d, actual %d; unit/factory/depot one %s, two %s, map %s; '
              'turn %d, limit %s' % (player, v['round'], v['level'], 'move' if v['move'] else 'attack',
                                     v['high'], v['actual'], *('/'.join(map(str, v[k])) for k in ('one', 'two', 'map')),
                                     v['turn'], 'none' if v['limit'] == 0xFF else v['limit']))
        if vram:
            for page in (0, 1):
                n, same, box = compare(pix, vram, page)
                print('  page %d: %d pixels drawn, %d as in the video memory%s' % (
                    page, n, same, '' if box is None else ', the others within x %d..%d, y %d..%d' % (
                        box[0], box[2], box[1], box[3])))
        if a.png:
            os.makedirs(a.png, exist_ok=True)
            to_png(os.path.join(a.png, 'status%d.png' % player), game, pix)
    return 0


if __name__ == '__main__':
    sys.exit(main())
