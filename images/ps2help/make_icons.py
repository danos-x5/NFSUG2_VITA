"""Upscales the PS2 version's front-end button icons and writes the DXT3 texture data header and a preview sheet.

Needs Pillow. Run: python3 images/ps2help/make_icons.py
Reads images/ps2help/ps2/ (from decode_ps2icons.py); writes src/ug2/helpicons_art.h and images/ps2help/icons_preview.png.
"""
import os
import struct

from PIL import Image, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
SIZE = 64
SOURCE = 32
LIME = (202, 244, 138)
SHOULDER_INSIDE = (4, 10, 28, 21)  # x0, y0, x1, y1 inside the L1/R1 box, in source pixels


def drop_digit(img):
    """Turns the PS2's L1/R1 into the Vita's L/R: removes the 1 and centres the letter."""
    x0, y0, x1, y1 = SHOULDER_INSIDE
    alpha = img.getchannel("A")
    columns = [x for x in range(x0, x1) if any(alpha.getpixel((x, y)) for y in range(y0, y1))]
    glyph = [columns[0]]
    for x in columns[1:]:
        if x - glyph[-1] > 1:
            break
        glyph.append(x)
    letter = img.crop((glyph[0], y0, glyph[-1] + 1, y1))
    out = img.copy()
    out.paste((0, 0, 0, 0), (x0, y0, x1, y1))
    shift = round((x0 + x1 - 1) / 2 - (glyph[0] + glyph[-1]) / 2)
    out.paste(letter, (glyph[0] + shift, y0))
    return out


def upscale(img):
    """Smooth edges at SIZE: alpha upscaled and re-sharpened; dim parts keep their alpha."""
    alpha = img.getchannel("A")
    big = alpha.resize((SIZE * 4, SIZE * 4), Image.BICUBIC).filter(ImageFilter.GaussianBlur(3))
    big = big.point(lambda v: max(0, min(255, int((v - 128) * 2.2 + 128))))
    sharp = big.resize((SIZE, SIZE), Image.LANCZOS)
    soft = alpha.resize((SIZE, SIZE), Image.BICUBIC)
    a = [min(s, f) if f < 200 else s for s, f in zip(sharp.getdata(), soft.getdata())]
    out = Image.new("RGBA", (SIZE, SIZE), LIME + (0,))
    out.putdata([LIME + (v,) for v in a])
    return out


ICONS = [
    ("CROSS", "PS2_CROSS"),
    ("CIRCLE", "PS2_CIRCLE"),
    ("SQUARE", "PS2_SQUARE"),
    ("TRIANGLE", "PS2_TRIANGLE"),
    ("L", "PS2_L1"),
    ("R", "PS2_R1"),
    ("DPAD_UP", "PS2_DPADU"),
    ("DPAD_DOWN", "PS2_DPADD"),
    ("DPAD_LEFT", "PS2_DPADL"),
    ("DPAD_RIGHT", "PS2_DPADR"),
    ("LSTICK", "PS2_LEFTANALOG"),
    ("RSTICK", "PS2_RIGHTANALOG"),
    ("START", "PS2_START"),
    ("SELECT", "PS2_SELECT"),
]


def render(name, source):
    img = Image.open(os.path.join(HERE, "ps2", source + ".png")).convert("RGBA")
    if name in ("L", "R"):
        img = drop_digit(img)
    return upscale(img)


def rgb565(c):
    return ((c[0] >> 3) << 11) | ((c[1] >> 2) << 5) | (c[2] >> 3)


def unpack565(v):
    r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
    return (r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2)


def dist(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))


