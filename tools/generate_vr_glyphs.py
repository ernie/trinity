#!/usr/bin/env python3
"""
Generate the VR button glyph atlas: white outline glyphs on alpha in an 8x8 grid of 64-pixel cells, in the order
code/game/vr_glyph.c lists them, and optionally a preview sheet.

Usage:
    python generate_vr_glyphs.py [output_dir] [--preview PNG]

Example:
    python generate_vr_glyphs.py ../assets/gfx/vr --preview ../build/vr_glyphs_preview.png

Requirements:
    pip install Pillow
"""

import argparse
import math
import re
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow")
    exit(1)

CELL = 64
GRID = 8
PAD = 8                     # empty border in each cell, so mipmaps never bleed one glyph into the next
SS = 8                      # supersampling for antialiased edges
UNIT = (CELL - 2 * PAD) * SS / 24   # shapes are drawn on a 24-unit grid inside the padding
STROKE = 1.8
FONT = Path(__file__).resolve().parent / 'fonts' / 'Rajdhani-Bold.ttf'

# Same order as vrgNames in code/game/vr_glyph.c; the index is the atlas cell.
NAMES = [
    'trigger_l', 'trigger_r', 'grip_l', 'grip_r', 'gripclick_l', 'gripclick_r', 'thumbrest_l', 'thumbrest_r',
    'bumper_l', 'bumper_r', 'trackpad_l', 'trackpad_r', 'a_l', 'a_r', 'b_l', 'b_r',
    'stickclick_l', 'stickclick_r', 'stick_up_l', 'stick_down_l', 'stick_left_l', 'stick_right_l',
    'stick_up_r', 'stick_down_r', 'stick_left_r', 'stick_right_r',
    'a', 'b', 'x', 'y', 'menu', 'view', 'dpad_up', 'dpad_down', 'dpad_left', 'dpad_right',
]

TRIGGER = 'M9.5 2.5 H17 Q20.5 2.5 20.5 6 V21.5 H3 Q7.5 17 7.5 10.5 V5 Q7.5 2.5 9.5 2.5 Z'
BUMPER = 'M2.5 18 V13 Q2.5 6.5 12 6.5 Q21.5 6.5 21.5 13 V18 Q21.5 19 20.5 19 H3.5 Q2.5 19 2.5 18 Z'
GRIP = 'M8 2 H15.5 Q19 2 19 5.5 V18.5 Q19 22 15.5 22 H8 Q11 12 8 2 Z'
GRIP_PUSH = 'M1.2 8.4 L7.4 12 L1.2 15.6 Z'
CLICK = 'M9 0.8 H15 L12 4.6 Z'
STEM = 'M6.5 12 V22 H17.5 V12'
STICK_ARROW = 'M8.9 3.6 L12 0.5 L15.1 3.6 Z'
VIEW_BACK = 'M7 13.5 V8 Q7 7 8 7 H13.5'
DPAD_ARM = 'M9.3 1.8 H14.7 V7.6 L12 10.3 L9.3 7.6 Z'
DIR_ANGLE = {'up': 0, 'right': 90, 'down': 180, 'left': 270}


def _cap_ratio():
    font = ImageFont.truetype(str(FONT), 1000)
    return -font.getbbox('H', anchor='ls')[1] / 1000


CAP_RATIO = _cap_ratio()
_fonts = {}


def parse(path):
    """Subpaths of an absolute M/H/V/L/Q/Z path as ([(x, y), ...], closed), curves flattened."""
    tokens = re.findall(r'[MHVLQZ]|-?\d*\.?\d+', path)
    subpaths, points, i, cmd = [], [], 0, None

    def num():
        nonlocal i
        i += 1
        return float(tokens[i - 1])

    while i < len(tokens):
        if tokens[i] in 'MHVLQZ':
            cmd = tokens[i]
            i += 1
        if cmd == 'M':
            if points:
                subpaths.append((points, False))
            points = [(num(), num())]
            cmd = 'L'
        elif cmd == 'H':
            points.append((num(), points[-1][1]))
        elif cmd == 'V':
            points.append((points[-1][0], num()))
        elif cmd == 'L':
            points.append((num(), num()))
        elif cmd == 'Q':
            (x0, y0), cx, cy, x1, y1 = points[-1], num(), num(), num(), num()
            for s in range(1, 17):
                t = s / 16
                points.append(((1 - t) ** 2 * x0 + 2 * (1 - t) * t * cx + t * t * x1,
                               (1 - t) ** 2 * y0 + 2 * (1 - t) * t * cy + t * t * y1))
        elif cmd == 'Z':
            subpaths.append((points, True))
            points, cmd = [], None
    if points:
        subpaths.append((points, False))
    return subpaths


