#!/usr/bin/env python3
"""
Generate number sprite textures from a TTF font for Quake 3 damage plums.

Usage:
    python generate_number_sprites.py <font.ttf> [output_dir] [--size SIZE]

Example:
    python generate_number_sprites.py "d:/Development/Projects/quakelive-unpacked/fonts/handelgothic.ttf"
    python generate_number_sprites.py "d:/Development/Projects/quakelive-unpacked/fonts/handelgothic.ttf" "../assets/gfx/2d/damage/"

Requirements:
    pip install Pillow
"""

import argparse
import os
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow")
    exit(1)


# Mapping of digit index to filename (matching Quake 3 convention)
DIGIT_NAMES = {
    '0': 'zero_32b',
    '1': 'one_32b',
    '2': 'two_32b',
    '3': 'three_32b',
    '4': 'four_32b',
    '5': 'five_32b',
    '6': 'six_32b',
    '7': 'seven_32b',
    '8': 'eight_32b',
    '9': 'nine_32b',
    '-': 'minus_32b',
}

# Characters to measure for consistent dimensions
ALL_CHARS = '0123456789-'


def measure_max_dimension(font, chars, padding=4):
    """Measure the maximum dimension (width or height) needed for all characters.

    Returns a square size that fits all characters, ensuring consistent rendering.
    """
    temp_img = Image.new('RGBA', (500, 500), (0, 0, 0, 0))
    temp_draw = ImageDraw.Draw(temp_img)

    max_dim = 0
    for char in chars:
        bbox = temp_draw.textbbox((0, 0), char, font=font)
        char_width = bbox[2] - bbox[0]
        char_height = bbox[3] - bbox[1]
        max_dim = max(max_dim, char_width, char_height)

    # Add padding and round to multiple of 4
    total_dim = max_dim + padding * 2
    return ((total_dim + 3) // 4) * 4


def generate_digit_sprite(char, font, cell_size):
    """Generate a single digit sprite with transparency in a square cell.

    All sprites will be cell_size x cell_size, with the glyph centered.
    This ensures consistent rendering when the engine scales sprites uniformly.
    """
    # Create a temporary image to measure the text
    temp_img = Image.new('RGBA', (500, 500), (0, 0, 0, 0))
    temp_draw = ImageDraw.Draw(temp_img)

    # Get text bounding box
    bbox = temp_draw.textbbox((0, 0), char, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]

    # Create square image
    img = Image.new('RGBA', (cell_size, cell_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Center the text horizontally and vertically
    x = (cell_size - text_width) // 2 - bbox[0]
    y = (cell_size - text_height) // 2 - bbox[1]

    # Draw the character in white (shader will colorize it)
    draw.text((x, y), char, font=font, fill=(255, 255, 255, 255))

    return img


def main():
    # Get script directory for default output path
    script_dir = Path(__file__).parent.resolve()
    default_output = script_dir.parent / "assets" / "gfx" / "2d" / "damage"

    parser = argparse.ArgumentParser(
        description='Generate number sprite textures from a TTF font'
    )
    parser.add_argument('font', help='Path to the TTF font file')
    parser.add_argument('output_dir', nargs='?', default=str(default_output),
                        help=f'Output directory for sprites (default: {default_output})')
    parser.add_argument('--size', type=int, default=128,
                        help='Font size in pixels (default: 128)')
    parser.add_argument('--format', choices=['tga', 'png'], default='tga',
                        help='Output format (default: tga)')

    args = parser.parse_args()

    # Validate font file
    font_path = Path(args.font)
    if not font_path.exists():
        print(f"Error: Font file not found: {font_path}")
        return 1

    # Create output directory
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Load the font
    try:
        font = ImageFont.truetype(str(font_path), args.size)
    except Exception as e:
        print(f"Error loading font: {e}")
        return 1

    print(f"Generating sprites from: {font_path}")
    print(f"Font size: {args.size}px")
    print(f"Output directory: {output_dir}")
    print(f"Output format: {args.format}")

    # Calculate fixed square cell size based on largest dimension
    cell_size = measure_max_dimension(font, ALL_CHARS)
    print(f"Cell size: {cell_size}x{cell_size}px")
    print()

    # Generate each digit
    for char, name in DIGIT_NAMES.items():
        img = generate_digit_sprite(char, font, cell_size)

        output_file = output_dir / f"{name}.{args.format}"

        if args.format == 'tga':
            # Save as TGA (32-bit with alpha)
            img.save(str(output_file), format='TGA')
        else:
            # Save as PNG
            img.save(str(output_file), format='PNG')

        print(f"  Generated: {name}.{args.format} ({img.width}x{img.height})")

    print()
    print("Done! Sprites generated successfully.")

    return 0


if __name__ == '__main__':
    exit(main())
