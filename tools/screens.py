#!/usr/bin/env python3
"""Battle Isle's screens drawn from the game's files and a run's memory,
as BATTLE.EXE draws them, and compared with the run's video memory.

    screens.py --status RAM [--vram VRAM] [--png DIR] [--game DIR] [--load SEG]
    screens.py --unit RAM [--vram VRAM] [--png DIR] [--game DIR] [--load SEG]
    screens.py --building RAM [--line N|clear] [--vram VRAM] [--png DIR] ...
    screens.py --menu RAM [--id N] [--sel N] [--vram VRAM] [--png DIR] ...
    screens.py --menu RAM --message position|insert [--vram VRAM] ...
    screens.py --menu RAM --message name [--typed TEXT] [--vram VRAM] ...
    screens.py --scores RAM [--hi FILE] [--map N] [--vram VRAM] [--png DIR] ...
    screens.py --stats RAM [--vram VRAM] [--png DIR] ...
    screens.py --field RAM [--line0 WHAT] [--line1 WHAT] [--vram VRAM] ...

RAM is a run's memory (run.py -ram), VRAM its video memory (run.py
-vram), both of the same moment.  --status draws the status screen of
each player in that player's window and says how many of the drawn
pixels the video memory has; --png writes the pictures
(DIR/status<player>.png, in 00.PAL's colours, what is not drawn clear).
--game names the game's folder (default ISLE), --load the segment the
runner loaded the program at (its report's "load ... at"; default 0077).

--unit draws the unit's screen (below) of each player whose cursor has
the state 4 (the record's +17h) in the run's memory.

--menu draws a menu (--id, default 3: START, OPTIONS, DISK, EXIT; the
items from the run's memory, draw_menu below): its texts alone, or with
--sel (the chosen item, which the run's memory does not show) the whole
screen: the picture MENU.IFF behind, the texts and the cursor.  Checked
against four runs (keys: space at 14 s, then from 33 s down and enter to
the menu and down to the item; -ram and -vram at 40 or 42 s): the title
menu on START, OPTIONS (menu 0) on SETTING, DISK (menu 1) on RATING and
SETTING (menu 4) on its second item: all 64000 pixels were in the video
memory on the page shown (DATA:0352).  The page drawn to (DATA:0350) had
them all in one run and a pass's redraw under way in the others (the
sprites' background put back, the texts and the cursor drawn in part).
Five more runs (space at 14 s, keys from 33 s, -ram -vram at 40 to 42 s),
each with all 64000 pixels in the video memory on the page shown: PLAYER
(menu 2, HUMAN, HUMAN, OK; --sel 0), EXIT/CANCEL (menu 5, --sel 0), a
code typed (OPTIONS, enter on its first item, then c, o, n: the item is
hidden, flag 20h, and edit_text draws CON and the sphere after it at the
item's place; --id 0 --sel 0), and LOAD's two messages (--message):
"SELECT POSITION 0 TO 9" (position; enter on LOAD) and "PLEASE INSERT
DISK" (insert; after the digit 0).  --message name draws the screen that
asks for the scores' name after a map, "TYPE NAME FOR TOP FOUR" and
below it the letters typed so far (--typed) with the sphere after them.
Checked against two runs of a map's end poked in (HANDOFF.md: the battle
against the computer, player 1's state 7 and result 0Fh, space at 70 s;
the screen is up from about 84 s; -ram -vram at 90 s with nothing typed,
at 91 s with h, a, n typed at 86, 87, 88 s): all 64000 pixels in the video
memory on the page shown.  Not seen on a screen: the mouse's menu (6),
the texts with other choices (COMPUTER, the limits).

--scores draws the scores' screen (show_scores, T1090:12B7; RATING in the
DISK menu) from a .HI file (--hi), or as it is without one, with the code
of the map --map (default the chosen map, F27EE:2523, as from RATING).
Checked against a run without a file (the DISK run's keys, enter on
RATING at 39 s, -ram -vram at 44 s): the code FIRST and four times 00000
EMPTY, all 64000 pixels in the video memory on the page shown (the other
page held neither it nor the menu: the picture alone, presumably; not
compared).  With a file, three runs, all 64000 pixels on the page shown
in each: the poked end with a name typed and enter at 89 s (-ram -vram at
95 s; the game wrote MAP\\04.HI, which the runner keeps in its -state
folder: 00495 HANS and three times 00000 EMPTY under the code EAGLE,
--map 4: after a map the code is that of the map F27EE:251B, which was 4
there), the same again over that file with another name (the same score
twice: the file's first comes first), and RATING with that file put in
as MAP\\00.HI (-put; FIRST, the same table).

--stats draws the statistics after a map (after_map, T15AC:0007;
draw_stats below) from the run's memory while they are up.  Checked
against a run (ISLE's map 03, the other player's headquarters taken after
a round: HANDOFF.md has the keys; space at 140 s, -ram -vram at 170 s):
won, RATING : 1210, ROUNDS : 1, SCALE : 1 - 1, the keyword EAGLE, two
level curves of 3 points; and against the battle with the computer lost
(HANDOFF.md's fights.keys, -ram -vram at 545 s; the screen is up from
534.6 s): MISSION NOT COMPLETED, RATING : 0, ROUNDS : 11, SCALE : 1 - 2,
curves of 22 points that fall and rise.  All 64000 pixels in the video
memory on the page shown in both.  Not seen: 32 points and more (SCALE
1 - 4), 64 and more (the rows turned round), the last map's text.

--field draws the map's whole screen (draw_field below): the picture
GAME.IFF, each player's window of his own map with its units and marks,
over it the screen the player's cursor state names (a building's, the
status, a unit's: as the options above draw them), the lines below the
windows and the cursors.  The line of a window showing the map is the
unit's under the cursor; --line0 and --line1 say otherwise (unit:N, in
hex, another unit's line; a number, in hex, show_message's text, which
is not in the memory; clear).  Checked against 44 dumps of runs (ISLE's
maps 00, 03 and 14; HANDOFF.md has their keys): the map alone in both
windows in 22 (the windows moved over the map, a unit's reach marked,
its targets marked, the marks 40h and 10h, which PATT.LIB's entries 1
and 2 show and whose meaning is not read, a unit's line for the unit
under the cursor, of the other player's too, an order's aim marked), a
building's screen in one half in 19, the status in one or both, the
unit's.  All 64000 pixels were in the video memory on the page shown in
43; in one, a depot's screen that came up when a unit moved in, 15
pixels of row 179 were another ground's: a screen over the window does
not draw that row (the clipping), which keeps the window's pixels of
before, drawn here from the map as it is.  The page drawn to had them
all in 41 (a pass under way in one; in one the aim's mark was not on
it).  The overview over a window (state 3; the picture is the .PMP of
the game's that the memory holds) in four more dumps of map 03: player
0's as it comes up, its frame moved two down and three right, both
players' at once with player 1's frame moved, and the window after fire
ended it; all 64000 pixels on both pages in each.  Six more with fire
held (state 1: the cursor's entries 1, 2, 3, 9, 0 and 4) had them all
but for a unit poked into the run for entry 4.  Ten of a move (the
reach, the path marked, the unit under way and at its aim, with
--line0 6 after the arrival) had all on the page shown.  Not seen: marks of
player 1, a unit with 2 in its +6 (drawn only for its own side), a dot
of +20h's colour, the cursor's states 5 and 6 over the map, the
squares' explosions.

--building draws the building's screen (draw_building below) of each
player whose cursor has the state 2 (+17h) in the run's memory.  Checked
against a poked run (HANDOFF.md, the headquarters with a unit) and four
runs by keys on ISLE's map 03 (the code MARSS; the cursor onto the
building by keys held 0.06 s, a square each, then fire with left held;
HANDOFF.md has the keys): player 0's headquarters with empty slots (FREE
PART), both players' headquarters at once, player 0's depot (an R-1 DEMON
in slot 0) and a factory of nobody's (a T-3 SCORPION): all 24160 pixels
drawn for each were in the video memory, on both pages.

The building's screen is drawn as its loop (T0708:2928..3C75,
BATTLE.hints at draw_building) leaves it: the chosen slot's unit, FREE
PART for an empty slot, the cursor's picture for the choice made with
fire held, and with the list of types up (the cursor record's +18h 0Ch)
that list in the slots' place, the type's big picture and PRODUCT ENERGY
with its cost.  The message line below the window is drawn with it: the
unit's line (unit_line, T11FD:0103) for a unit of the viewer's, cleared
for an empty slot.  A message there (show_message) is not in the run's
memory beyond the player's bit of F27EE:250C: with that bit set the line
is drawn only when --line names the text (its number in hex); after the
message's time the line is clear until the slot changes, which --line
clear draws.  Checked against 14 runs (ISLE's map 03, each player's depot;
map 14, code DEMON, player 1's factory; HANDOFF.md has the keys), all
25312 pixels drawn in the video memory on both pages in each but one (a
dump with fire and left held in the list: one page had them all, the
other not the list's column, a pass under way presumably): a slot chosen
by down, fire held with up (picture 4), down (8) and left (7), a repair
refused (--line 1E, and --line clear 3 s later), the list, moved by two
and scrolled by two, a unit built (in its slot, the energy less its
cost), the list left by right.  Not seen: an entry of EXP.LIB on the
unit's line (all units had 0 at +1), a record with +22h 2.

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
unit that holds others (the word +4 with bit 40h, T1479:070B).  The
unit's line below the window (unit_line, T11FD:0103, which T1479:030A
calls) is drawn too: checked against the same run for player 0 (25312
pixels with it).

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
import ifffiles  # noqa: E402
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
SUFFIXES = 0x0013                       # F27EE: st, nd, rd, th, 3 bytes each
MAKEABLE = 0x4136                       # F27EE: the types list_makeable found
MESSAGE_SEG, MESSAGES_AT, MESSAGE_REC = 0x2789, 8, 0x18  # show_message's texts
C_LINE = 0x0F                           # AMOK.DAT: the message line's background
LINE_Y = 0xBD                           # the message line's row


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
        self.char24 = [e for _, e in libfiles.read(unpacked(find(find(game, 'LIB'), 'CHAR24.LIB')))]
        self.patt = [e for _, e in libfiles.read(unpacked(find(find(game, 'LIB'), 'PATT.LIB')))]
        self.exp = [e for _, e in libfiles.read(unpacked(find(find(game, 'LIB'), 'EXP.LIB')))]
        self.bigunit = mapfiles.sorted_entries(game, 'BIGUNIT')
        self.unit = mapfiles.sorted_entries(game, 'UNIT')
        self.ground = mapfiles.ground_images(game)

    def menu_picture(self):
        return ifffiles.read(unpacked(find(self.game, 'MENU.IFF')))


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

    def unit24(self, x, y, e, base):
        """draw_unit24 (T2506:000A): an entry of 24x24, not clipped"""
        clip, self.clip = self.clip, None
        self.entry(x, y, e, base)
        self.clip = clip

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

    def message(self, n):
        """show_message's text n (F2789:0008, 18h bytes each)"""
        at = (self.load + MESSAGE_SEG) * 16 + MESSAGES_AT + MESSAGE_REC * n
        return self.data[at:at + MESSAGE_REC].split(b'\0')[0]

    def drawn_page(self):
        """the page the program draws to, 0 or 1 (DATA:0350: A000 or A400);
        the other is shown"""
        return (struct.unpack_from('<H', self.data, (self.load + DATA) * 16 + 0x350)[0] - 0xA000) // 0x400

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
            'ground': ram.far(seg, off + pos), 'player': player, 'suffixes': ram.bytes(SUFFIXES, 12)}


