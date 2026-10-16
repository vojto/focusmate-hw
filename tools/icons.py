"""
Turns the dashboard icons from the sketch (tools/sketch/dashboard.html) into
src/icons.h, so the device shows the same icons the sketch does. Each SVG is
rendered white on transparent at the device size, and its alpha channel
(0-255 per pixel, rows top to bottom) becomes a byte array the firmware
blends onto the black screen in the icon's color.

Usage: uv run --with resvg-py --with pillow python tools/icons.py
"""
import io
import re
from pathlib import Path

import resvg_py
from PIL import Image

SIZE = 26
ROOT = Path(__file__).resolve().parent.parent
SKETCH = ROOT / "tools/sketch/dashboard.html"
HEADER = ROOT / "src/icons.h"

icons = re.findall(r"^\s+(\w+): '(<svg.*?</svg>)',$", SKETCH.read_text(), re.M)
lines = [
    "#pragma once",
    "#include <stdint.h>",
    "",
    "/*",
    " * The dashboard icons, made by tools/icons.py from the SVGs in",
    " * tools/sketch/dashboard.html; rerun it after changing them. Each icon is",
    f" * {SIZE}x{SIZE} bytes of coverage, 0-255, rows top to bottom.",
    " */",
    "",
    f"#define ICON_SIZE {SIZE}",
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
    lines += ["", f"const uint8_t ICON_{name.upper()}[] = {{"] + [f"  {row}," for row in rows] + ["};"]
HEADER.write_text("\n".join(lines) + "\n")
print(f"{len(icons)} icons -> {HEADER.relative_to(ROOT)}")
