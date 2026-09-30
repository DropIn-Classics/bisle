#!/usr/bin/env python3
"""Battle Isle's graphics libraries (LIB\\*.LIB, their .DAT): read them,
write them back, show them.

    libfiles.py [FILE ...] [--png DIR] [--pal FILE] [--base N]

Without FILE every .LIB of the game's folder is taken.  One line per
file: the entries, their kinds, whether the entries and the directory
written back give the (unpacked) file's bytes, how packed entries were
packed, and whether the .DAT beside it names the library's entries.
--png writes each entry the programs can draw as a PNG into DIR
(DIR/<file>/<nnn>_<name>.png), its colours those of --pal (default the
game's 00.PAL, at level 252, see palfiles.py), transparent pixels clear.

The format, as BATTLE.EXE's loader T0CEB:0008 and its drawing routines
take it (the file itself is packed, see tpwmfiles.py):

    a long: the directory's offset; the entries; the directory: records
    of 12 bytes, a name (8 bytes, zero-padded) and the entry's offset (a
    long).  The loader replaces the directory with far pointers to the
    entries, in its order, up to a record of offset 0 (the 12 zero bytes
    it puts after the file).

    An entry (the offsets are those of the directory, in its order, each
    entry ends where the next begins; the last where the directory
    does):
      +0   8 bytes, a label (the name, "INFO"+name, "ILBM    "; not read
           by the loader or the drawing routines as far as seen)
      +8   the transparent value (a pixel of it is not drawn)
      +9   the kind: 'P' or 'U' (T2470:0008 draws 'P' with T23DE:000A,
           anything else with T23FD:000A)
      +0Ah, +0Ch  added to x and y when drawn (words, signed presumably)
      +0Eh the width w, +10h the height h
      +12h the pixels, plane by plane (unchained 256 colours: plane p
           holds the columns p, p+4, ...; the first drawn to the plane of
           x): a plane's h rows one after the other;
           'U' a byte a pixel, w // 4 + (p < w % 4) bytes a row;
           'P' two pixels a byte (the high nibble first), w // 4 bytes a
           row: 8 * (w // 4) pixels wide, the byte k of a row giving the
           columns 8k+p and 8k+4+p.
      Each pixel is drawn as its value plus a colour base the caller
      gives; the transparent value is compared before the base is added.
      A 'U' entry of odd length has one byte more (a pad byte, of
      varying values).
    MOON's BIGUNIT.LIB and FIGHT.LIB have each entry packed on its own
    (TPWM, and a pad byte to an even length); MOON.EXE's T261E:0008
    unpacks an entry that begins with TPWM (T27D5:0008) before drawing.

    NAME.DAT beside NAME.LIB (UNIT, PART, BIGUNIT; UNITB, BIGUNITB): the
    names of the entries (8 bytes each) in the order the game wants
    them.  The loader, when asked (flag 1: unit, part, bigunit), sorts
    the directory by it (T0CEB:0368); a name the directory does not
    have (BIGUNIT.DAT's last, MAA) is left out and the loader's list
    ends before it.  MOON ships no .DAT (what MOON.EXE does then is not
    looked into).

Checked in runs (the video memory at the first map of BATTLE.EXE and of
MOON.EXE): unit sprites of UNIT.LIB as decoded here, with base 20h for
player 1 and 30h for player 2, the terrain of PART.LIB (base 0), a
cursor, the frame of RAND.LIB (base 40h); MOON's units of width 14 are
24 pixels wide as decoded.  BIGUNIT and FIGHT (the fight scenes) were
not seen in video memory.

ISLE's UNITB.LIB, BIGUNITB.LIB, 00.LIB and 01.LIB have entries of
another kind (+8 0Fh, +9 0Fh, then other fields; 1-bit planes by the
looks, not decoded).  No program of the three games names these files.
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir  # noqa: E402
import palfiles  # noqa: E402
import png  # noqa: E402
import tpwmfiles  # noqa: E402

HEAD = 0x12
# the colour bases seen in runs (see the docstring); others drawn at 0
BASES = {'UNIT': 0x20, 'RAND': 0x40}


def pixel_bytes(kind, w, h):
    if kind == 'P':
        return 4 * (w // 4) * h
    if kind == 'U':
        return w * h
    return None


def parse_entry(blob):
    """an entry's fields; 'packed' holds TPWM's (length, items, spare)
    and the pad byte when the entry is packed on its own"""
    e = {'packed': None}
    if blob[:4] == tpwmfiles.MAGIC:
        try:
            parsed, pad = tpwmfiles.parse(blob), None
        except ValueError:
            parsed, pad = tpwmfiles.parse(blob[:-1]), blob[-1]
        e['packed'] = (parsed, pad)
        blob = tpwmfiles.expand(parsed[0], parsed[1])
    e['label'] = blob[:8]
    e['key'], kind = blob[8], chr(blob[9])
    e['kind'] = kind
    e['dx'], e['dy'], e['w'], e['h'] = struct.unpack_from('<hhHH', blob, 10)
    n = pixel_bytes(kind, e['w'], e['h'])
    if n is None or HEAD + n > len(blob):
        e['kind'] = None
        e['raw'] = blob
        return e
    e['pixels'] = blob[HEAD:HEAD + n]
    e['tail'] = blob[HEAD + n:]
    return e


def entry_bytes(e):
    if e['kind'] is None:
        raw = e['raw']
    else:
        raw = (e['label'] + bytes((e['key'], ord(e['kind']))) +
               struct.pack('<hhHH', e['dx'], e['dy'], e['w'], e['h']) + e['pixels'] + e['tail'])
    if e['packed'] is None:
        return raw
    (n, items, spare), pad = e['packed']
    if tpwmfiles.expand(n, items) != raw:
        raise ValueError('the entry changed; pack it anew')
    return tpwmfiles.write(n, items, spare) + (b'' if pad is None else bytes((pad,)))


def read(data):
    """[(name, entry)] of a library's (unpacked) bytes"""
    diro = struct.unpack_from('<I', data, 0)[0]
    if diro > len(data) or (len(data) - diro) % 12:
        raise ValueError('no directory at %X' % diro)
    recs = [(data[k:k + 8], struct.unpack_from('<I', data, k + 8)[0])
            for k in range(diro, len(data), 12)]
    ends = [o for _, o in recs[1:]] + [diro]
    out = []
    for (name, o), end in zip(recs, ends):
        if not 4 <= o < end:
            raise ValueError('entry %s at %X out of order' % (name, o))
        out.append((name, parse_entry(data[o:end])))
    return out