def message_line(s, a, player, text=None):
    """show_message (T11FD:0004: number, player): the line below a
    player's window, x0 + 5 .. x0 + 148 and y 189 .. 196, filled with AMOK's
    +0Fh, and a text (none for the number -1) centred on x0 + 76 (3 pixels
    a character a side) in colour 3 for player 0, 12h for player 1"""
    x0 = WINDOW * player
    s.fill(x0 + 5, LINE_Y, x0 + 0x94, LINE_Y + 7, a[C_LINE])
    if text is not None:
        s.colour = 0x12 if player else 3
        s.chars(x0 + 0x4C - 3 * len(text), LINE_Y, [text])


def unit_line(s, a, player, u, t, suffixes):
    """T11FD:0103 (unit, player), which T1479:030A calls after the numbers
    of a unit of the viewer's: the message line cleared, then in the
    player's colour from x = x0 + 5: at x + 6 the unit's +2 (its +3 for a
    type whose word +0Eh has bit 4), at x + 14 EXP.LIB's entry of the
    unit's +1 less 1 (none for 0, the sixth above 6; drawn with the
    clipping's last row at 200), and unless the unit's word +4 has bit 2
    its +9 ending before x + 38 with st, nd, rd or th (F27EE:0013) at
    x + 38, and the type's second name (+2Bh) at x + 52"""
    message_line(s, a, player)
    x = WINDOW * player + 5
    s.colour = 0x12 if player else 3
    s.number(u[3] if (t[0x0E] | t[0x0F] << 8) & 4 else u[2], x + 6, LINE_Y)
    if u[1]:
        clip, s.clip = s.clip, s.clip and s.clip[:3] + (HEIGHT,)
        s.entry(x + 0x0E, LINE_Y, s.f.exp[min(u[1], 6) - 1], 0)
        s.clip = clip
    if not (u[4] | u[5] << 8) & 2:
        s.number(u[9], x + 0x20 - 6 * (u[9] >= 10) - 6 * (u[9] >= 100), LINE_Y)
        k = 3 if u[9] > 3 else (u[9] - 1) & 0xFF
        s.chars(x + 0x26, LINE_Y, [suffixes[3 * k:].split(b'\0')[0]])
    s.chars(x + 0x34, LINE_Y, [t[0x2B:].split(b'\0')[0]])


def unit_numbers(s, a, x, y, u, t, line=None):
    """T1479:030A for a unit of the viewer's: the rectangle at x, y (70 by
    40) filled again, the icon, the labels and the numbers, and the unit's
    line below the window (line: the player and the suffixes)"""
    s.fill(x, y, x + 0x45, y + 0x27, a[C_BOX])
    x, y = x + 6, y + 2
    s.entry(x, y + 1, s.f.shop[6], 0)
    s.text(x + 0x2A, y, 0, a[C_TEXT])
    s.colour = a[C_TEXT]
    for dy, n in ((0, t[0x0A]), (6, t[9]), (0x0C, t[0x0B]), (0x15, u[0] >> 1), (0x1B, t[1])):
        s.number(n, x + 0x18, y + dy)
    t5 = t[5] | t[6] << 8
    s.number(t[8] - 1 if t5 & 4 else 0, x + 0x30, y)
    s.number(t[8] - 1 if t5 & 8 else 0, x + 0x30, y + 0x0C)
    s.number(t[7] - 1, x + 0x30, y + 6)
    if line:
        unit_line(s, a, line[0], u, t, line[1])


