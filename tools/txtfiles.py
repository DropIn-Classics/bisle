#!/usr/bin/env python3
"""Battle Isle's texts and its small font (GAME.TXT, TITEL.TXT,
CHAR6.DAT): read them, write them back, show them.

    txtfiles.py [GAMEDIR ...] [--list] [--png DIR] [--match VRAM]

Without GAMEDIR the three games' folders are taken.  One line per file:
what it holds and whether it, written back, gives the (unpacked) file's
bytes.  --list prints the texts.  --png writes the font as a sheet
(DIR/<game>/CHAR6.png, 16 characters a row, a cell of 8x8) and each text
of GAME.TXT as the game draws it (DIR/<game>/GAME/<nn>.png).  --match
VRAM looks for each text of GAME.TXT in a run's video memory (run.py
-vram) and says where it is and in which colour.

The formats, as BATTLE.EXE's routines take them (all three files are
packed, see tpwmfiles.py):

CHAR6.DAT, the font of 6x6 pixels: 12 bytes a character, 255 of them,
    the character's number the byte of the text.  T0708 loads it and
    keeps a far pointer to it (DATA:02DC).  A character is six rows, a
    word each; draw_chars (T2525:000A: x, y, text far) draws the pixels
    whose bits are set, the leftmost bit 7 of the word's first byte, in
    the colour at DATA:00D6, and leaves the others as they are; it goes 6
    pixels on after each character, and to x's first value and 6 rows
    down at a byte 7Ch or 0Dh; 0 ends the text.  It takes 12 bits of a
    row's word (the word rotated, three bytes of video memory, the planes
    by a table of the four bits reversed), so a character could be wider
    than 6; none of the files' has a bit beyond the sixth pixel.

GAME.TXT, the texts of the game's screens: texts one after the other,
    a text its lines, each ended by 0, then a byte 2 and one byte more
    (0 in the files).  draw_text (T164D:0140: x, y, number, colour) skips
    `number` texts (to a 2, then one byte), sets the colour and draws
    line after line with draw_chars, 6 rows down a line, up to the 2.
    draw_number (T164D:01DB: number, x, y) draws a number in decimal the
    same way.  The numbers the code gives (as read): 0Ah, 0Bh, 0Ch, 0Dh
    in the status screen (T1479:0DC5): ROUND .. ACTUAL, the table's
    heads, ATTACK or MOVE.

TITEL.TXT, the lines that run up the title screen: lines, each ended by
    0, up to one that begins with 2.  A line's bytes are entries of
    LIB\\CHAR24.LIB, the entry's number plus 3 (the entries are named
    24_ and the character's code, so a letter is its code less 30h, a
    digit its code less 2Bh); 1 is a gap of a character's width.
    draw_text24 (T164D:000C: x, y, text far, far, ?) draws a line with
    draw_entry, 24 pixels on a character; next_line (T164D:00A2) goes to
    the line after the next 0, or back to the first when that begins
    with 2.  T1727:0004 (the title) has three lines on the screen at a
    time, each centred (160 less 12 a character), one row up a step from
    y 200 (the second and third start at 234 and 268) and replaced by
    the next line at 98; it draws inside y 132 to 199.  An empty line
    makes a gap.

The three games' CHAR6.DAT and GAME.TXT are the same bytes, TITEL.TXT is
ISLE's in DESERT and another in MOON; MOON's are read here as
BATTLE.EXE reads ISLE's (MOON.EXE has the same draw_chars, draw_text24,
next_line and title by their code; its draw_text is not looked for).

Checked against a run (BATTLE.EXE to the status screen, -vram at 56 s;
--match): the texts 0Ah, 0Bh, 0Dh, 0Fh and 10h were in video memory
pixel for pixel as drawn here, in colour 49h (AMOK.DAT's +0Dh), on both
pages.  The title's lines were seen in a shot (25 s), not compared pixel
for pixel.  The other texts of GAME.TXT and the bytes 7Ch and 0Dh (which
no text of the files has) were not seen in a run.
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
import png  # noqa: E402
import tpwmfiles  # noqa: E402

ROWS = 6                                # a character's rows, a word each
STEP = 6                                # pixels to the next character, rows to the next line
END = 2                                 # ends a text of GAME.TXT, and TITEL.TXT's lines
NEWLINES = (0x7C, 0x0D)                 # draw_chars goes to the next line at these
GAP24 = 1                               # TITEL.TXT: nothing drawn, a character on
FIRST24 = 3                             # TITEL.TXT: the byte of CHAR24.LIB's first entry
PAGE = 0x4000                           # --match: the bytes of a plane a page is looked for in


def unpacked(path):
    data = open(path, 'rb').read()
    return tpwmfiles.unpack(data) if data[:4] == tpwmfiles.MAGIC else data


def read_font(data):
    """the characters, each its six rows (words)"""
    if len(data) % (2 * ROWS):
        raise ValueError('%d bytes, not characters of %d' % (len(data), 2 * ROWS))
    return [struct.unpack_from('<%dH' % ROWS, data, i) for i in range(0, len(data), 2 * ROWS)]


def write_font(font):
    return b''.join(struct.pack('<%dH' % ROWS, *rows) for rows in font)


def row_pixels(word):
    """the columns draw_chars sets for a row's word: 12 bits, the first
    bit 7 of the low byte, then down to bit 0, then bits 15 to 12"""
    bits = ((word & 0xFF) << 4) | (word >> 12)
    return [k for k in range(12) if bits & (0x800 >> k)]


def font_width(font):
    return max([k + 1 for rows in font for w in rows for k in row_pixels(w)] or [0])


def read_texts(data):
    """GAME.TXT: [(lines, the byte after the 2)]"""
    texts = []
    i = 0
    while i < len(data):
        lines = []
        while True:
            if i >= len(data):
                raise ValueError('text %d does not end' % len(texts))
            if data[i] == END:
                break
            j = data.index(0, i) if 0 in data[i:] else -1
            if j < 0:
                raise ValueError('text %d: a line does not end' % len(texts))
            lines.append(data[i:j])
            i = j + 1
        if i + 1 >= len(data):
            raise ValueError('no byte after the last text')
        texts.append((lines, data[i + 1]))
        i += 2
    return texts


def write_texts(texts):
    return b''.join(b''.join(line + b'\0' for line in lines) + bytes((END, after))
                    for lines, after in texts)


def read_title(data):
    """TITEL.TXT: (lines, the bytes from the 2 on)"""
    lines = []
    i = 0
    while i < len(data) and data[i] != END:
        j = data.find(0, i)
        if j < 0:
            raise ValueError('a line does not end')
        lines.append(data[i:j])
        i = j + 1
    if i >= len(data):
        raise ValueError('no end')
    return lines, data[i:]


def write_title(title):
    lines, tail = title
    return b''.join(line + b'\0' for line in lines) + tail


def chars24(d):
    """byte -> character for TITEL.TXT, from CHAR24.LIB's entry names
    beside the folder's LIB (None when it is not there)"""
    try:
        entries = libfiles.read(unpacked(find(os.path.join(d, 'LIB'), 'CHAR24.LIB')))
    except (OSError, ValueError):
        return None
    out = {GAP24: ' '}
    for n, (name, _) in enumerate(entries):
        s = libfiles.show(name)
        if s.startswith('24_') and s[3:].isdigit():
            out[n + FIRST24] = chr(int(s[3:]))
    return out


