# deskkit

Building blocks for small, always-on desk displays on M5Stack devices, and a simulator that runs them in a window on a Mac with their real data.

It lives in `lib/` of the first app that uses it (focusmate-hw), where PlatformIO finds it as a local library. It holds nothing about any one app, so it can move to a repo of its own and be pulled in with `lib_deps`.

## Three ideas

**Device**: what the app runs on. A device is a config file in `boards/`, like `core2.ini`. It has a section for the real device (`[core2]`) and one for the device in the simulator (`[core2_simulator]`). M5Unified already handles the hardware differences (screen, touch, buttons, speaker), so a new device is mostly a new ini file.

**Platform**: how the code runs: on an ESP32, or on the desktop. Only Wi-Fi, background tasks, web requests and the serial port differ, and they all sit behind `src/deskkit/platform/platform.h`:

- `platform/esp32.cpp`: Wi-Fi, SNTP clock, FreeRTOS tasks, HTTPClient, USB serial.
- `platform/desktop.cpp`: always online, threads, libcurl, the terminal as the serial port.
- `platform/desktop_main.cpp`: `main()` for the desktop. It runs the app's `setup()` and `loop()` while SDL draws the window.
- `desktop/Arduino.h`: the bits of the Arduino core the desktop lacks, like `String`, `millis` and `delay`.

Each `.cpp` compiles only on its own platform (`#ifdef ESP_PLATFORM`). Everything else runs the same on both.

**App**: the app's own `src/`, with Arduino's `setup()` and `loop()`. It calls `begin()` and `update()` and draws its screens with the building blocks below.

## Building blocks

| Header | What it gives |
| --- | --- |
| `deskkit/app.h` | `begin(config)` for Wi-Fi, clock, screen and console; `update()`; `wasTapped()` (touch, or the main button) |
| `deskkit/display.h` | `frame`, drawn a whole screen at a time and redrawn only when its key changes; `drawText`, `drawIcon`, `setBrightness` |
| `deskkit/theme.h` | the shared colors |
| `deskkit/color.h` | `blend`, `gradient` in RGB or OKLCH, `Oklch` |
| `deskkit/icon.h` | `Icon`, made from SVGs by `tools/icons.py` |
| `deskkit/grid.h` | a 2x3 (or 2x4) grid of values with icons and units |
| `deskkit/slides.h` | `SlideShow`: screens that take turns, with a notice bar |
| `deskkit/fetch.h` | `Fetch`: a JSON API polled in the background; `Shared<T>` to hand its data to the main loop |
| `deskkit/network.h` | `isWifiConnected`, `isTimeSynced` |
| `deskkit/timetext.h` | "7:24", "16 Oct", "W43" |
| `deskkit/isotime.h` | ISO 8601 times to and from seconds |
| `deskkit/sound.h` | `playNotes` |
| `deskkit/console.h` | debug commands over serial, and screenshots |

## Starting an app

```ini
; platformio.ini
[platformio]
extra_configs = lib/deskkit/boards/*.ini

[env:core2]
extends = core2

[env:simulator]
extends = core2_simulator
build_flags = ${core2_simulator.build_flags} -DDEBUG_TOOLS
```

```cpp
// src/main.cpp
#include <deskkit/app.h>
#include <deskkit/display.h>

const deskkit::Config CONFIG = {"wifi", "password", "CET-1CEST,M3.5.0,M10.5.0/3"};

void setup() { deskkit::begin(CONFIG); }

void loop() {
  deskkit::update();
  if (!deskkit::isNewFrame("hello")) return;
  deskkit::drawText("Hello", 160, 120, {&fonts::FreeSans18pt7b, deskkit::TEXT_WHITE}, middle_center);
  deskkit::pushFrame();
}
```

## Simulator

```sh
brew install sdl2
pio run -e simulator && .pio/build/simulator/program
```

A click is a tap, and sound plays through the Mac. Keys 1-6 resize the window. With `DEBUG_TOOLS`, the app's debug commands work in the terminal: type a letter and press Enter.

## Tools

- `tools/screenshot.py <commands> <out.png> [port|sim]` sends debug commands and saves the screen as a PNG, from the device over USB or from the simulator. The simulator runs without a window.
- `tools/icons.py <sketch.html> <icons.h>` turns the SVGs in an HTML sketch into `Icon`s.

## Rules

- C++11: that's what the ESP32's Arduino core builds with.
- Colors are `uint32_t` `0xRRGGBB`. M5GFX reads an `int` as RGB565.
- M5GFX takes `String` only on Arduino, so pass `.c_str()` to it.
- Don't name a project folder `native`. PlatformIO then reads `platform = native` as that folder.
