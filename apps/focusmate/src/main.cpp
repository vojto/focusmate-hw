#include <Arduino.h>
#include <M5Unified.h>
#include <deskkit/app.h>
#include <deskkit/display.h>
#include <deskkit/network.h>
#include <widgets/data/sessions.h>
#include <widgets/widgets.h>
#include "debug.h"
#include "secrets.h"

/*
 * Focusmate session display for an M5Stack Core2. Its widgets take turns every
 * minute, or on a tap; around a session the Focusmate widget is urgent and
 * stays, and a tap peeks at the others (see deskkit/widgets.h). The screen
 * dims at night, unless something is urgent.
 */

using namespace deskkit;

#define WIDGET_MS 60000     // how long a widget stays
#define BRIGHTNESS 255      // 0-255, the most
#define NIGHT_BRIGHTNESS 1  // the dimmest that still shows anything
#define NIGHT_START_HOUR 23 // night runs from here to NIGHT_END_HOUR, local time
#define NIGHT_END_HOUR 7
#define VOLUME 128          // 0-255
#define LOOP_MS 20          // short, so the session's ticks land evenly
#define TAP_HZ 2000         // the click that confirms a tap
#define TAP_MS 15

// Europe/Bratislava, including daylight saving time changes
const Config CONFIG = {WIFI_SSID, WIFI_PASSWORD, "CET-1CEST,M3.5.0,M10.5.0/3"};

// WEIGHT_WIDGET, the weekly averages as rows of bars, is off for now (2026-10-30)
static const Widget *const WIDGETS[] = {&FOCUSMATE_WIDGET, &DASHBOARD_WIDGET, &SLEEP_WIDGET, &WEIGHT_RANGE_WIDGET};
Rotation rotation(WIDGETS, WIDGET_MS);

static void updateBrightness(time_t now);

void setup() {
  begin(CONFIG);
  M5.Speaker.setVolume(VOLUME);
  addDebugCommands();
}

void loop() {
  update();
  time_t now = time(nullptr);
  if (wasTapped()) {
    M5.Speaker.tone(TAP_HZ, TAP_MS);
    rotation.tap();
  }
  rotation.show(now, fetchProblem());
  updateBrightness(now);
  delay(LOOP_MS);
}

// Dim at night, unless a widget is urgent; full before the clock is set
static void updateBrightness(time_t now) {
  struct tm local;
  localtime_r(&now, &local);
  bool isNight = isTimeSynced() && (local.tm_hour >= NIGHT_START_HOUR || local.tm_hour < NIGHT_END_HOUR);
  setBrightness(isNight && !rotation.isUrgent() ? NIGHT_BRIGHTNESS : BRIGHTNESS);
}
