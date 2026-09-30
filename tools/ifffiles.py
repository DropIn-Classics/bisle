#!/usr/bin/env python3
"""Battle Isle's pictures (.IFF, .LBM): read them, write them back, show
them.

    ifffiles.py [FILE ...] [--png DIR]

Without FILE every file of the game's folder that is (unpacked) an IFF
FORM is taken.  One line per file: the kind, the size, whether the
chunks and the picture's runs written back give the file's bytes, and
whether packing the pixels anew (see pack_row) gives the same runs.
--png writes each picture as a PNG into DIR, its colours as the DAC
shows them at level 252 (see palfiles.py).

The format, as BATTLE.EXE's reader (segment T2550) takes it:

    "FORM", a length (big endian, as all numbers here), a kind: "PBM "
    (one byte a pixel) or "ILBM" (planes; none in the game's files),
    then chunks: 4 letters, a length, the data, a pad byte when the
    length is odd.  The reader searches for BMHD, CMAP and BODY:
    BMHD +0 width, +2 height, +8 planes, +0Ah compression; CMAP goes to
    DATA:04A0 (the palette set_palette gets: 8-bit values); BODY with
    compression 1 is read row by row, width bytes each: a byte n, 0..127
    takes the n+1 bytes after it as they are, -127..-1 the next byte
    -n+1 times (-128 is not looked into; the reader takes it as 129
    times); a run does not go on over a row's end.  The other chunks
    (DPPS, CRNG, TINY ...) the reader does not look at.

All 16 pictures of the three games are PBM, 8 planes, compression 1;
pack_row (a run of 3 or more same bytes, or 2 at the start of a run,
is repeated, else bytes as they are, at most 128 a run) gives every row
of them as it is stored.  Checked in a run: the map's frame (GAME.IFF)
was in video memory as this tool decodes it, all but colour 64, where
the maps are drawn.
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir  # noqa: E402
import png  # noqa: E402
import palfiles  # noqa: E402
import tpwmfiles  # noqa: E402


def parse_form(data):
    """(kind, [(id, bytes, pad byte or None)]) of an IFF FORM."""
    if data[:4] != b'FORM' or len(data) < 12:
        raise ValueError('not an IFF FORM')
    total = struct.unpack('>I', data[4:8])[0] + 8
    if total != len(data):
        raise ValueError('FORM length %d, file %d' % (total, len(data)))
    chunks = []
    i = 12
    while i < total:
        cid = data[i:i + 4].decode('latin-1')
        n = struct.unpack('>I', data[i + 4:i + 8])[0]
        body = data[i + 8:i + 8 + n]
        if len(body) != n:
            raise ValueError('chunk %s runs past the end' % cid)
        i += 8 + n
        pad = None
        if n & 1:
            pad = data[i]
            i += 1
        chunks.append((cid, body, pad))
    return data[8:12].decode('latin-1'), chunks


def write_form(kind, chunks):
    out = bytearray()
    for cid, body, pad in chunks:
        out += cid.encode('latin-1') + struct.pack('>I', len(body)) + body
        if len(body) & 1:
            out.append(pad or 0)
    return b'FORM' + struct.pack('>I', len(out) + 4) + kind.encode('latin-1') + bytes(out)


def parse_body(body, width, height):
    """the rows' runs: per row a list of ('lit', bytes) / ('rep', count,
    byte); and the bytes after the last row"""
    rows = []
    i = 0
    for _ in range(height):
        runs = []
        left = width
        while left > 0:
            n = body[i]
            i += 1
            if n < 128:
                k = n + 1
                if k > left:
                    raise ValueError('a run over the row end')
                runs.append(('lit', bytes(body[i:i + k])))
                i += k
            else:
                k = 257 - n
                if k > left:
                    raise ValueError('a run over the row end')
                runs.append(('rep', k, body[i]))
                i += 1
            left -= k
        rows.append(runs)
    return rows, bytes(body[i:])


def write_runs(runs):
    out = bytearray()
    for r in runs:
        if r[0] == 'lit':
            out.append(len(r[1]) - 1)
            out += r[1]
        else:
            out += bytes((257 - r[1], r[2]))
    return bytes(out)


def pixels_of(rows):
    out = bytearray()
    for runs in rows:
        for r in runs:
            out += r[1] if r[0] == 'lit' else bytes((r[2],)) * r[1]
    return bytes(out)


def pack_row(row):
    """runs for a row of pixels: a guess at the packer the pictures were
    made with, checked against the files by main()"""
    runs = []
    lit = bytearray()
    i = 0
    n = len(row)
    while i < n:
        k = 1
        while i + k < n and row[i + k] == row[i] and k < 128:
            k += 1
        if k >= 3 or (k == 2 and not lit):
            if lit:
                runs.append(('lit', bytes(lit)))
                lit = bytearray()
            runs.append(('rep', k, row[i]))
            i += k
        else:
            lit += row[i:i + k]
            i += k
            while len(lit) > 128:
                runs.append(('lit', bytes(lit[:128])))
                lit = lit[128:]
            if len(lit) == 128:
                runs.append(('lit', bytes(lit)))
                lit = bytearray()
    if lit:
        runs.append(('lit', bytes(lit)))
    return runs


def read(data):
    """a picture: dict with kind, chunks, width, height, planes,
    compression, rows, rest, pixels, cmap"""
    kind, chunks = parse_form(data)
    get = {cid: body for cid, body, _ in chunks}
    for need in ('BMHD', 'CMAP', 'BODY'):
        if need not in get:
            raise ValueError('no %s' % need)
    bm = get['BMHD']
    width, height = struct.unpack('>HH', bm[:4])
    planes, compression = bm[8], bm[10]
    if kind != 'PBM ' or planes != 8 or compression != 1:
        raise ValueError('%s, %d planes, compression %d: not read' % (kind, planes, compression))
    rows, rest = parse_body(get['BODY'], width, height)
    cmap = get['CMAP']
    return dict(kind=kind, chunks=chunks, width=width, height=height, planes=planes,
                compression=compression, rows=rows, rest=rest, pixels=pixels_of(rows),
                cmap=[tuple(cmap[i:i + 3]) for i in range(0, len(cmap) - 2, 3)])


def write(pic):
    body = b''.join(write_runs(r) for r in pic['rows']) + pic['rest']
    chunks = [(cid, body if cid == 'BODY' else b, pad) for cid, b, pad in pic['chunks']]
    return write_form(pic['kind'], chunks)


def to_png(path, pic):
    cmap = (pic['cmap'] + [(0, 0, 0)] * 256)[:256]
    rgb = [tuple(png.dac_to_rgb(v) for v in c) for c in palfiles.dac(cmap, 8)]
    png.write_indexed(path, pic['width'], pic['height'], pic['pixels'], rgb)


def iff_files(root):
    for d, dirs, files in os.walk(root):
        dirs.sort()
        for f in sorted(files):
            path = os.path.join(d, f)
            with open(path, 'rb') as fh:
                head = fh.read(4)
            if head == b'FORM':
                yield path
            elif head == tpwmfiles.MAGIC:
                if tpwmfiles.unpack(open(path, 'rb').read())[:4] == b'FORM':
                    yield path


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('files', nargs='*')
    ap.add_argument('--png', help='write each picture as a PNG here')
    a = ap.parse_args()
    root = game_dir()
    files = a.files or list(iff_files(root))
    bad = 0
    for path in files:
        rel = os.path.relpath(path, root) if not a.files else path
        data = open(path, 'rb').read()
        packed = data[:4] == tpwmfiles.MAGIC
        if packed:
            data = tpwmfiles.unpack(data)
        try:
            pic = read(data)
        except ValueError as e:
            print('%-22s %s' % (rel, e))
            bad += 1
            continue
        same = write(pic) == data
        w = pic['width']
        repacked = sum(pack_row(pic['pixels'][y * w:(y + 1) * w]) == runs
                       for y, runs in enumerate(pic['rows']))
        bad += not same
        print('%-22s %-6s %s %3dx%3d  %s; rows packed anew alike: %d of %d%s; chunks %s' % (
            rel, 'packed' if packed else 'plain', pic['kind'], w, pic['height'],
            'written back identical' if same else 'WRITTEN BACK OTHERWISE',
            repacked, pic['height'],
            ', %d bytes after the rows' % len(pic['rest']) if pic['rest'] else '',
            ' '.join(c[0] for c in pic['chunks'] if c[0] not in ('BMHD', 'CMAP', 'BODY'))[:40]))
        if a.png:
            dst = os.path.join(a.png, os.path.relpath(path, root) + '.png')
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            to_png(dst, pic)
    print('%d files, %d not read or not written back' % (len(files), bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
