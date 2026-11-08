#pragma once

/*
 * The start of every deskkit app. Call begin() in setup() and update() first
 * thing in loop(); in between, the app draws its screens into the frame
 * (display.h). The same setup() and loop() run on the device and, through
 * platform/desktop_main.cpp, in the simulator.
 */

namespace deskkit {

struct Config {
  const char *wifiSsid;      // the ESP32 joins only 2.4 GHz networks
  const char *wifiPassword;
  const char *timeZone;      // POSIX TZ, like "CET-1CEST,M3.5.0,M10.5.0/3"
  bool isPortrait;           // taller than wide; left out, the screen is landscape
};

// Starts the device, the frame, Wi-Fi with the clock, and the debug console.
// The screen is turned to the config's orientation, whichever way the
// device's panel is built (the Core2's is landscape, the Tab5's portrait).
// The clock comes only from the internet, never from a clock chip, so the
// screen shows no time that wasn't synced.
void begin(const Config &config);

// Reads touch and buttons, and runs a debug command if one came in
void update();

// A tap on the touch screen, or a press of the main button on a device without one
bool wasTapped();

}  // namespace deskkit
