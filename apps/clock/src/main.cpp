#include <Arduino.h>
#include <M5Unified.h>
#include <deskkit/app.h>
#include <deskkit/console.h>
#include <widgets/widgets.h>
#include "secrets.h"

/*
 * The desk display on the M5Stack Tab5. For now it shows one widget at a time
 * in the middle of the screen, like a Core2 would: the clock, then sleep,
 * taking turns every minute or on a tap. Showing several widgets at once on
 * the big screen comes next.
 */

using namespace deskkit;

#define WIDGET_MS 60000     // how long a widget stays
#define LOOP_MS 20
#define TAP_HZ 2000         // the click that confirms a tap
#define TAP_MS 15

// Europe/Bratislava, including daylight saving time changes
const Config CONFIG = {WIFI_SSID, WIFI_PASSWORD, "CET-1CEST,M3.5.0,M10.5.0/3"};

static const Widget *const WIDGETS[] = {&CLOCK_WIDGET, &SLEEP_WIDGET};
static Rotation rotation(WIDGETS, WIDGET_MS);

void setup() {
  begin(CONFIG);
  addCommand('t', [] { rotation.tap(); });  // a tap: the next widget
}

void loop() {
  update();
  if (wasTapped()) {
    M5.Speaker.tone(TAP_HZ, TAP_MS);
    rotation.tap();
  }
  rotation.show(time(nullptr));
  delay(LOOP_MS);
}
