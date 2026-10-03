#!/usr/bin/env python3
"""
Generate the VR keyboard's art: 9-slice key cap, panel and glow images, and the icon strip.

Usage:
    python generate_vkb_art.py [output_dir]

Example:
    python generate_vkb_art.py ../assets/gfx/vkb

Requirements:
    pip install Pillow
"""

import argparse
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow")
    exit(1)

SIZE = 64        # 9-slice images; the engine slices at a quarter of the size
SS = 4           # supersampling for antialiased edges
CELL = 128       # icon strip cell
CELLS = 8


def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(len(a)))


def rounded(size, radius, row_color, outline=None, alpha=255):
    """A rounded rectangle whose fill varies by row: row_color(t) for t from 0 (top) to 1 (bottom)."""
    big = size * SS
    rgb = Image.new('RGB', (big, big))
    px = rgb.load()
    for y in range(big):
        color = row_color(y / (big - 1))
        for x in range(big):
            px[x, y] = color
    mask = Image.new('L', (big, big), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, big - 1, big - 1), radius=radius * SS, fill=alpha)
    img = rgb.copy()
    img.putalpha(mask)
    if outline:
        ImageDraw.Draw(img).rounded_rectangle((0, 0, big - 1, big - 1), radius=radius * SS, outline=outline, width=SS)
    return img.resize((size, size), Image.LANCZOS)


def cap():
    # near white with a faint top-to-bottom falloff, so the engine's tint is the key's color; no bevel band, it read as a glitch
    return rounded(SIZE, 12, lambda t: lerp((255, 255, 255), (228, 229, 233), t))


def panel():
    return rounded(SIZE, 10, lambda t: lerp((31, 32, 36), (18, 19, 22), t), outline=(58, 59, 66, 255), alpha=245)


def glow():
    # falloff by distance to an inset rounded rectangle, so the glow is as thick around the corners as along the sides;
    # a blurred shape would thin at the corners
    big = SIZE * SS
    inset, radius, reach = 16 * SS, 6 * SS, 14 * SS
    mask = Image.new('L', (big, big), 0)
    px = mask.load()
    half = big / 2 - inset
    for y in range(big):
        for x in range(big):
            dx, dy = max(abs(x + 0.5 - big / 2) - (half - radius), 0), max(abs(y + 0.5 - big / 2) - (half - radius), 0)
            d = (dx * dx + dy * dy) ** 0.5 - radius
            t = min(max(d / reach, 0.0), 1.0)
            px[x, y] = int(round(255 * (1 - t) ** 2))
    white = Image.new('L', (big, big), 255)
    return Image.merge('RGBA', (white, white, white, mask)).resize((SIZE, SIZE), Image.LANCZOS)


def icons():
    big = CELL * SS
    strip = Image.new('RGBA', (big * CELLS, big), (0, 0, 0, 0))
    d = ImageDraw.Draw(strip)
    W = 10 * SS      # stroke width
    white = (255, 255, 255, 255)

    def pt(cell, x, y):
        return (cell * big + x * SS, y * SS)

    def line(cell, points):
        d.line([pt(cell, x, y) for x, y in points], fill=white, width=W, joint='curve')

    def poly(cell, points):
        d.polygon([pt(cell, x, y) for x, y in points], fill=white)

    # Every symbol's ink spans the same height, rows 28 to 100 of the 128 cell, so one draw size matches the font's capitals
    # 0 backspace: a long arrow pointing left
    line(0, [(40, 64), (114, 64)])
    poly(0, [(14, 64), (48, 38), (48, 90)])    # the same head as Enter and Tab
    # 1 enter: down then left, arrow head at the left
    line(1, [(102, 28), (102, 72), (48, 72)])
    poly(1, [(18, 72), (52, 44), (52, 100)])
    # 2 tab: arrow right into a stop bar
    line(2, [(14, 64), (76, 64)])
    poly(2, [(104, 64), (72, 38), (72, 90)])
    line(2, [(112, 28), (112, 100)])
    # 3-6 arrows as chevrons
    poly(3, [(30, 64), (90, 28), (90, 100)])
    poly(4, [(98, 64), (38, 28), (38, 100)])
    poly(5, [(64, 28), (28, 100), (100, 100)])
    poly(6, [(64, 100), (28, 28), (100, 28)])
    return strip.resize((CELL * CELLS, CELL), Image.LANCZOS)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('output_dir', nargs='?', default=str(Path(__file__).resolve().parent / '../assets/gfx/vkb'))
    args = parser.parse_args()
    out = Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)
    for name, image in (('cap', cap()), ('panel', panel()), ('glow', glow()), ('icons', icons())):
        image.save(out / f'{name}.tga', format='TGA')
        print(f'wrote {name}.tga {image.size}')


if __name__ == '__main__':
    main()
