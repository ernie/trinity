#!/usr/bin/env python3
"""
Prerender a TrueType font into the engine's fonts/fontImage_<size>.dat glyph table and page images.

The table carries the metrics tr_font.c's FreeType path would write at the given point size, so the
engine's existing loader draws it. Pages are rendered at --page pixels (default 1024) instead of the
loader's nominal 256, with the same glyph placement scaled up, since the table's page coordinates
are normalized.

Usage:
    python generate_font.py <font.ttf> <pointsize> [output_dir] [--page 1024]

Example:
    python generate_font.py fonts/Rajdhani-Bold.ttf 24 ../assets/fonts

Requirements:
    pip install Pillow
"""

import argparse
import math
import struct
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow")
    exit(1)

NOMINAL = 256                                   # the loader's page size; placement runs at this scale
GLYPHS = 256
FIRST, LAST = 32, 126                           # printable ASCII
GLYPH_RECORD = struct.Struct('<7i4fi32s')       # height top bottom pitch xSkip imageWidth imageHeight s t s2 t2 glyph shaderName
TRAILER = struct.Struct('<f64s')                # glyphScale, name


def measure(font, scale):
    """Per-glyph metrics at the nominal point size, rounded outward the way the engine's FreeType path rounds."""
    metrics = {}
    max_height = 0
    for code in range(FIRST, LAST + 1):
        ch = chr(code)
        advance = int(font.getlength(ch) / scale) + 1
        left, top, right, bottom = font.getbbox(ch, anchor='ls')   # baseline origin, y down
        if right <= left or bottom <= top:
            metrics[code] = dict(empty=True, xSkip=advance)
            continue
        n_left, n_right = math.floor(left / scale), math.ceil(right / scale)
        n_top, n_bottom = math.ceil(-top / scale), math.floor(-bottom / scale)
        # bottom is stored in pixels; the engine's own writer stores FreeType's unshifted 26.6 value, and no painter reads it
        width, height = n_right - n_left, n_top - n_bottom
        metrics[code] = dict(empty=False, left=n_left, top=int(-top / scale) + 1, box_top=n_top, bottom=n_bottom,
                             height=height, pitch=(width + 3) & -4, xSkip=advance)
        max_height = max(max_height, height)
    return metrics, max_height


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('font')
    parser.add_argument('pointsize', type=int)
    parser.add_argument('output_dir', nargs='?', default=str(Path(__file__).resolve().parent / '../assets/fonts'))
    parser.add_argument('--page', type=int, default=1024, help='page image size in pixels, a multiple of 256')
    args = parser.parse_args()

    scale = args.page // NOMINAL
    if scale * NOMINAL != args.page:
        parser.error('--page must be a multiple of 256')
    out = Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)

    font = ImageFont.truetype(args.font, args.pointsize * scale)
    metrics, max_height = measure(font, scale)

    pages = []
    page = Image.new('L', (args.page, args.page), 0)
    draw = ImageDraw.Draw(page)
    x_out = y_out = 0
    records = [None] * GLYPHS

    def page_name(index):
        return f'fonts/fontImage_{index}_{args.pointsize}'

    code = FIRST
    while code <= LAST:
        g = metrics[code]
        if g['empty']:
            records[code] = (0, 0, 0, 0, g['xSkip'], 0, 0, 0.0, 0.0, 0.0, 0.0, 0, page_name(len(pages)).encode())
            code += 1
            continue
        if x_out + g['pitch'] + 1 >= NOMINAL - 1:
            x_out = 0
            y_out += max_height + 1
        if y_out + max_height + 1 >= NOMINAL - 1:
            pages.append(page)
            page = Image.new('L', (args.page, args.page), 0)
            draw = ImageDraw.Draw(page)
            x_out = y_out = 0
            continue                                # place this glyph on the fresh page
        # The cell's top-left is the glyph box corner; the baseline origin sits box_top below it and left to its right.
        draw.text(((x_out - g['left']) * scale, (y_out + g['box_top']) * scale), chr(code), font=font, fill=255, anchor='ls')
        s, t = x_out / NOMINAL, y_out / NOMINAL
        records[code] = (g['height'], g['top'], g['bottom'], g['pitch'], g['xSkip'], g['pitch'], g['height'],
                         s, t, s + g['pitch'] / NOMINAL, t + g['height'] / NOMINAL, 0, page_name(len(pages)).encode())
        x_out += g['pitch'] + 1
        code += 1
    pages.append(page)

    for index, image in enumerate(pages):
        white = Image.new('L', image.size, 255)
        rgba = Image.merge('RGBA', (white, white, white, image))
        rgba.save(out / f'fontImage_{index}_{args.pointsize}.tga', format='TGA')

    blank = (0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0, page_name(0).encode())
    data = b''.join(GLYPH_RECORD.pack(*(records[i] or blank)) for i in range(GLYPHS))
    data += TRAILER.pack(48.0 / args.pointsize, f'fonts/fontImage_{args.pointsize}'.encode())
    assert len(data) == 20548, len(data)
    (out / f'fontImage_{args.pointsize}.dat').write_bytes(data)
    print(f'wrote {len(pages)} page(s) and fontImage_{args.pointsize}.dat to {out}')


if __name__ == '__main__':
    main()
