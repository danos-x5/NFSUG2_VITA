#!/usr/bin/env python3
"""Makes the Vita icon and LiveArea images from the logo in your own game files.

Writes sce_sys/game/, which git ignores and the build prefers over the plain default images.
"""

import argparse
import math
import os
import struct
import sys
import zlib

HEADERS = 0x33310004
FORMATS = 0x33310005
PIXELS = 0x33320002
HEADER_SIZE = 124
FORMAT_SIZE = 32
LOGO_FILE = os.path.join('GLOBAL', 'GLOBALA.BUN')
LOGO_NAME = 'NFSU_LOGO_CHROME'
GLOW_INNER = (0x1C, 0x2F, 0x47)
GLOW_OUTER = (0x03, 0x05, 0x0A)


def chunks(data, start, end, found):
    """Collects (id, size, offset) of every chunk, nested ones included."""
    while start + 8 <= end:
        ident, size = struct.unpack_from('<II', data, start)
        if start + 8 + size > end:
            break
        found.append((ident, size, start + 8))
        if ident & 0x80000000:
            chunks(data, start + 8, start + 8 + size, found)
        start += 8 + size
    return found


def decode_dxt3(block, width, height):
    """DXT3 blocks to a flat RGBA list."""
    def colour(c):
        return ((c >> 11) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31)

    pixels = [(0, 0, 0, 0)] * (width * height)
    position = 0
    for top in range(0, height, 4):
        for left in range(0, width, 4):
            alpha, c0, c1, bits = struct.unpack_from('<QHHI', block, position)
            position += 16
            a, b = colour(c0), colour(c1)
            palette = (a, b, tuple((2 * x + y) // 3 for x, y in zip(a, b)),
                       tuple((x + 2 * y) // 3 for x, y in zip(a, b)))
            for i in range(16):
                x, y = left + i % 4, top + i // 4
                if x < width and y < height:
                    pixels[y * width + x] = palette[(bits >> (2 * i)) & 3] + (((alpha >> (4 * i)) & 15) * 17,)
    return pixels


def load_texture(path, name):
    """Returns (width, height, RGBA pixels) of a named DXT3 texture."""
    with open(path, 'rb') as f:
        data = f.read()
    found = chunks(data, 0, len(data), [])
    packs = zip([c for c in found if c[0] == HEADERS], [c for c in found if c[0] == FORMATS],
                [c for c in found if c[0] == PIXELS])
    for headers, formats, pixels in packs:
        base = pixels[2]
        while base % 0x80 and data[base] == 0x11:
            base += 1
        for i in range(headers[1] // HEADER_SIZE):
            entry = data[headers[2] + i * HEADER_SIZE:headers[2] + (i + 1) * HEADER_SIZE]
            if entry[12:36].split(b'\0')[0].decode('latin1') != name:
                continue
            fields = struct.unpack_from('<22I', entry, 36)
            width, height = fields[8] & 0xFFFF, fields[8] >> 16
            fourcc = data[formats[2] + i * FORMAT_SIZE + 20:formats[2] + i * FORMAT_SIZE + 24]
            if fourcc != b'DXT3':
                raise ValueError('%s is stored as %r, expected DXT3' % (name, fourcc))
            return width, height, decode_dxt3(data[base + fields[3]:base + fields[3] + width * height], width, height)
    raise ValueError('%s not found in %s' % (name, path))


def trim(width, height, pixels):
    """Crops away fully transparent borders."""
    rows = [y for y in range(height) if any(pixels[y * width + x][3] for x in range(width))]
    columns = [x for x in range(width) if any(pixels[y * width + x][3] for y in range(height))]
    left, right, top, bottom = columns[0], columns[-1] + 1, rows[0], rows[-1] + 1
    return right - left, bottom - top, [pixels[y * width + x] for y in range(top, bottom) for x in range(left, right)]


def weights(source, target):
    """Per output position: (first source index, normalised triangle weights)."""
    scale = source / target
    support = max(1.0, scale)
    table = []
    for i in range(target):
        centre = (i + 0.5) * scale
        first = max(0, int(math.floor(centre - support)))
        last = min(source, int(math.ceil(centre + support)))
        w = [max(0.0, 1.0 - abs((j + 0.5 - centre) / support)) for j in range(first, last)]
        total = sum(w) or 1.0
        table.append((first, [x / total for x in w]))
    return table


def resize(width, height, pixels, new_width, new_height):
    """Smooth resize on premultiplied alpha."""
    pre = [(r * a / 255.0, g * a / 255.0, b * a / 255.0, float(a)) for r, g, b, a in pixels]
    across = weights(width, new_width)
    rows = []
    for y in range(height):
        row = pre[y * width:(y + 1) * width]
        rows.append([tuple(sum(row[first + k][c] * w for k, w in enumerate(ws)) for c in range(4))
                     for first, ws in across])
    out = []
    for first, ws in weights(height, new_height):
        for x in range(new_width):
            r, g, b, a = (sum(rows[first + k][x][c] * w for k, w in enumerate(ws)) for c in range(4))
            out.append((r * 255.0 / a, g * 255.0 / a, b * 255.0 / a, a) if a > 0 else (0.0, 0.0, 0.0, 0.0))
    return out


def fit(width, height, max_width, max_height):
    """Largest size inside the box that keeps the proportions."""
    scale = min(max_width / width, max_height / height)
    return max(1, round(width * scale)), max(1, round(height * scale))


def centred(canvas_width, canvas_height, backdrop, width, height, pixels):
    """Draws the image centred over a single RGBA backdrop colour."""
    out = [tuple(float(c) for c in backdrop)] * (canvas_width * canvas_height)
    left, top = (canvas_width - width) // 2, (canvas_height - height) // 2
    br, bg, bb, ba = backdrop
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[y * width + x]
            t = a / 255.0
            alpha = a + ba * (1.0 - t)
            if alpha <= 0:
                continue
            under = ba * (1.0 - t)
            out[(top + y) * canvas_width + left + x] = ((r * a + br * under) / alpha, (g * a + bg * under) / alpha,
                                                        (b * a + bb * under) / alpha, alpha)
    return out


def quantise(pixels, limit=256):
    """Median cut to a palette; returns (palette, indices)."""
    counts = {}
    for p in pixels:
        key = tuple(min(255, max(0, int(round(c)))) for c in p)
        counts[key] = counts.get(key, 0) + 1
    boxes = [list(counts.items())]
    while len(boxes) < limit:
        best, channel, spread = None, 0, 0
        for box in boxes:
            if len(box) < 2:
                continue
            for c in range(4):
                values = [colour[c] for colour, _ in box]
                size = (max(values) - min(values)) * math.sqrt(sum(n for _, n in box))
                if size > spread:
                    best, channel, spread = box, c, size
        if best is None:
            break
        best.sort(key=lambda item: item[0][channel])
        half, running, cut = sum(n for _, n in best) / 2.0, 0, 1
        for i, (_, n) in enumerate(best[:-1]):
            running += n
            cut = i + 1
            if running >= half:
                break
        boxes.remove(best)
        boxes += [best[:cut], best[cut:]]
    palette, lookup = [], {}
    for box in boxes:
        total = sum(n for _, n in box)
        lookup.update((colour, len(palette)) for colour, _ in box)
        palette.append(tuple(int(round(sum(colour[c] * n for colour, n in box) / total)) for c in range(4)))
    return palette, [lookup[tuple(min(255, max(0, int(round(c)))) for c in p)] for p in pixels]


def glow(width, height):
    """The plain background: a dark blue glow, as (palette, indices)."""
    palette = [tuple(round(a + (b - a) * i / 255.0) for a, b in zip(GLOW_INNER, GLOW_OUTER)) + (255,)
               for i in range(256)]
    radius = max(width, height) / 2.0
    indices = [min(255, int(math.hypot(x + 0.5 - width / 2.0, y + 0.5 - height / 2.0) / radius * 255.0 + 0.5))
               for y in range(height) for x in range(width)]
    return palette, indices


def write_png(path, width, height, palette, indices):
    """Writes an 8-bit palette PNG, as the Vita requires."""
    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))

    rows = b''.join(b'\0' + bytes(indices[y * width:(y + 1) * width]) for y in range(height))
    parts = [b'\x89PNG\r\n\x1a\n', chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0)),
             chunk(b'PLTE', b''.join(bytes(c[:3]) for c in palette))]
    if any(c[3] < 255 for c in palette):
        parts.append(chunk(b'tRNS', bytes(c[3] for c in palette)))
    parts += [chunk(b'IDAT', zlib.compress(rows, 9)), chunk(b'IEND', b'')]
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(b''.join(parts))


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('game', help='the game folder, the one with SPEED2.EXE')
    parser.add_argument('--out', default=os.path.join(here, '..', '..', 'sce_sys', 'game'),
                        help='output folder (default: sce_sys/game)')
    args = parser.parse_args()
    source = os.path.join(args.game, LOGO_FILE)
    if not os.path.isfile(source):
        sys.exit('%s not found: give the game folder, the one with SPEED2.EXE.' % source)
    try:
        logo = trim(*load_texture(source, LOGO_NAME))
    except (ValueError, struct.error, IndexError) as error:
        sys.exit('Could not read the logo (%s). The build will use the plain default images.' % error)

    out = os.path.normpath(args.out)
    contents = os.path.join(out, 'livearea', 'contents')
    size = fit(logo[0], logo[1], 118, 118)
    icon = centred(128, 128, (255, 255, 255, 255), size[0], size[1], resize(*logo, *size))
    write_png(os.path.join(out, 'icon0.png'), 128, 128, *quantise(icon))
    size = fit(logo[0], logo[1], 250, 134)
    startup = centred(280, 158, (0, 0, 0, 77), size[0], size[1], resize(*logo, *size))
    write_png(os.path.join(contents, 'startup.png'), 280, 158, *quantise(startup))
    write_png(os.path.join(contents, 'bg.png'), 840, 500, *glow(840, 500))
    print('Wrote icon0.png, livearea/contents/startup.png and bg.png to ' + out)


if __name__ == '__main__':
    main()