def mirrored(subpaths):
    return [([(24 - x, y) for x, y in pts], closed) for pts, closed in subpaths]


def rotated(subpaths, degrees):
    a = math.radians(degrees)
    c, s = math.cos(a), math.sin(a)
    return [([(12 + (x - 12) * c - (y - 12) * s, 12 + (x - 12) * s + (y - 12) * c) for x, y in pts], closed)
            for pts, closed in subpaths]


def px(p):
    return (PAD * SS + p[0] * UNIT, PAD * SS + p[1] * UNIT)


def stroke(draw, subpaths, width=STROKE):
    w = width * UNIT
    for points, closed in subpaths:
        pts = [px(p) for p in points]
        if closed:
            pts.append(pts[0])
        draw.line(pts, fill=255, width=round(w), joint='curve')
        for x, y in pts:  # Pillow rounds interior joins only; this rounds caps and the closing corner too
            draw.ellipse((x - w / 2, y - w / 2, x + w / 2, y + w / 2), fill=255)


def fill(draw, subpaths):
    for points, _ in subpaths:
        draw.polygon([px(p) for p in points], fill=255)


def circle(draw, cx, cy, r, width=STROKE):
    half = width / 2
    draw.ellipse(px((cx - r - half, cy - r - half)) + px((cx + r + half, cy + r + half)),
                 outline=255, width=round(width * UNIT))


def dot(draw, cx, cy, r):
    draw.ellipse(px((cx - r, cy - r)) + px((cx + r, cy + r)), fill=255)


def side_arc(draw, cx, cy, r, mid, span, width=STROKE):
    """A round-capped arc of span degrees on the circle of radius r, centered on angle mid (0 right, 180 left)."""
    half = width / 2
    draw.arc(px((cx - r - half, cy - r - half)) + px((cx + r + half, cy + r + half)), mid - span / 2, mid + span / 2,
             fill=255, width=round(width * UNIT))
    for a in (mid - span / 2, mid + span / 2):
        dot(draw, cx + r * math.cos(math.radians(a)), cy + r * math.sin(math.radians(a)), half)


def rounded_rect(draw, x0, y0, x1, y1, r, width=STROKE):
    half = width / 2
    draw.rounded_rectangle(px((x0 - half, y0 - half)) + px((x1 + half, y1 + half)), radius=(r + half) * UNIT,
                           outline=255, width=round(width * UNIT))


def filled_rounded_rect(draw, x0, y0, x1, y1, r):
    draw.rounded_rectangle(px((x0, y0)) + px((x1, y1)), radius=r * UNIT, fill=255)


def rounded_outline(x0, y0, x1, y1, r, steps=24):
    """Perimeter of a rounded rectangle as a closed polyline in grid units."""
    pts = []
    for cx, cy, a0 in ((x1 - r, y0 + r, -90), (x1 - r, y1 - r, 0), (x0 + r, y1 - r, 90), (x0 + r, y0 + r, 180)):
        for s in range(steps + 1):
            a = math.radians(a0 + 90 * s / steps)
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    pts.append(pts[0])
    return pts


def dashed(draw, pts, dash, gap, width=STROKE):
    """Strokes the polyline in dash-long pieces with gap-long breaks."""
    on, left, piece = True, dash, [pts[0]]
    for (xa, ya), (xb, yb) in zip(pts, pts[1:]):
        seg, pos = math.hypot(xb - xa, yb - ya), 0.0
        while seg - pos > left:
            pos += left
            p = (xa + (xb - xa) * pos / seg, ya + (yb - ya) * pos / seg)
            if on:
                piece.append(p)
                stroke(draw, [(piece, False)], width)
            piece = [p]
            on, left = not on, gap if on else dash
        left -= seg - pos
        if on:
            piece.append((xb, yb))
    if on and len(piece) > 1:
        stroke(draw, [(piece, False)], width)


def letter(draw, text, cx, cy, cap):
    """text with a cap height of cap units, centered on (cx, cy)."""
    size = round(cap * UNIT / CAP_RATIO)
    font = _fonts.setdefault(size, ImageFont.truetype(str(FONT), size))
    left, top, right, _ = font.getbbox(text, anchor='ls')
    x, y = px((cx, cy))
    draw.text((x - (left + right) / 2, y - top / 2), text, font=font, fill=255, anchor='ls')


