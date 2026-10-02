"""
Drives the debug firmware (src/debug.h) over serial: sends one-letter commands
and saves the device's screen as a PNG, so the drawing can be checked from the
computer. Needs the `debug` environment flashed.

Usage: uv run --with pyserial --with pillow python tools/screenshot.py <commands> <out.png> [port]
Each letter of <commands> is sent in turn, then the screen is captured. Use "-" to only capture.
"""
import sys
import time

import serial
from PIL import Image

BAUD = 460800
PORT = "/dev/cu.usbserial-5C9A0603501"
SCALE = 2


def send(port, command, expected, seconds=40):
    """Repeats the command until the device answers with the expected line."""
    deadline = time.time() + seconds
    while time.time() < deadline:
        port.reset_input_buffer()
        port.write(command.encode())
        line_deadline = time.time() + 1.5
        while time.time() < line_deadline:
            line = port.readline().decode("utf-8", "replace").strip()
            if line.startswith(expected):
                return line
    sys.exit(f"no answer to '{command}'")


commands, out = sys.argv[1], sys.argv[2]
port = serial.Serial(sys.argv[3] if len(sys.argv) > 3 else PORT, BAUD, timeout=0.5)
for command in commands.replace("-", ""):
    send(port, command, "OK")
    time.sleep(1.5)  # let the screen redraw

_, width, height = send(port, "s", "SHOT").split()
width, height = int(width), int(height)
size = width * height * 3
pixels = b""
deadline = time.time() + 15
# Changing the port's timeout here would drop the bytes that arrive meanwhile
while len(pixels) < size and time.time() < deadline:
    pixels += port.read(size - len(pixels))
port.close()
if len(pixels) != size:
    sys.exit(f"got {len(pixels)} bytes, expected {size}")
image = Image.frombytes("RGB", (width, height), pixels)
image.resize((width * SCALE, height * SCALE), Image.NEAREST).save(out)
print(out)
