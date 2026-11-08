# Desk displays

Small always-on displays for the desk, built on M5Stack devices. Each app lives in `apps/` and runs on a real device or in a simulator on a Mac. The code they share is a framework, deskkit, in `lib/deskkit`.

| App | Device | What it shows |
| --- | --- | --- |
| [focusmate](apps/focusmate) | Core2 | [Focusmate](https://www.focusmate.com) sessions as a Time Timer-like pie, plus health and weight widgets |
| [clock](apps/clock) | Tab5 | The clock and sleep widgets, one at a time for now |

## Layout

```
apps/<name>/            one PlatformIO project per app
  platformio.ini        picks the device, from lib/deskkit/boards
  src/                  the app's code: which widgets it shows, and how
  include/secrets.h     keys and Wi-Fi (not in git)
lib/deskkit/            the shared framework
  boards/               devices: core2.ini, tab5.ini, desktop.ini
  src/deskkit/          widgets and their rotation, drawing, data fetching, debug console, platform layer
  tools/                screenshots and icons
lib/widgets/            the widgets the apps pick from: Focusmate, sleep, weight, clock...
```

An app is a list of widgets. Each widget says how much it matters right now: normal widgets take turns, an urgent one (Focusmate, near a session) takes over, a hidden one is skipped.

## Working on an app

Every app has the same three environments: the device, the device with debug tools, and the simulator. From the app's folder:

```sh
cd apps/focusmate
cp include/secrets.example.h include/secrets.h   # then fill it in
pio run -t upload                        # flash the device
pio run -e simulator && .pio/build/simulator/program
```

Or from the repo's root, with `-d`:

```sh
pio run -d apps/clock -e simulator
```

The simulator needs `brew install sdl2`.

## Starting a new app

1. Make `apps/<name>/` with a `platformio.ini` like [the clock's](apps/clock/platformio.ini), extending a device from `lib/deskkit/boards`.
2. Write `src/main.cpp`: list the widgets in a `deskkit::Rotation`, call `deskkit::begin()` in `setup()`, and `deskkit::update()` and the rotation's `show()` in `loop()`. [The clock's](apps/clock/src/main.cpp) is the shortest.
3. Copy an `include/secrets.example.h` to `include/secrets.h`; the widgets need every key in it.
4. For a device that has no file in `boards/` yet, add one.

See [lib/deskkit/README.md](lib/deskkit/README.md) for what the framework offers, and [lib/widgets/README.md](lib/widgets/README.md) for the widgets.

## License

MIT, see [LICENSE](LICENSE).
