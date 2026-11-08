"""
Turns the SVG icons in an HTML sketch into a C++ header of deskkit Icons
(deskkit/icon.h), so the device shows the same icons the sketch does. The
sketch lists them in an object, one per line: `name: '<svg ...>...</svg>',`.
Each SVG is rendered white on transparent at the device size, and its alpha
channel (0-255 per pixel, rows top to bottom) becomes the coverage that
drawIcon blends onto the black screen in the icon's color.

Usage: uv run --with resvg-py --with pillow python lib/deskkit/tools/icons.py <sketch.html> <icons.h>
"""
import io
import re
import sys
from pathlib import Path

import resvg_py
from PIL import Image

SIZE = 26
SKETCH, HEADER = Path(sys.argv[1]), Path(sys.argv[2])

icons = re.findall(r"^\s+(\w+): '(<svg.*?</svg>)',$", SKETCH.read_text(), re.M)
lines = [
    "#pragma once",
    "#include <deskkit/icon.h>",
    "",
    "/*",
    f" * Made by lib/deskkit/tools/icons.py from the SVGs in {SKETCH.name}; rerun it",
    f" * after changing them. Each icon is {SIZE}x{SIZE} pixels.",
    " */",
]
for name, svg in icons:
    svg = svg.replace("currentColor", "#fff").replace('class="icon" ', f'width="{SIZE}" height="{SIZE}" ')
    if "xmlns" not in svg:
        svg = svg.replace("<svg ", '<svg xmlns="http://www.w3.org/2000/svg" ', 1)
    png = bytes(resvg_py.svg_to_bytes(svg_string=svg))
    alpha = Image.open(io.BytesIO(png)).convert("RGBA").getchannel("A")
    assert alpha.size == (SIZE, SIZE), (name, alpha.size)
    data = list(alpha.tobytes())
    rows = [", ".join(f"{v:3d}" for v in data[y * SIZE:(y + 1) * SIZE]) for y in range(SIZE)]
    constant = f"ICON_{name.upper()}"
    lines += ["", f"const uint8_t {constant}_COVERAGE[] = {{"] + [f"  {row}," for row in rows] + ["};"]
    lines += [f"const deskkit::Icon {constant} = {{{SIZE}, {constant}_COVERAGE}};"]
HEADER.write_text("\n".join(lines) + "\n")
print(f"{len(icons)} icons -> {HEADER}")