def title_text(line, table):
    return ''.join(table.get(c, '<%02X>' % c) for c in line)


def line_text(line):
    return ''.join(chr(c) if 32 <= c < 127 else '<%02X>' % c for c in line)


def text_pixels(font, lines):
    """the pixels draw_text sets for a text's lines, as (x, y) from the
    text's place, and the cells they are in as (x, y, w, h)"""
    points = []
    cells = []
    y0 = 0
    for line in lines:
        x, y = 0, y0
        for c in line:
            if c in NEWLINES:
                x, y = 0, y + STEP
                continue
            if c < len(font):
                for r, word in enumerate(font[c]):
                    points += [(x + k, y + r) for k in row_pixels(word)]
            cells.append((x, y, STEP, ROWS))
            x += STEP
        y0 += STEP
    return points, cells


def render(font, lines):
    """(width, height, pixels 0 or 1) of a text"""
    points, cells = text_pixels(font, lines)
    w = max([x + 1 for x, _ in points] + [x + cw for x, _, cw, _ in cells] + [1])
    h = max([y + 1 for _, y in points] + [STEP * len(lines), 1])
    pix = bytearray(w * h)
    for x, y in points:
        pix[y * w + x] = 1
    return w, h, bytes(pix)


def sheet(font):
    """the font, 16 characters a row in cells of 8x8"""
    rows = (len(font) + 15) // 16
    w, h = 128, 8 * rows
    pix = bytearray(w * h)
    for n, glyph in enumerate(font):
        for r, word in enumerate(glyph):
            for k in row_pixels(word):
                if k < 8:
                    pix[(8 * (n // 16) + 1 + r) * w + 8 * (n % 16) + 1 + k] = 1
    return w, h, bytes(pix)


PALETTE = [(0, 0, 0), (255, 255, 255)] + [(0, 0, 0)] * 254


def match(font, texts, vram):
    """where each text of GAME.TXT is in a run's video memory: [(number,
    page, x, y, colour)] for the places where all the text's pixels have
    one colour and no other pixel of its characters' cells has it; a
    place inside a longer text's is left out.  A page is PAGE bytes of
    each plane, 200 rows of 320 pixels from its start."""
    shapes = []
    for n, (lines, _) in enumerate(texts):
        points, cells = text_pixels(font, lines)
        if len(points) < 12:
            continue
        on = set(points)
        off = [(x, y) for cx, cy, cw, ch in cells for x in range(cx, cx + cw) for y in range(cy, cy + ch)
               if (x, y) not in on]
        shapes.append((n, points, off, max(x for x, _ in points) + 1, max(y for _, y in points) + 1))
    found = []
    for page in range(len(vram) // 4 // PAGE):
        base = page * PAGE
        screen = [vram[4 * (base + y * 80 + x // 4) + (x & 3)] for y in range(200) for x in range(320)]
        hits = []
        for n, points, off, w, h in shapes:
            x0, y0 = points[0]
            for oy in range(200 - h + 1):
                for ox in range(320 - w + 1):
                    c = screen[(oy + y0) * 320 + ox + x0]
                    if all(screen[(oy + y) * 320 + ox + x] == c for x, y in points) and \
                            all(screen[(oy + y) * 320 + ox + x] != c
                                for x, y in off if ox + x < 320 and oy + y < 200):
                        hits.append((n, ox, oy, c, {(ox + x, oy + y) for x, y in points}))
        for n, ox, oy, c, own in hits:
            if not any(own < other for _, _, _, _, other in hits):
                found.append((n, page, ox, oy, c))
    return found


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
    ap.add_argument('--list', action='store_true', help='print the texts')
    ap.add_argument('--png', help='write the font and the texts of GAME.TXT as PNGs here')
    ap.add_argument('--match', metavar='VRAM', help="look for GAME.TXT's texts in a run's video memory")
    a = ap.parse_args()
    root = game_dir()
    total = bad = 0
    for d in a.dirs or list(game_dirs(root)):
        rel = d if a.dirs else os.path.relpath(d, root)
        game = os.path.basename(os.path.normpath(d))
        font = None
        for name, read, write in (('CHAR6.DAT', read_font, write_font),
                                  ('GAME.TXT', read_texts, write_texts),
                                  ('TITEL.TXT', read_title, write_title)):
            total += 1
            try:
                data = unpacked(find(d, name))
                got = read(data)
            except (OSError, ValueError) as e:
                print('%s\\%s %s' % (rel, name, e))
                bad += 1
                continue
            same = write(got) == data
            bad += not same
            if name == 'CHAR6.DAT':
                font = got
                used = sum(1 for rows in font if any(rows))
                line = '%d characters, %d not empty, up to %d pixels wide' % (len(font), used, font_width(font))
                if a.png:
                    os.makedirs(os.path.join(a.png, game), exist_ok=True)
                    w, h, pix = sheet(font)
                    png.write_indexed(os.path.join(a.png, game, 'CHAR6.png'), w, h, pix, PALETTE)
            elif name == 'GAME.TXT':
                line = '%d texts, %d lines' % (len(got), sum(len(lines) for lines, _ in got))
                odd = [n for n, (_, after) in enumerate(got) if after]
                if odd:
                    line += ', not 0 after the 2 in %s' % ' '.join('%02X' % n for n in odd)
                if a.list:
                    for n, (lines, _) in enumerate(got):
                        print('%02X %s' % (n, ' / '.join("'%s'" % line_text(s) for s in lines)))
                if a.png and font:
                    out = os.path.join(a.png, game, 'GAME')
                    os.makedirs(out, exist_ok=True)
                    for n, (lines, _) in enumerate(got):
                        w, h, pix = render(font, lines)
                        png.write_indexed(os.path.join(out, '%02X.png' % n), w, h, pix, PALETTE)
                if a.match and font:
                    hits = match(font, got, open(a.match, 'rb').read())
                    for n, page, x, y, c in hits:
                        print('text %02X on page %d at x %d, y %d in colour %02X: %s' % (
                            n, page, x, y, c, ' / '.join("'%s'" % line_text(s) for s in got[n][0])))
                    line += ', %d of them in the video memory' % len({hit[0] for hit in hits})
            else:
                lines, tail = got
                table = chars24(d)
                line = '%d lines, %d empty, the longest %d characters' % (
                    len(lines), sum(1 for s in lines if not s), max(map(len, lines)))
                if table is None:
                    line += ', no CHAR24.LIB to read them with'
                else:
                    missing = sorted({c for s in lines for c in s if c not in table})
                    line += ', all characters in CHAR24.LIB' if not missing else \
                        ', NOT in CHAR24.LIB: %s' % ' '.join('%02X' % c for c in missing)
                    if a.list:
                        for s in lines:
                            print("'%s'" % title_text(s, table))
                if tail != bytes((END, 0)):
                    line += ', ends %s' % tail.hex()
            print('%s\\%s %s; %s' % (rel, name, line,
                                     'written back identical' if same else 'WRITTEN BACK OTHERWISE'))
    print('%d files, %d not read or not written back' % (total, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