def dxt3(img):
    width, height = img.size
    pixels = img.load()
    out = bytearray()
    for by in range(0, height, 4):
        for bx in range(0, width, 4):
            block = [pixels[bx + i % 4, by + i // 4] for i in range(16)]
            for i in range(0, 16, 2):
                a0 = (block[i][3] * 15 + 127) // 255
                a1 = (block[i + 1][3] * 15 + 127) // 255
                out.append(a0 | (a1 << 4))
            best, first, second = -1, 0, 0
            for i in range(16):
                for j in range(i + 1, 16):
                    d = dist(block[i][:3], block[j][:3])
                    if d > best:
                        best, first, second = d, i, j
            c0, c1 = rgb565(block[first][:3]), rgb565(block[second][:3])
            if c0 < c1:
                c0, c1 = c1, c0
            e0, e1 = unpack565(c0), unpack565(c1)
            palette = [e0, e1, tuple((2 * a + b) // 3 for a, b in zip(e0, e1)),
                       tuple((a + 2 * b) // 3 for a, b in zip(e0, e1))]
            indices = 0
            for i in range(16):
                k = min(range(4), key=lambda n: dist(block[i][:3], palette[n]))
                indices |= (0 if c0 == c1 else k) << (2 * i)
            out += struct.pack("<HHI", c0, c1, indices)
    return bytes(out)


ATLAS = 256
COLUMNS = ATLAS // SIZE
REAR = 32
REAR_CELL = len(ICONS)      # the cell after Select holds the four rear icons, two by two
ARM_CELL = REAR_CELL + 1    # the next cell holds the four single-arm d-pad icons, two by two
ARMS = ("UP", "DOWN", "LEFT", "RIGHT")


def arm_of(x, y):
    """Which d-pad arm a source pixel belongs to, split along the diagonals through the centre."""
    dx, dy = x - (SOURCE - 1) / 2, y - (SOURCE - 1) / 2
    if abs(dy) >= abs(dx):
        return "UP" if dy < 0 else "DOWN"
    return "LEFT" if dx < 0 else "RIGHT"


def arm_icon(arm):
    """The PS2 d-pad with only one arm filled: the filled arm from the art that fills it, the other three outlines.

    PS2_DPADU and PS2_DPADD are the same art (vertical arms filled), as are PS2_DPADL and PS2_DPADR."""
    vertical = Image.open(os.path.join(HERE, "ps2", "PS2_DPADU.png")).convert("RGBA")
    horizontal = Image.open(os.path.join(HERE, "ps2", "PS2_DPADL.png")).convert("RGBA")
    out = Image.new("RGBA", (SOURCE, SOURCE), (0, 0, 0, 0))
    for y in range(SOURCE):
        for x in range(SOURCE):
            here = arm_of(x, y)
            is_vertical = here in ("UP", "DOWN")
            art = vertical if is_vertical == (here == arm) else horizontal
            out.putpixel((x, y), art.getpixel((x, y)))
    return upscale(out).resize((REAR, REAR), Image.LANCZOS)


REAR_W, REAR_H, REAR_PITCH = 32, 18, 20     # a 16:9 pad, rows 20 texels apart so the DXT blocks of two rows do not share


def rear_icon(quadrant):
    """The Vita's rear pad, 16:9 with slightly rounded corners, split into four with one quadrant filled
    (0 UL, 1 UR, 2 LL, 3 LR)."""
    k = 8
    w, h = REAR_W * k, REAR_H * k
    m, stroke = 2 * k, 2 * k
    box = (m, m, w - m - 1, h - m - 1)
    mid_x, mid_y = w // 2, h // 2
    img = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle(box, radius=2 * k, outline=255, width=stroke)
    d.line((mid_x, m, mid_x, h - m), fill=255, width=stroke)
    d.line((m, mid_y, w - m, mid_y), fill=255, width=stroke)
    body = Image.new("L", (w, h), 0)
    ImageDraw.Draw(body).rounded_rectangle(box, radius=2 * k, fill=255)
    quad = Image.new("L", (w, h), 0)
    x0, x1 = (0, mid_x) if quadrant % 2 == 0 else (mid_x, w)
    y0, y1 = (0, mid_y) if quadrant < 2 else (mid_y, h)
    ImageDraw.Draw(quad).rectangle((x0, y0, x1 - 1, y1 - 1), fill=255)
    fill = Image.composite(body, Image.new("L", (w, h), 0), quad)
    img = Image.composite(Image.new("L", (w, h), 255), img, fill)
    alpha = img.resize((REAR_W, REAR_H), Image.LANCZOS)
    out = Image.new("RGBA", (REAR_W, REAR_H), LIME + (0,))
    out.putdata([LIME + (v,) for v in alpha.getdata()])
    return out


def main():
    atlas = Image.new("RGBA", (ATLAS, ATLAS), LIME + (0,))
    header = os.path.join(HERE, "..", "..", "src", "ug2", "helpicons_art.h")
    names = []
    for n, (name, source) in enumerate(ICONS):
        atlas.alpha_composite(render(name, source), ((n % COLUMNS) * SIZE, (n // COLUMNS) * SIZE))
        names.append(name)
    base = ((REAR_CELL % COLUMNS) * SIZE, (REAR_CELL // COLUMNS) * SIZE)
    for q in range(4):
        atlas.alpha_composite(rear_icon(q), (base[0] + (q % 2) * REAR_W, base[1] + (q // 2) * REAR_PITCH))
        names.append("REAR_" + ("UL", "UR", "LL", "LR")[q])
    base = ((ARM_CELL % COLUMNS) * SIZE, (ARM_CELL // COLUMNS) * SIZE)
    for q, arm in enumerate(ARMS):
        atlas.alpha_composite(arm_icon(arm), (base[0] + (q % 2) * REAR, base[1] + (q // 2) * REAR))
        names.append("ARM_" + arm)
    data = dxt3(atlas)
    lines = ["// Generated by images/ps2help/make_icons.py: the PS2 front-end button icons, upscaled, a %dx%d DXT3 atlas of %dx%d cells."
             % (ATLAS, ATLAS, SIZE, SIZE), "// Cells in order: " + ", ".join(names) + " (REAR_* are 32x18, two by two with rows 20 texels apart, in the cell after SELECT; ARM_* are 32x32 d-pad icons with one arm filled, two by two in the next cell).",
             "#ifndef UG2_HELPICONS_ART_H_", "#define UG2_HELPICONS_ART_H_", "", "#include <cstdint>", "",
             "namespace ug2 { namespace helpiconsart {", "", "const int Size = %d;" % ATLAS,
             "const int Cell = %d;" % SIZE, "const uint8_t Atlas[%d] =\n{" % len(data)]
    for i in range(0, len(data), 32):
        lines.append("    " + ",".join("0x%02x" % b for b in data[i:i + 32]) + ",")
    lines.append("};\n\n}}\n\n#endif /* !UG2_HELPICONS_ART_H_ */")
    with open(header, "w") as f:
        f.write("\n".join(lines) + "\n")
    bg = Image.new("RGBA", atlas.size, (60, 60, 64, 255))
    bg.alpha_composite(atlas)
    bg.resize((ATLAS * 2, ATLAS * 2), Image.NEAREST).save(os.path.join(HERE, "icons_preview.png"))
    print("wrote", header, names)


if __name__ == "__main__":
    main()
