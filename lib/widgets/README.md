# widgets

The widgets the desk display apps pick from. Each app lists the ones it wants in a deskkit `Rotation` (see [deskkit](../deskkit/README.md)), so the same widget can show on a Core2 and a Tab5.

| Widget | Shows | Priority |
| --- | --- | --- |
| `CLOCK_WIDGET` | the time and date | normal |
| `FOCUSMATE_WIDGET` | a countdown before a session, a pie during it; chimes and ticks | urgent from 10 minutes before a session to its end, hidden otherwise |
| `DASHBOARD_WIDGET` | date, age, steps, sleep, HRV, resting heart rate | normal |
| `SLEEP_WIDGET` | last night and seven weekly averages | normal |
| `WEIGHT_WIDGET` | weekly averages as rows of bars | normal |
| `WEIGHT_RANGE_WIDGET` | weekly lightest to heaviest weigh-in, as columns | normal |

All are 320x240.

## Layout

- `src/widgets/widgets.h`: every widget, declared.
- `src/widgets/<name>.cpp`: one widget each. `focusmate_screens.cpp` draws Focusmate's pie and countdown.
- `src/widgets/data/`: the fetches (Focusmate sessions, Moonlite health, Foodmark weight). A widget starts the ones it needs; each starts only once.
- `src/widgets/icons.h`: made by `../deskkit/tools/icons.py` from `apps/focusmate/tools/sketch/dashboard.html`.

## Keys

The data files include `secrets.h` from the app's `include/` folder: `FOCUSMATE_API_KEY`, `MOONLITE_API_TOKEN`, `FOODMARK_API_TOKEN`. The whole library is built for every app, so every app needs all of them.

## Adding a widget

Write `src/widgets/<name>.cpp` with a `const deskkit::Widget <NAME>_WIDGET = {key, draw, start, priority, update};` (leave out the trailing ones it doesn't need), declare it in `widgets.h`, and list it in an app.