def unit_picture(s, a, x, y, t, base):
    """T1479:0C93: the type's big picture and name"""
    s.entry(x, y + t[0x18], s.f.bigunit[t[0x19]], base)
    name = t[0x1A:0x2B].split(b'\0')[0]
    s.colour = a[C_TEXT]
    s.chars(x + 0x30 - 3 * len(name), y + 0x56, [name])


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
    unit_numbers(s, a, x0 + 43, 124, u, t, (v['player'], v['suffixes']))
    unit_picture(s, a, x0 + 30, 20, t, 0x30 if word4 & 3 else 0x20)
    # T1479:05AF
    s.entry(x0 + 0x74, 0x86, files.shop[5], 0)
    s.entry(x0 + 15, 0x86, files.shop[5], 0)
    s.hexagon(x0 + 16, 0x87, files.ground[v['ground']])
    s.entry(x0 + 16, 0x87, files.unit[u[8] * 6 + u[0x0F + v['player']]], 0x30 if word4 & 1 else 0x20)
    s.entry(x0 + 0x75, 0x87, files.cursor[v['cursor']], 0)
    return s.pix


def building_values(ram, player):
    """what the building's screen (T1479:0AA6) shows for a player, from the
    run's memory: the record the player's cursor record (+24h, +26h) points
    to, its 7 slots for the player, its flags byte +19h, and the units'
    records the slots name"""
    rec = PLAYERS + PLAYER_REC * player
    off, seg = struct.unpack_from('<HH', ram.bytes(rec + 0x24, 4))
    b = [ram.far(seg, off + k) for k in range(0x1A)]
    slots = [ram.far(seg, off + 7 * player + i) for i in range(7)]
    units = {n: ram.bytes(UNITS + UNIT_REC * n, UNIT_REC) for n in slots if n <= 0xF0}
    mode = ram.byte(rec + 0x18)
    # with the list of types up (+18h 0Ch) +0Eh is the place in the list and
    # +0Ah the slot it was called from
    sel = ram.word(rec + (0x0A if mode == 0x0C else 0x0E))
    sel_unit = ram.bytes(UNITS + UNIT_REC * slots[sel], UNIT_REC) if slots[sel] <= 0xF0 else None
    first = ram.word(rec + 0x20)
    listed = ram.byte(MAKEABLE + first + ram.word(rec + 0x0E))
    return {'state': ram.byte(rec + 0x17), 'mode': mode, 'flags': b[0x19], 'slots': slots,
            'units': units, 'player': player, 'kind': ram.word(rec + 0x22), 'sel': sel, 'value': b[0x16 + player],
            'cursor': ram.byte(rec + 0x1B), 'cursor_at': (ram.word(rec + 0x10), ram.word(rec + 0x12)),
            'shown': ram.word(STATE) & (8 if player else 4) == 0,
            'type': ram.bytes(TYPES + TYPE_REC * sel_unit[8], TYPE_REC) if sel_unit else None,
            'suffixes': ram.bytes(SUFFIXES, 12), 'line': None,
            'list': list(ram.bytes(MAKEABLE + first, 7)), 'list_second': player,
            'listed': listed, 'listed_type': ram.bytes(TYPES + TYPE_REC * listed, TYPE_REC) if listed != 0xFF else None}


def bar(s, x, y, colour, back, h, value, size):
    """LT164D_028D: a bar of size / 2 pixels in the colour back (when not
    0) and over it value / 2 + 1 pixels in colour, h + 1 rows"""
    value, size = value >> 1, size >> 1
    if back:
        s.fill(x, y, x + size, y + h, back)
    if 0 < value <= size:
        s.fill(x, y, x + value, y + h, colour)


