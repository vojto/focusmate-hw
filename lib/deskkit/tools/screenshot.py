"""
Drives a deskkit app's debug console (deskkit/console.h): sends one-letter
commands and saves the screen as a PNG, so the drawing can be checked from the
computer. Needs a build with DEBUG_TOOLS flashed, or, with "sim" as the port,
the app's `simulator` environment built, which it starts and closes. The
simulator runs without a window or sound, so nothing pops up.

Usage: uv run --with pyserial --with pillow python lib/deskkit/tools/screenshot.py <commands> <out.png> [port|sim]
Each letter of <commands> is sent in turn, then the screen is captured. Use "-" to only capture.
Without a port it takes the first USB serial port. Run it from the app's folder.
"""
import glob
import os
import subprocess
import sys
import threading
import time

import serial
from PIL import Image

BAUD = 460800
PORTS = "/dev/cu.usb*"  # usbserial-* for a USB-serial bridge (Core2), usbmodem* for native USB
SMALL_SCREEN = 480  # screens up to this wide are saved at twice their size, to be readable
SIMULATOR = ".pio/build/simulator/program"
SIMULATOR_STARTUP_S = 5  # long enough for the first fetches


class Simulator:
    """Stands in for the serial port: runs the simulator and talks to it over stdin and stdout."""

    def __init__(self):
        # No window or sound, so it doesn't pop up over what you're doing
        quiet = {**os.environ, "SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy"}
        self.process = subprocess.Popen([SIMULATOR], stdin=subprocess.PIPE, stdout=subprocess.PIPE, env=quiet)
        self.buffer = bytearray()
        self.lock = threading.Lock()
        threading.Thread(target=self._collect, daemon=True).start()
        time.sleep(SIMULATOR_STARTUP_S)

    def _collect(self):
        while chunk := self.process.stdout.read1(65536):
            with self.lock:
                self.buffer += chunk

    def _take(self, has_enough, count):
        deadline = time.time() + 0.5
        while time.time() < deadline:
            with self.lock:
                if has_enough():
                    taken = bytes(self.buffer[:count()])
                    del self.buffer[: len(taken)]
                    return taken
            time.sleep(0.01)
        return b""

    def write(self, data):
        self.process.stdin.write(data)
        self.process.stdin.flush()

    def reset_input_buffer(self):
        with self.lock:
            self.buffer.clear()

    def readline(self):
        return self._take(lambda: b"\n" in self.buffer, lambda: self.buffer.index(b"\n") + 1)

    def read(self, size):
        return self._take(lambda: self.buffer, lambda: size)

    def close(self):
        self.process.terminate()


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
port_name = sys.argv[3] if len(sys.argv) > 3 else next(iter(sorted(glob.glob(PORTS))), None)
if not port_name:
    sys.exit("no USB serial port; plug in the device, or use sim")
port = Simulator() if port_name == "sim" else serial.Serial(port_name, BAUD, timeout=0.5)
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
scale = 2 if width <= SMALL_SCREEN else 1
image.resize((width * scale, height * scale), Image.NEAREST).save(out)
print(out)