def draw_glyph(draw, name):
    hand = name[-1] if name[-2:] in ('_l', '_r') else ''
    base = name[:-2] if hand else name
    side = (lambda sp: mirrored(sp)) if hand == 'r' else (lambda sp: sp)
    mark = hand.upper()
    if base == 'trigger':
        stroke(draw, side(parse(TRIGGER)))
        letter(draw, mark, 14 if hand == 'l' else 10, 12.5, 7)
    elif base == 'bumper':
        stroke(draw, parse(BUMPER))
        letter(draw, mark, 12, 13.2, 6.5)
    elif base in ('grip', 'gripclick'):
        stroke(draw, side(parse(GRIP)))
        if base == 'gripclick':
            fill(draw, side(parse(GRIP_PUSH)))
        letter(draw, mark, 14 if hand == 'l' else 10, 12, 7)
    elif base == 'thumbrest':
        dashed(draw, rounded_outline(2.5, 6, 21.5, 18, 6), 3, 2.2)
        letter(draw, mark, 12, 12, 7)
    elif base == 'trackpad':
        rounded_rect(draw, 5, 1.5, 19, 22.5, 7)  # the Index pad: a tall thumb groove, not a square
        letter(draw, mark, 12, 12, 7)
    elif base in ('a', 'b') and hand:
        cx = 12.8 if hand == 'l' else 11.2  # the pair shifts away from its arc so it stays centered
        circle(draw, cx, 12, 8)
        letter(draw, base.upper(), cx, 12, 7.5)
        side_arc(draw, cx, 12, 10.9, 180 if hand == 'l' else 0, 140)
    elif base == 'stickclick':
        fill(draw, parse(CLICK))
        rounded_rect(draw, 2, 7, 22, 12, 2.5)
        stroke(draw, parse(STEM))
        letter(draw, mark, 12, 17.3, 6)
    elif base.startswith('stick_'):
        circle(draw, 12, 12, 7.6)
        fill(draw, rotated(parse(STICK_ARROW), DIR_ANGLE[base[6:]]))
        letter(draw, mark, 12, 12, 7)
    elif base in ('a', 'b', 'x', 'y'):
        circle(draw, 12, 12, 9.8)
        letter(draw, base.upper(), 12, 12, 8)
    elif base == 'menu':
        circle(draw, 12, 12, 9.8)
        for y in (8.5, 12, 15.5):
            stroke(draw, [([(7.5, y), (16.5, y)], False)], 1.9)
    elif base == 'view':
        circle(draw, 12, 12, 9.8)
        stroke(draw, parse(VIEW_BACK), 1.7)
        filled_rounded_rect(draw, 10, 10, 17, 17, 1)
    elif base.startswith('dpad_'):
        turn = DIR_ANGLE[base[5:]]
        fill(draw, rotated(parse(DPAD_ARM), turn))
        for extra in (90, 180, 270):
            stroke(draw, rotated(parse(DPAD_ARM), turn + extra), 1.5)
    else:
        raise ValueError(f'no drawing for {name}')


def cell_image(name):
    layer = Image.new('L', (CELL * SS, CELL * SS), 0)
    draw_glyph(ImageDraw.Draw(layer), name)
    return layer.resize((CELL, CELL), Image.LANCZOS)


def atlas():
    alpha = Image.new('L', (CELL * GRID, CELL * GRID), 0)
    for i, name in enumerate(NAMES):
        alpha.paste(cell_image(name), ((i % GRID) * CELL, (i // GRID) * CELL))
    image = Image.new('RGBA', alpha.size, (255, 255, 255, 0))
    image.putalpha(alpha)
    return image


def preview(image, path):
    """Each glyph big, then at 20 pixels in the menu's normal, focus and disabled colors, on the menu's dark blue."""
    cols, w, h = 6, 220, 96
    sheet = Image.new('RGB', (cols * w, (len(NAMES) + cols - 1) // cols * h), (29, 31, 43))
    draw = ImageDraw.Draw(sheet)
    for i, name in enumerate(NAMES):
        x, y = (i % cols) * w + 10, (i // cols) * h + 8
        glyph = image.crop(((i % GRID) * CELL, (i // GRID) * CELL, (i % GRID + 1) * CELL, (i // GRID + 1) * CELL))
        sheet.paste(Image.new('RGB', glyph.size, (255, 255, 255)), (x, y), glyph)
        small = glyph.resize((27, 27), Image.LANCZOS)  # a 20-pixel glyph's padded cell
        for j, color in enumerate(((214, 223, 235), (255, 191, 0), (91, 98, 114))):
            sheet.paste(Image.new('RGB', small.size, color), (x + 74 + j * 30, y + 18), small)
        draw.text((x, y + 70), name, fill=(125, 135, 156))
    sheet.save(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('output_dir', nargs='?', default=str(Path(__file__).resolve().parent / '../assets/gfx/vr'))
    parser.add_argument('--preview', help='also write a preview sheet to this PNG')
    args = parser.parse_args()
    out = Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)
    image = atlas()
    image.save(out / 'glyphs.tga', format='TGA')
    print(f'wrote {out / "glyphs.tga"}')
    if args.preview:
        preview(image, args.preview)
        print(f'wrote {args.preview}')


if __name__ == '__main__':
    main()