def draw_building(files, v, clip=None):
    """the building's screen (T1479:0AA6: player, the building's record) as
    its routines draw it, read from the code:

      the window, a box (44, 22, 96, 96) and a box (56, 125, 72, 40) filled
      with AMOK's +10h (the second holds the numbers of the unit in slot 0,
      which T0708 draws later: not drawn here).  The slots (T1479:0931: x,
      y, player, the record's far pointer): seven, from (16, 12) down in
      steps of 24, each SHOP.LIB's entry 4 at colour base 40h and, when the
      slot's byte (the record's +7 * player + slot) is not above F0h, the
      unit of that number (UNIT.LIB's entry type * 6 + 1, base 30h for
      player 1, 20h for 0; 30h when the unit's word +4 has bit 1, 20h when
      neither bit 1 nor bit 2) and, when that word has bit 200h and
      T0D36:00F5 (player, word) is not 0, or has bit 2, PATT.LIB's entry 2
      at the slot, base 0 (an arrow; what the bit 200h means is not read).
      The title (draw_text, x 44 and y 13): GAME.TXT's text 3, 5, 4 or 6 by
      the record's +19h bit 4, 8, 10h, else, in AMOK's +0Bh.  A player's
      screen is 160 pixels right of player 0's.

      What T0708 draws after it (T0708:2AF0, 2D91, once per change of the
      chosen slot, the cursor record's +0Eh): the big box filled again,
      the numbers box (T1479:030A, when the player's bit 4 or 8 of
      F27EE:250C is clear) and the big picture (T1479:0C93, colour base 30h
      when the unit's word +4 has bit 1, or bit 2 with player 1) of the
      unit in the chosen slot; for a record with +22h 1 (the headquarters,
      presumably) a bar (LT164D:028D) right of the title, the value
      (record +16h + player) / 7 + 1 of 35 half-pixels wide, and the value
      itself as a number in a cleared box; last the cursor, CURSOR.LIB's
      entry of +1Bh at the record's +10h, +12h.  In the numbers' place,
      filled again (T1479:028E: 70 by 40 in AMOK's +10h): GAME.TXT's text
      2 (FREE PART) for an empty slot, text 8 or 9 for a unit whose word
      +4 has bit 400h or 800h (read, not seen in a run).  The message
      line: unit_line for the unit (by T1479:030A), cleared for an empty
      slot or such a unit; v['line'] (a text, or b'' for none) instead
      when given.  With the list of types up (v['mode'] 0Ch; T1479:0B9B and
      T0708:37AA): the seven slots again with the types from F27EE:4136
      (UNIT.LIB's entry type * 6 + 1, base 30h for player 1), the big box
      filled, and for the type at the cursor the numbers' place filled,
      text 7 there, the type's +3Eh as a number at +1Eh, +18h and the big
      picture.  Not seen: a record with +22h 2 (a unit that holds
      others; the headquarters, the depots and the factories had 1)."""
    s = Screen(files, clip)
    a = files.amok
    x0 = WINDOW * v['player']
    window(s, files, x0)
    s.box(x0 + 0x2C, 0x16, 0x60, 0x60, a[C_BOX])
    s.box(x0 + 0x38, 0x7D, 0x48, 0x28, a[C_BOX])
    y = 0x0C
    for n in v['slots']:
        s.entry(x0 + 0x10, y, files.shop[4], FRAME_BASE)
        if n <= 0xF0:
            u = v['units'][n]
            word4 = u[4] | u[5] << 8
            base = 0x30 if v['player'] else 0x20
            if word4 & 1:
                base = 0x30
            elif not word4 & 2:
                base = 0x20
            s.entry(x0 + 0x10, y, files.unit[u[8] * 6 + 1], base)
            if word4 & 0x200 and (not word4 & 1 if v['player'] == 0 else bool(word4 & 1)) or word4 & 2:
                s.entry(x0 + 0x10, y, files.patt[2], 0)
        y += 0x18
    f = v['flags']
    s.text(x0 + 0x2C, 0x0D, 3 if f & 4 else 5 if f & 8 else 4 if f & 0x10 else 6, a[0x0B])
    # what T0708 draws then (T0708:2AF0, once the slot is chosen)
    s.fill(x0 + 0x2C, 0x16, x0 + 0x8B, 0x75, a[C_BOX])
    sel = v['slots'][v['sel']]
    if sel <= 0xF0:
        u, t = v['units'][sel], v['type']
        word4 = u[4] | u[5] << 8
        if word4 & 0xC00:
            s.fill(x0 + 0x38, 0x7D, x0 + 0x38 + 0x45, 0x7D + 0x27, a[C_BOX])
            s.text(x0 + 0x38, 0x7D, 8 if word4 & 0x400 else 9, a[C_TEXT])
            message_line(s, a, v['player'])
        else:
            # with a message up (the player's bit of F27EE:250C) the numbers are
            # not drawn again, and neither is the unit's line: drawn here as
            # left from the pass before
            unit_numbers(s, a, x0 + 0x38, 0x7D, u, t, (v['player'], v['suffixes']) if v['shown'] else None)
        flag = 1 if word4 & 1 else v['player'] if word4 & 2 else 0
        unit_picture(s, a, x0 + 0x2C, 0x16, t, 0x30 if flag else 0x20)
    else:
        s.fill(x0 + 0x38, 0x7D, x0 + 0x38 + 0x45, 0x7D + 0x27, a[C_BOX])
        s.text(x0 + 0x38, 0x7D, 2, a[C_TEXT])
        message_line(s, a, v['player'])
    if v['line'] is not None:
        message_line(s, a, v['player'], v['line'] or None)
    if v['kind'] == 1:
        x = x0 + 0x2C
        bar(s, x + 0x7A - 0x2C, 0x0D, a[0x13], a[0x14], 5, v['value'] // 7 + 1, 0x23)
        bar(s, x + 0x32, 0x0D, a[0x0E], a[0x0E], 6, 0x1E, 0x23)
        s.colour = a[0x13]
        s.number(v['value'], x + 0x32, 0x0D)
    if v['mode'] == 0x0C:
        # the list of types (T1479:0B9B) over the slots, and what T0708:37AA
        # draws for the type at the cursor
        y = 0x0C
        for n in v['list']:
            s.entry(x0 + 0x10, y, files.shop[4], FRAME_BASE)
            if n != 0xFF:
                s.entry(x0 + 0x10, y, files.unit[n * 6 + 1], 0x30 if v['list_second'] else 0x20)
            y += 0x18
        s.fill(x0 + 0x2C, 0x16, x0 + 0x8B, 0x75, a[C_BOX])
        if v['listed'] != 0xFF:
            t = v['listed_type']
            s.fill(x0 + 0x38, 0x7D, x0 + 0x38 + 0x45, 0x7D + 0x27, a[C_BOX])
            s.text(x0 + 0x38, 0x7D, 7, a[C_TEXT])
            s.number(t[0x3E], x0 + 0x38 + 0x1E, 0x7D + 0x18)
            unit_picture(s, a, x0 + 0x2C, 0x16, t, 0x30 if v['player'] & 1 else 0x20)
    s.entry(*v['cursor_at'], files.cursor[v['cursor']], 0)
    return s.pix


MENU_SEG = 0x2740                       # the frame of the menus' data (T1090's DS)
MENUS, ITEMS, ITEM_REC = 0x79, 0xA3, 0x2E


def menu_text(codes):
    """a text of the menus as draw_text24 (T164D:000C) reads it: each byte
    less 3 an entry of CHAR24.LIB, 1 a gap, 0 ends"""
    return list(codes.split(b'\0')[0])


def text24(s, x, y, codes, base=0):
    """draw_text24: 24 pixels on a character"""
    for c in menu_text(codes):
        if c != 1:
            s.entry(x, y, s.f.char24[c - 3], base)
        x += 0x18
    return x


def menu_values(ram, menu):
    """the items of a menu (T1090:0EDE) from the run's memory: the menu's
    record (6 bytes at F2740:0079: the item numbers, +5 their count), each
    item 2Eh bytes from F2740:00A3: a word of flags (bit 20h: not drawn),
    its texts of 10 bytes from +2, the one chosen by +2Bh"""
    base = (ram.load + MENU_SEG) * 16
    rec = ram.data[base + MENUS + 6 * menu:base + MENUS + 6 * menu + 6]
    items = []
    for k in rec[:rec[5]]:
        at = base + ITEMS + ITEM_REC * k
        flags = struct.unpack_from('<H', ram.data, at)[0]
        items.append({'item': k, 'flags': flags, 'text': ram.data[at + 2 + 10 * ram.data[at + 0x2B]:at + 12 + 10 * ram.data[at + 0x2B]],
                      'field': ram.data[at + 2:at + 12]})
    return items


# the texts LOAD shows, each (offset in F2740, x, y)
MESSAGES = {
    'position': [(0x2B, 0x40, 0x32), (0x34, 0x40, 0x54), (0x3D, 0x40, 0x76)],   # ask_position, T1090:11AC
    'insert': [(0x46, 0x57, 0x37), (0x4D, 0x57, 0x5B), (0x54, 0x57, 0x7F)],     # T1090:07CE
    'name': [(0x5B, 0x3A, 0x14), (0x65, 0x80, 0x38), (0x6F, 0x46, 0x5C)],       # the scores' name, T1090:0381
}


def message_values(ram, which):
    base = (ram.load + MENU_SEG) * 16
    return [(ram.data[base + at:base + at + 10], x, y) for at, x, y in MESSAGES[which]]


def draw_message(files, texts, typed=None):
    """LOAD's messages: over the menu's picture (restore_sprites takes the
    menu's texts and the cursor away) three texts by draw_text24.  The
    name for the scores (typed not None; T1090:0381) has below them what
    was typed so far, by edit_text at x 102 (66h), y 164 (A4h): the
    letters (a letter's byte is its key's less 50h, 17 for A) and
    CHAR24.LIB's entry 40, the sphere, right after them.  The letters are
    in a block the menu allocates, found by a local only: the tool is
    told them."""
    s = Screen(files)
    pic = files.menu_picture()
    for y in range(HEIGHT):
        s.pix[y * WIDTH:(y + 1) * WIDTH] = pic['pixels'][y * pic['width']:y * pic['width'] + WIDTH]
    for codes, x, y in texts:
        text24(s, x, y, codes)
    if typed is not None:
        codes = bytes(ord(c) - ord('A') + 17 for c in typed.upper()) + b'\0'
        s.entry(text24(s, 0x66, 0xA4, codes), 0xA4, files.char24[40], 0)
    return s.pix


def draw_menu(files, items, sel=None):
    """a menu's screen as a pass of the menu's loop (T1090:000E) draws it:
    the picture MENU.IFF behind (its upper left 320 by 200; the sprites'
    background is put back from it before each pass, T2467:0006), the
    items' texts (draw_menu, T1090:0EDE) at x 100 (64h), y 50 (32h) and 34
    (22h) more for each item, whether it is drawn or not (an item with
    flag 20h leaves its row empty), then the cursor, CHAR24.LIB's entry
    40 (a sphere), at x 66 (42h) and the chosen item's y.  sel is the
    chosen item (a local of the loop, not in the run's memory to find);
    None draws the texts alone."""
    s = Screen(files)
    if sel is not None:
        pic = files.menu_picture()
        for y in range(HEIGHT):
            s.pix[y * WIDTH:(y + 1) * WIDTH] = pic['pixels'][y * pic['width']:y * pic['width'] + WIDTH]
    y = 0x32
    for it in items:
        if not it['flags'] & 0x20:
            text24(s, 0x64, y, it['text'])
        y += 0x22
    if sel is not None:
        s.entry(0x42, 0x32 + 0x22 * sel, files.char24[40], 0)
        if items[sel]['flags'] & 0x20:
            # the chosen item is a field (edit_text, T1090:0F82, after the
            # cursor): what was typed, the item's text from +2, and
            # CHAR24.LIB's entry 40 once more right after it
            s.entry(text24(s, 0x64, 0x32 + 0x22 * sel, items[sel]['field']), 0x32 + 0x22 * sel, files.char24[40], 0)
    return s.pix


def scores_values(ram, files, hi=None, number=None):
    """what show_scores shows: the code of a map, the first 5 bytes of its
    record (10 bytes) in CODES.DAT, and the table of load_scores: four
    longs, then from +10h four names of 6 bytes; from a .HI file's bytes,
    or as load_scores has it without a file: the scores 0 and each name
    F2740:0010's 6 bytes.  The map is show_scores' second argument:
    F27EE:2523 (the map chosen) from RATING, the default here, and
    F27EE:251B (the .HI file's number) after a map."""
    base = (ram.load + MENU_SEG) * 16
    if hi is None:
        hi = bytes(16) + ram.data[base + 0x10:base + 0x16] * 4
    scores = [min(max(n, 0), 0x7EF4) for n in struct.unpack_from('<4l', hi)]
    names = [hi[0x10 + 6 * i:0x16 + 6 * i] for i in range(4)]
    if number is None:
        number = ram.word(0x2523)
    code = unpacked(find(files.game, 'CODES.DAT'))[10 * number:10 * number + 5]
    return {'map': number, 'code': code, 'scores': scores, 'names': names}


def draw_scores(files, v):
    """show_scores (T1090:12B7): over the menu's picture the map's code at
    (110, 22) and the four from the highest score down (equal ones in the
    table's order), the score in 5 digits (a digit's byte is it plus 5) at
    x 18 and the name at x 174, y 66 and 34 more each"""
    s = Screen(files)
    pic = files.menu_picture()
    for y in range(HEIGHT):
        s.pix[y * WIDTH:(y + 1) * WIDTH] = pic['pixels'][y * pic['width']:y * pic['width'] + WIDTH]
    text24(s, 0x6E, 0x16, v['code'])
    order = sorted(range(4), key=lambda i: -v['scores'][i])
    for row, i in enumerate(order):
        y = 0x42 + 0x22 * row
        text24(s, 0xAE, y, v['names'][i])
        text24(s, 0x12, y, bytes(int(c) + 5 for c in '%05d' % v['scores'][i]))
    return s.pix


def s16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


def line(s, x1, y1, x2, y2, colour):
    """T2541:0004, a line between the drawing record's two points: from
    the left point on, a pixel a column when it is wider than high (or as
    wide, going up: then one column more), else a pixel a row (as wide,
    going down: one row more); the other coordinate goes one on whenever
    a 16-bit sum of (the smaller width * 65536 / the larger) overflows.
    The right point itself is not drawn but in the two cases of one
    more.  A level line is draw_row's, an upright one draw_column's (both
    with their ends)."""
    if x1 == x2 or y1 == y2:
        s.fill(x1, y1, x2, y2, colour)
        return
    if x2 < x1:
        x1, y1, x2, y2 = x2, y2, x1, y1
    dx, dy = x2 - x1, abs(y2 - y1)
    sy = 1 if y2 > y1 else -1
    x, y, acc = x1, y1, 0
    if dx < dy or dx == dy and sy > 0:
        if dx == dy:
            dy += 1
        frac = (dx << 16) // dy & 0xFFFF
        for _ in range(dy):
            s.put(x, y, colour)
            y += sy
            acc += frac
            if acc > 0xFFFF:
                acc &= 0xFFFF
                x += 1
    else:
        if dx == dy:
            dx += 1
        frac = (dy << 16) // dx & 0xFFFF
        for _ in range(dx):
            s.put(x, y, colour)
            x += 1
            acc += frac
            if acc > 0xFFFF:
                acc &= 0xFFFF
                y += sy


def along(s, x1, y1, x2, y2, entry):
    """T15AC:095B: an entry at every x from x1 to x2, its y from y1 by the
    slope as a long of 16.16 (the division cut off towards 0, of the
    product the upper word)"""
    if x2 == x1:
        x2 += 1
    d = (y2 - y1) << 16
    slope = abs(d) // (x2 - x1) * (1 if d >= 0 else -1)
    for x in range(x1, x2 + 1):
        s.entry(x, s16((slope * (x - x1) >> 16) + y1), entry, 0)


def curve(s, x, y, row, entry, colour, top):
    """T15AC:07EC: a player's curve from x, y: a point every 16 pixels for
    fewer than 16 points, every 8 for fewer than 32, else every 4; a
    point's height its count * (400000h / the highest count of both
    players, 40h for 0) >> 16, up from y; between two points the line in
    the colour and over it the entry at every x"""
    step = 4 if len(row) >= 0x20 else 8 if len(row) >= 0x10 else 0x10
    scale = 0x400000 // (top or 0x40)
    height = [s16(c * scale >> 16) for c in row]
    for i in range(1, len(row)):
        y1, y2 = y - height[i - 1], y - height[i]
        line(s, x, y1, x + step, y2, colour)
        along(s, x, y1, x + step, y2, entry)
        x += step


HISTORY = 0x248E                        # F27EE: the record of history_add


def stats_values(ram, files):
    """what after_map shows, from the run's memory while its screen is up:
    the number of points (F27EE:2497) and the two rows (the far pointers
    F27EE:248F and 2493; T15AC:071F has put them in order by then), the
    score (the word F27EE:2597), the rounds (251D), whether the map is won
    (F27EE:250C bit 10h), the map it was (251B) and the one that follows
    (2523), and CODES.DAT's records of both"""
    n = ram.byte(HISTORY + 9)
    rows = []
    for at in (HISTORY + 1, HISTORY + 5):
        off, seg = struct.unpack_from('<HH', ram.bytes(at, 4))
        rows.append([ram.far(seg, off + i) for i in range(n)])
    codes = unpacked(find(files.game, 'CODES.DAT'))
    old, new = ram.word(0x251B), ram.word(0x2523)
    return {'rows': rows, 'score': ram.word(0x2597) & 0xFFFF, 'rounds': ram.word(0x251D) & 0xFFFF,
            'won': bool(ram.word(STATE) & 0x10), 'old': old, 'new': new,
            'last': bool(codes[10 * old + 8] & 1), 'code': bytes(c + 0x30 for c in codes[10 * new:10 * new + 5])}


def draw_stats(files, v):
    """the statistics after a map (after_map, T15AC:0007), read from the
    code: the picture STATS.IFF, the two curves (player 0's from x 29, y
    72 with STATS.LIB's entry 0 and colour 3, player 1's from y 168 with
    entry 1 and colour 12h), and the texts of GAME.TXT in AMOK's +0Dh, the
    values in AMOK's +0Bh: at y 86 text 1Ch (RATING) at x 40 and the score
    at x 94, text 13h (ROUNDS) at 130 and the rounds at 184, text 14h
    (SCALE) at 210 and text 15h, 16h or 17h (fewer than 16 points, fewer
    than 32, more) at 258; at (40, 182) for a map won text 12h and the
    following map's code (CODES.DAT's letters plus 30h) at x 262, or text
    18h after the last map (the record's +8 bit 0), for one not won text
    11h."""
    s = Screen(files)
    a = files.amok
    pic = ifffiles.read(unpacked(find(files.game, 'STATS.IFF')))
    for y in range(HEIGHT):
        s.pix[y * WIDTH:(y + 1) * WIDTH] = pic['pixels'][y * pic['width']:y * pic['width'] + WIDTH]
    stats = [e for _, e in libfiles.read(unpacked(find(find(files.game, 'LIB'), 'STATS.LIB')))]
    top = max(max(row) if row else 0 for row in v['rows'])
    curve(s, 0x1D, 0x48, v['rows'][0], stats[0], 3, top)
    curve(s, 0x1D, 0xA8, v['rows'][1], stats[1], 0x12, top)
    if not v['won']:
        s.text(0x28, 0xB6, 0x11, a[C_TEXT])
    elif v['last']:
        s.text(0x28, 0xB6, 0x18, a[C_TEXT])
    else:
        s.text(0x28, 0xB6, 0x12, a[C_TEXT])
        s.colour = a[0x0B]
        s.chars(0x28 + 0xDE, 0xB6, [v['code']])
    s.text(0x28, 0x56, 0x1C, a[C_TEXT])
    s.colour = a[0x0B]
    s.number(v['score'], 0x5E, 0x56)
    s.text(0x82, 0x56, 0x13, a[C_TEXT])
    s.colour = a[0x0B]
    s.number(v['rounds'], 0xB8, 0x56)
    n = len(v['rows'][0])
    s.text(0xD2, 0x56, 0x14, a[C_TEXT])
    s.text(0x102, 0x56, 0x17 if n >= 0x20 else 0x16 if n >= 0x10 else 0x15, a[0x0B])
    return s.pix


def stats_main(a, files, ram, vram, game):
    v = stats_values(ram, files)
    print('%s, score %d, rounds %d, map %d then %d (%s)%s; %d points: %s / %s' % (
        'won' if v['won'] else 'not won', v['score'], v['rounds'], v['old'], v['new'], v['code'].decode('latin-1'),
        ', the last map' if v['last'] else '', len(v['rows'][0]), *(' '.join(map(str, r)) for r in v['rows'])))
    pix = draw_stats(files, v)
    if vram:
        report(pix, vram, ram)
    if a.png:
        os.makedirs(a.png, exist_ok=True)
        to_png(os.path.join(a.png, 'stats.png'), game, pix)
    return 0


MAPS = 0x4152                           # F27EE: far pointers to the two players' maps
MAP_WIDTH = 0x246E                      # F27EE: the map's width in squares
MARKS = 0x1339                          # F27EE: the squares' marks, 64 a row
COLUMNS, ROWS = 9, 7                    # a window's squares


def same_side(a, b):
    """T0D36:00F5 (a, b; two units' +4, or a player for a)"""
    return bool(b & 1) if a & 1 else bool(b & 2) if a & 2 else not b & 1


def field_window(s, ram, player):
    """draw_window (T0E9B:0A9B: the window's first square, the player) and
    the marks over it (T0E9B:0C65)"""
    f = s.f
    rec = PLAYERS + PLAYER_REC * player
    first = ram.word(rec + 2)
    width = ram.word(MAP_WIDTH)
    off, seg = struct.unpack_from('<HH', ram.bytes(MAPS + 4 * player, 4))
    squares = [(col, row, WINDOW * player + 16 * col, (12 if col & 1 else 0) + 24 * row)
               for col in range(COLUMNS) for row in range(ROWS)]
    for col, row, x, y in squares:
        at = off + first + 2 * col + 2 * width * row
        s.hexagon(x, y, f.ground[ram.far(seg, at)])
        n = ram.far(seg, at + 1)
        if n > 0xF0:
            continue
        u = ram.bytes(UNITS + UNIT_REC * n, UNIT_REC)
        word4, word6 = struct.unpack_from('<HH', u, 4)
        if word6 & 2 and not same_side(word4, player):
            continue
        s.unit24(x, y, f.unit[u[8] * 6 + u[0x0F + player]], 0x30 if word4 & 1 else 0x20)
    at = (first >> 1) % width + ((first >> 1) // width << 6)
    for col, row, x, y in squares:
        m = ram.byte(MARKS + at + col + 0x40 * row) & (0xAA if player else 0x55)
        if m & 0xC0:
            s.unit24(x, y, f.patt[1], 0)
        elif m & 0x30:
            s.unit24(x, y, f.patt[2], 0)
        if m & 0x0F:
            s.unit24(x, y, f.patt[0], 0x30 if player else 0x20)


def loaded_pmp(files, ram):
    """the entry of the map's .PMP: the one of the game's whose bytes the
    run's memory holds (the map's loop keeps the pointer in its frame)"""
    d = find(files.game, 'MAP')
    for f in sorted(os.listdir(d)):
        if f.upper().endswith('.PMP'):
            data = unpacked(os.path.join(d, f))
            if data[4:] in ram.data:
                return mapfiles.read_pmp(data)[1]
    raise ValueError("none of the game's .PMP files is in the memory")


def draw_overview(files, ram, player):
    """the overview over a player's window (T0708:2202, state 3):
    draw_shop_window, then draw_overview (T0E9B:0931) at the cursor
    record's +4, +6: the .PMP's entry at base 70h and a dot for each of
    the unit records 0..F0h without 8000h, 4000h or 2 in its +4 and
    without 2 in its +6, at the square its +0Bh (player 1's window:
    +0Dh) names in the player's map, 2 pixels a square and 2 around
    (overview_dot: the colour, +1 right and below, +2 at the fourth; the
    colour AMOK's +22h by the unit's owner, +20h by the owner for a unit
    of the window's player's side with 200h in its +4)"""
    s = Screen(files, ram.clip())
    a = files.amok
    rec = PLAYERS + PLAYER_REC * player
    x, y = ram.word(rec + 4), ram.word(rec + 6)
    window(s, files, WINDOW * player)
    s.entry(x, y, loaded_pmp(files, ram), mapfiles.OVERVIEW_BASE)
    width = ram.word(MAP_WIDTH)
    for n in range(0xF1):
        u = ram.bytes(UNITS + UNIT_REC * n, UNIT_REC)
        word4, word6 = struct.unpack_from('<HH', u, 4)
        if word4 & 0xC002 or word6 & 2:
            continue
        square = (struct.unpack_from('<h', u, 0x0B + 2 * player)[0] - 1) >> 1
        owner = word4 & 1
        c = a[(0x20 if word4 & 0x200 and same_side(owner, player) else 0x22) + owner]
        px, py = x + 2 * (square % width) + 2, y + 2 * (square // width) + 2
        for dx, dy, d in ((0, 0, 0), (1, 0, 1), (1, 1, 2), (0, 1, 1)):
            s.put(px + dx, py + dy, c + d)
    return s.pix


def cursor_unit(ram, player):
    """T0708:1607..17FA: the unit whose line the map's loop shows for a
    player, or None"""
    rec = PLAYERS + PLAYER_REC * player
    if ram.byte(rec + 0x17) or ram.word(STATE) & (8 if player else 4):
        return None
    off, seg = struct.unpack_from('<HH', ram.bytes(MAPS + 4 * player, 4))
    n = ram.far(seg, off + ram.word(rec) + 1)
    if n > 0xF0:
        return None
    word4, word6 = struct.unpack_from('<HH', ram.bytes(UNITS + UNIT_REC * n, UNIT_REC), 4)
    if word6 & 4 or word6 & 2 and bool(word4 & 1) != bool(player):
        return None
    return n - 1 if word4 & 0x40 and not word4 & 0x80 else n


def draw_field(files, ram, lines=(None, None)):
    """the map's whole screen as the map's loop (T0708) leaves it, read
    from the code:

      the picture GAME.IFF (the frame around the two windows and the two
      message lines below them);
      for each player by the cursor record's state (+17h): 2 the
      building's screen, 4 the status screen (+18h 7) or the unit's (+18h
      6), each as above; 3 (+18h 5) the overview (draw_overview above:
      its picture at the record's +4, +6, which the loop sets to x0 + 77 -
      (the map's width + 2), 96 - (its height + 2) when the state comes
      up; the record's +0Ch, +0Eh are then the square the window is to
      begin at, moved by two with a direction, held from 0 to the width -
      10 and the height - 8, and the cursor, CURSOR.LIB's entry 6, a
      frame, is at the picture's place + twice that; fire ends it with
      the window there); else the player's window: 9 columns of 7 squares
      from the square the record's +2 names (an offset into the player's
      own map, the far pointer F27EE:4152 + 4 * player, two bytes a
      square), a column 16 pixels right of the one before and the odd
      ones 12 lower, a square 24 lower than the one above; the ground's
      hexagon (PART.LIB), then the unit (UNIT.LIB's entry type * 6 + the
      record's +0Fh + player, by draw_unit24, which does not clip: the
      odd columns' last row is 179; base 30h with bit 1 in its +4, else
      20h) unless it has 2 in its +6 and is not of the window's player's
      side (same_side);
      over the window the marks (T0E9B:0C65; F27EE:1339, 64 bytes a row of
      the map, the bits 55h player 0's, AAh player 1's): PATT.LIB's entry
      1 for a bit of C0h, else entry 2 for one of 30h, both at base 0, and
      entry 0 at base 20h (player 1: 30h) for one of 0Fh;
      the message line below a window (T0708:1607..1829): in a pass
      without a direction or fire, the cursor in state 0 on a unit of
      the player's map (the square the record's +0 names) and no message
      up (the player's bit 4 or 8 of F27EE:250C): that unit's line
      (unit_line; the unit before it for one with 40h and not 80h in its
      +4, the second square of two), unless the unit has 4 in its +6, or
      2 there and is the other player's; in the attack phase (the
      record's +16h bit 2) for a unit of the player's side with an aim
      (+11h) that square is drawn again with the mark 4 or 8 set for the
      call (T0E9B:125C: PATT.LIB's entry 0 over it), and a timer of 5
      passes started.  lines[player] says otherwise: b'' cleared,
      ('unit', n) unit n's line, else a text (show_message); a screen
      over the window draws its own line;
      the cursor, CURSOR.LIB's entry of the record's +1Bh at its +10h,
      +12h, base 0 (the map loop's end, T0708:4385).

    -> the pixels and what each half shows"""
    s = Screen(files, ram.clip())
    a = files.amok
    pic = ifffiles.read(unpacked(find(files.game, 'GAME.IFF')))
    for y in range(HEIGHT):
        s.pix[y * WIDTH:(y + 1) * WIDTH] = pic['pixels'][y * pic['width']:y * pic['width'] + WIDTH]
    shows = []
    for player in (0, 1):
        rec = PLAYERS + PLAYER_REC * player
        state, mode = ram.byte(rec + 0x17), ram.byte(rec + 0x18)
        over = None
        field_window(s, ram, player)
        if state == 2:
            shows.append('a building')
            over = draw_building(files, building_values(ram, player), ram.clip())
        elif state == 4 and mode == 7:
            shows.append('the status')
            over = draw_status(files, player, status_values(ram, player), ram.clip())
        elif state == 4 and mode == 6:
            shows.append('a unit')
            over = draw_unit_info(files, unit_values(ram, player), ram.clip())
        elif state == 3 and mode == 5:
            shows.append('the overview, its window at square %d, %d' % (ram.word(rec + 0x0C), ram.word(rec + 0x0E)))
            over = draw_overview(files, ram, player)
        else:
            shows.append('the map from square %d, %d' % ((ram.word(rec + 2) >> 1) % ram.word(MAP_WIDTH),
                                                        (ram.word(rec + 2) >> 1) // ram.word(MAP_WIDTH)))
        if over:
            s.pix = [o if o >= 0 else p for o, p in zip(over, s.pix)]
        line = lines[player]
        if line is None and not over:
            n = cursor_unit(ram, player)
            if n is not None:
                line = ('unit', n)
                u = ram.bytes(UNITS + UNIT_REC * n, UNIT_REC)
                aim = struct.unpack_from('<H', u, 0x11)[0]
                if ram.byte(rec + 0x16) & 2 and aim and same_side(player, u[4]):
                    # T0E9B:125C with the mark 4 or 8 set for the call
                    width, first = ram.word(MAP_WIDTH), ram.word(rec + 2) >> 1
                    col, row = (aim >> 1) % width - first % width, (aim >> 1) // width - first // width
                    if 0 <= col < COLUMNS and 0 <= row < ROWS:
                        shows[-1] += ', the aim of unit %02X marked' % n
                        s.unit24(WINDOW * player + 16 * col, (12 if col & 1 else 0) + 24 * row, files.patt[0],
                                 0x30 if player else 0x20)
        if isinstance(line, tuple):
            u = ram.bytes(UNITS + UNIT_REC * line[1], UNIT_REC)
            unit_line(s, a, player, u, ram.bytes(TYPES + TYPE_REC * u[8], TYPE_REC), ram.bytes(SUFFIXES, 12))
        elif line is not None:
            message_line(s, a, player, line or None)
        if not over or state == 3:
            s.entry(ram.word(rec + 0x10), ram.word(rec + 0x12), files.cursor[ram.byte(rec + 0x1B)], 0)
    return s.pix, shows


def field_main(a, files, ram, vram, game):
    lines = []
    for player, arg in enumerate((a.line0, a.line1)):
        if arg is None or arg == 'clear':
            lines.append(None if arg is None else b'')
        elif arg == 'unit':
            lines.append(('unit', ram.byte(PLAYERS + PLAYER_REC * player + 0x1E)))
        elif arg.startswith('unit:'):
            lines.append(('unit', int(arg[5:], 16)))
        else:
            lines.append(ram.message(int(arg, 16)))
    pix, shows = draw_field(files, ram, lines)
    for player in (0, 1):
        rec = ram.bytes(PLAYERS + PLAYER_REC * player, PLAYER_REC)
        print('player %d: %s; state %d, +18h %02X, cursor %d at %d, %d, +1Ch %04X, unit %02X' % (
            player, shows[player], rec[0x17], rec[0x18], rec[0x1B], *struct.unpack_from('<hh', rec, 0x10),
            struct.unpack_from('<H', rec, 0x1C)[0], rec[0x1E]))
    if vram:
        report(pix, vram, ram)
    if a.png:
        os.makedirs(a.png, exist_ok=True)
        to_png(os.path.join(a.png, 'field.png'), game, pix)
    return 0


def menu_name(codes):
    return ''.join(' ' if c == 1 else chr(ord('A') + c - 17) if 17 <= c <= 42 else str(c - 5) if 5 <= c <= 14 else '?'
                   for c in menu_text(codes))


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


def report(pix, vram, ram=None):
    for page in (0, 1):
        n, same, box = compare(pix, vram, page)
        which = '' if ram is None else ' (drawn to)' if ram.drawn_page() == page else ' (shown)'
        print('  page %d%s: %d pixels drawn, %d as in the video memory%s' % (
            page, which, n, same, '' if box is None else ', the others within x %d..%d, y %d..%d' % (
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


def building_main(a, files, ram, vram, game):
    shown = 0
    for player in (0, 1):
        v = building_values(ram, player)
        if v['state'] != 2:
            continue
        if a.line is not None:
            v['line'] = b'' if a.line == 'clear' else ram.message(int(a.line, 16))
        shown += 1
        print('player %d: slots %s, flags %02X' % (player, ' '.join('%02X' % n for n in v['slots']), v['flags']))
        pix = draw_building(files, v, ram.clip())
        if vram:
            report(pix, vram)
        if a.png:
            os.makedirs(a.png, exist_ok=True)
            to_png(os.path.join(a.png, 'building%d.png' % player), game, pix)
    if not shown:
        print('no player has the state 2')
    return 0


def menu_main(a, files, ram, vram, game):
    if a.message:
        texts = message_values(ram, a.message)
        print('%s: %s' % (a.message, ', '.join(menu_name(t) for t, _, _ in texts)))
        pix = draw_message(files, texts, (a.typed or '') if a.message == 'name' else None)
        if vram:
            report(pix, vram, ram)
        if a.png:
            os.makedirs(a.png, exist_ok=True)
            to_png(os.path.join(a.png, '%s.png' % a.message), game, pix)
        return 0
    items = menu_values(ram, a.menu_id)
    print('menu %d: %s' % (a.menu_id, ', '.join('%s%s' % (menu_name(i['text']), ' (hidden)' if i['flags'] & 0x20 else '')
                                                for i in items)))
    pix = draw_menu(files, items, a.sel)
    if vram:
        report(pix, vram, ram)
    if a.png:
        os.makedirs(a.png, exist_ok=True)
        to_png(os.path.join(a.png, 'menu%d.png' % a.menu_id), game, pix)
    return 0


def scores_main(a, files, ram, vram, game):
    v = scores_values(ram, files, open(a.hi, 'rb').read() if a.hi else None, a.map)
    print('map %d, code %s: %s' % (v['map'], menu_name(v['code']), ', '.join(
        '%s %d' % (menu_name(n), n2) for n, n2 in zip(v['names'], v['scores']))))
    pix = draw_scores(files, v)
    if vram:
        report(pix, vram, ram)
    if a.png:
        os.makedirs(a.png, exist_ok=True)
        to_png(os.path.join(a.png, 'scores.png'), game, pix)
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--status', metavar='RAM', help="draw the status screens from a run's memory")
    ap.add_argument('--unit', metavar='RAM', help="draw the unit's screens from a run's memory")
    ap.add_argument('--building', metavar='RAM', help="draw the building's screens from a run's memory")
    ap.add_argument('--menu', metavar='RAM', help="draw the texts of a menu (--id) from a run's memory")
    ap.add_argument('--scores', metavar='RAM', help="draw the scores' screen (RATING) from a run's memory")
    ap.add_argument('--stats', metavar='RAM', help="draw the statistics after a map from a run's memory")
    ap.add_argument('--field', metavar='RAM', help="draw the map's whole screen from a run's memory")
    ap.add_argument('--line0', metavar='WHAT', help="with --field: player 0's message line: unit (the unit of the "
                    "cursor record's +1Eh), unit:N, a text's number (hex) or clear (default: as the picture has it)")
    ap.add_argument('--line1', metavar='WHAT', help="the same for player 1")
    ap.add_argument('--hi', metavar='FILE', help='the .HI file for --scores (default: the table without a file)')
    ap.add_argument('--id', dest='menu_id', type=int, default=3, help='the menu (default 3, the title menu)')
    ap.add_argument('--sel', type=int, help='the chosen item: draw the picture behind and the cursor too')
    ap.add_argument('--message', choices=sorted(MESSAGES),
                    help="with --menu: one of LOAD's messages or the scores' name instead of a menu")
    ap.add_argument('--line', metavar='N', help="with --building: the message line holds show_message's text N "
                                                "(hex), or nothing (clear)")
    ap.add_argument('--typed', metavar='TEXT', help='with --message name: the letters typed so far')
    ap.add_argument('--map', type=int, metavar='N', help="with --scores: the map whose code is shown (default F27EE:2523)")
    ap.add_argument('--vram', help="compare with the run's video memory")
    ap.add_argument('--png', metavar='DIR', help='write the pictures here')
    ap.add_argument('--game', help="the game's folder (default ISLE)")
    ap.add_argument('--load', default='0077', help='the segment the program was loaded at (hex)')
    a = ap.parse_args()
    game = a.game or os.path.join(game_dir(), 'ISLE')
    files = Files(game)
    if [bool(a.status), bool(a.unit), bool(a.building), bool(a.menu), bool(a.scores), bool(a.stats),
            bool(a.field)].count(True) != 1:
        ap.error('one of --status, --unit, --building, --menu, --scores, --stats and --field')
    ram = Ram(open(a.status or a.unit or a.building or a.menu or a.scores or a.stats or a.field, 'rb').read(),
              int(a.load, 16))
    vram = open(a.vram, 'rb').read() if a.vram else None
    if a.field:
        return field_main(a, files, ram, vram, game)
    if a.stats:
        return stats_main(a, files, ram, vram, game)
    if a.scores:
        return scores_main(a, files, ram, vram, game)
    if a.unit:
        return unit_main(a, files, ram, vram, game)
    if a.menu:
        return menu_main(a, files, ram, vram, game)
    if a.building:
        return building_main(a, files, ram, vram, game)
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
