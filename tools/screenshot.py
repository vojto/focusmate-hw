"""
Drives the debug firmware (src/debug.h) over serial: sends one-letter commands
and saves the device's screen as a PNG, so the drawing can be checked from the
computer. Needs the `debug` environment flashed, or, with "sim" as the port,
the desktop simulator built (`pio run -e native`), which it starts and closes.

Usage: uv run --with pyserial --with pillow python tools/screenshot.py <commands> <out.png> [port|sim]
Each letter of <commands> is sent in turn, then the screen is captured. Use "-" to only capture.
"""
import os
import subprocess
import sys
import threading
import time

import serial
from PIL import Image

BAUD = 460800
PORT = "/dev/cu.usbserial-5C9A0603501"
SCALE = 2
SIMULATOR = ".pio/build/native/program"
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
port_name = sys.argv[3] if len(sys.argv) > 3 else PORT
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
image.resize((width * SCALE, height * SCALE), Image.NEAREST).save(out)
print(out)
