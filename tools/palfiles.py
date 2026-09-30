#!/usr/bin/env python3
"""Battle Isle's palettes (.PAL): read them, write them back, show them.

    palfiles.py [FILE ...] [--png DIR]

Without FILE every .PAL of the game's folder is taken.  A palette file
is packed (TPWM, see tpwmfiles.py) or not; unpacked it is 768 bytes, red,
green and blue of the 256 colours in turn.  One line per file: packed or
not, the values' range, the kind, and whether writing it back (the
packed file from the parsed items, the palette from its colours) gives
the same bytes.  --png writes each palette as a 128x128 PNG into DIR,
the colours in 16 rows of 16, as the DAC shows them (8-bit palettes at
level 252, see below).

Two kinds, told apart here by the values (a guess where no value is
above 63):
  8-bit (0..255): 00..02.PAL, MENU.PAL, MOON's ANIM\\END, HQ, TOT.
    BATTLE.EXE's T251F:000E (level, palette far) writes all 256 DAC
    entries as (value * level) >> 10, the level a byte (read from the
    code).  In a run on map 16 (00.PAL, the CONRA key script of
    HANDOFF.md, 60 s) the DAC's first 24 entries were those of level 252
    and of no other level: the top of a fade in steps of 4, presumably.
    Where the level comes from is not looked into.  Many entries are
    (255, 0, 0), unused ones presumably.
  6-bit (0..63): the other ANIM\\*.PAL, the animations' palettes, taken
    as DAC values (not checked where the game sets them).
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir  # noqa: E402
import png  # noqa: E402
import tpwmfiles  # noqa: E402

SIZE = 768


def read(data):
    """(colours, packed, items) of a palette file's bytes: colours the
    256 (r, g, b) triples as stored."""
    packed = data[:4] == tpwmfiles.MAGIC
    parsed = None
    if packed:
        parsed = tpwmfiles.parse(data)
        data = tpwmfiles.expand(parsed[0], parsed[1])
    if len(data) != SIZE:
        raise ValueError('%d bytes, not %d' % (len(data), SIZE))
    return [tuple(data[i:i + 3]) for i in range(0, SIZE, 3)], packed, parsed


def write(colours, parsed=None):
    """The file's bytes: the colours, packed with the parsed items when
    the file was packed."""
    raw = bytes(c for colour in colours for c in colour)
    if parsed is None:
        return raw
    n, items, spare = parsed
    if tpwmfiles.expand(n, items) != raw:
        raise ValueError('the colours changed; pack them anew')
    return tpwmfiles.write(n, items, spare)


def kind(colours):
    return 8 if max(max(c) for c in colours) > 63 else 6


def dac(colours, bits, level=252):
    """the DAC values at a level (for 8-bit palettes as T251F:000E makes them)"""
    if bits == 6:
        return colours
    return [tuple(v * level >> 10 for v in c) for c in colours]


def swatch(path, colours, bits):
    rgb = [tuple(png.dac_to_rgb(v) for v in c) for c in dac(colours, bits)]
    pixels = bytes((y // 8) * 16 + x // 8 for y in range(128) for x in range(128))
    png.write_indexed(path, 128, 128, pixels, rgb)


def pal_files(root):
    for d, dirs, files in os.walk(root):
        dirs.sort()
        for f in sorted(files):
            if f.upper().endswith('.PAL'):
                yield os.path.join(d, f)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('files', nargs='*')
    ap.add_argument('--png', help='write each palette as a PNG here')
    a = ap.parse_args()
    root = game_dir()
    files = a.files or list(pal_files(root))
    bad = 0
    for path in files:
        rel = os.path.relpath(path, root) if not a.files else path
        data = open(path, 'rb').read()
        try:
            colours, packed, parsed = read(data)
        except ValueError as e:
            print('%-24s %s' % (rel, e))
            bad += 1
            continue
        same = write(colours, parsed) == data
        bad += not same
        bits = kind(colours)
        values = [v for c in colours for v in c]
        print('%-24s %-6s %3d..%3d  %d-bit  %s' % (rel, 'packed' if packed else 'plain',
              min(values), max(values), bits,
              'written back identical' if same else 'WRITTEN BACK OTHERWISE'))
        if a.png:
            dst = os.path.join(a.png, os.path.splitext(os.path.relpath(path, root))[0] + '.png')
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            swatch(dst, colours, bits)
    print('%d files, %d not read or not written back' % (len(files), bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