def write(entries):
    body = bytearray()
    dirs = bytearray()
    for name, e in entries:
        dirs += name + struct.pack('<I', 4 + len(body))
        body += entry_bytes(e)
    return struct.pack('<I', 4 + len(body)) + bytes(body) + bytes(dirs)


def image(e):
    """(width, height, values with -1 transparent) of a 'P' or 'U' entry"""
    w, h, key, px = e['w'], e['h'], e['key'], e['pixels']
    out = []
    if e['kind'] == 'P':
        b = w // 4
        width = 8 * b
        out = [-1] * (width * h)
        i = 0
        for p in range(4):
            for y in range(h):
                for k in range(b):
                    c = px[i]
                    i += 1
                    for v, x in ((c >> 4, 8 * k + p), (c & 15, 8 * k + 4 + p)):
                        if v != key:
                            out[y * width + x] = v
        return width, h, out
    out = [-1] * (w * h)
    i = 0
    for p in range(4):
        for y in range(h):
            for x in range(p, w, 4):
                if px[i] != key:
                    out[y * w + x] = px[i]
                i += 1
    return w, h, out


def to_png(path, e, rgb, base):
    width, height, vals = image(e)
    clear = (e['key'] + base) & 0xFF
    pixels = bytes(clear if v < 0 else (v + base) & 0xFF for v in vals)
    png.write_indexed(path, width, height, pixels, rgb, clear)


def dat_check(path, entries):
    """None without a .DAT, else (names in it, those the library lacks)"""
    dat = os.path.splitext(path)[0] + '.DAT'
    if not os.path.exists(dat):
        return None
    data = open(dat, 'rb').read()
    if data[:4] == tpwmfiles.MAGIC:
        data = tpwmfiles.unpack(data)
    names = [data[k:k + 8] for k in range(0, len(data), 8)]
    have = {n for n, _ in entries}
    return names, [n for n in names if n not in have]


def lib_files(root):
    for d, dirs, files in os.walk(root):
        dirs.sort()
        for f in sorted(files):
            if f.upper().endswith('.LIB'):
                yield os.path.join(d, f)


def show(name):
    return name.rstrip(b'\0').decode('latin-1')


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('files', nargs='*')
    ap.add_argument('--png', help='write each entry as a PNG here')
    ap.add_argument('--pal', help='the palette for --png (default ISLE/00.PAL)')
    ap.add_argument('--base', type=lambda s: int(s, 0),
                    help='the colour base for --png (default: as seen in runs, else 0)')
    a = ap.parse_args()
    root = game_dir()
    files = a.files or list(lib_files(root))
    rgb = None
    if a.png:
        colours, _, _ = palfiles.read(open(a.pal or os.path.join(root, 'ISLE', '00.PAL'), 'rb').read())
        rgb = [tuple(png.dac_to_rgb(v) for v in c)
               for c in palfiles.dac(colours, palfiles.kind(colours))]
    bad = 0
    for path in files:
        rel = os.path.relpath(path, root) if not a.files else path
        data = open(path, 'rb').read()
        if data[:4] == tpwmfiles.MAGIC:
            data = tpwmfiles.unpack(data)
        try:
            entries = read(data)
            same = write(entries) == data
        except ValueError as e:
            print('%-22s %s' % (rel, e))
            bad += 1
            continue
        bad += not same
        kinds = {}
        packers = {}
        for _, e in entries:
            k = e['kind'] or 'other'
            kinds[k] = kinds.get(k, 0) + 1
            if e['packed']:
                (n, items, _), _ = e['packed']
                p = tpwmfiles.which_packer(n, items, tpwmfiles.expand(n, items))
                packers[str(p)] = packers.get(str(p), 0) + 1
        dc = dat_check(path, entries)
        print('%-22s %3d entries (%s)%s  %s%s' % (
            rel, len(entries), ' '.join('%s %d' % kv for kv in sorted(kinds.items())),
            ', packed each: ' + ', '.join('%s %d' % kv for kv in sorted(packers.items()))
            if packers else '',
            'written back identical' if same else 'WRITTEN BACK OTHERWISE',
            '' if dc is None else '; .DAT %d names%s' % (
                len(dc[0]), ', not in the library: ' + ' '.join(map(show, dc[1])) if dc[1] else '')))
        if a.png:
            stem = os.path.splitext(os.path.basename(path))[0].upper()
            base = a.base if a.base is not None else BASES.get(stem, 0)
            out = os.path.join(a.png, os.path.relpath(path, root) if not a.files else os.path.basename(path))
            for i, (name, e) in enumerate(entries):
                if e['kind'] is not None:
                    os.makedirs(out, exist_ok=True)
                    safe = ''.join(c if c.isalnum() else '_' for c in show(name))
                    to_png(os.path.join(out, '%03d_%s.png' % (i, safe)), e, rgb, base)
    print('%d files, %d not read or not written back' % (len(files), bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
