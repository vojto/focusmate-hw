#include <Arduino.h>
#include <deskkit/app.h>
#include <deskkit/display.h>
#include <deskkit/network.h>
#include "secrets.h"

/*
 * A desk clock for the M5Stack Tab5: the time, large, and the date under it.
 * It is the first app on the Tab5, a starting point that shows the screen,
 * Wi-Fi and the internet clock all work; the time shows only once the clock
 * is synced.
 */

using namespace deskkit;

#define TIME_SIZE 5             // Font7 is 48px tall, so 240px
#define TIME_RISE 40            // the time sits this far above the middle
#define DATE_GAP 70             // from the time's bottom to the date's middle
#define LOOP_MS 100

const uint32_t TIME_GREEN = 0x5DCAA5;
const lgfx::IFont *const DATE_FONT = &fonts::FreeSans24pt7b;

// Europe/Bratislava, including daylight saving time changes
const Config CONFIG = {WIFI_SSID, WIFI_PASSWORD, "CET-1CEST,M3.5.0,M10.5.0/3"};

static void showClock(time_t now);
static String timeText(const struct tm &local);
static String dateText(const struct tm &local);

void setup() { begin(CONFIG); }

void loop() {
  update();
  showClock(time(nullptr));
  delay(LOOP_MS);
}

// Redrawn when the minute changes
static void showClock(time_t now) {
  struct tm local;
  localtime_r(&now, &local);
  String time = isTimeSynced() ? timeText(local) : "--:--";
  String date = isTimeSynced() ? dateText(local) : "";
  if (!isNewFrame(time + " " + date)) return;
  int middle = frame.height() / 2 - TIME_RISE;
  frame.setFont(&fonts::Font7);
  frame.setTextSize(TIME_SIZE);
  frame.setTextColor(TIME_GREEN);
  frame.setTextDatum(middle_center);
  frame.drawString(time.c_str(), frame.width() / 2, middle);
  int dateMiddle = middle + frame.fontHeight() / 2 + DATE_GAP;
  drawText(date, frame.width() / 2, dateMiddle, {DATE_FONT, TEXT_GRAY}, middle_center);
  pushFrame();
}

// "9:05", "14:30"
static String timeText(const struct tm &local) {
  char text[8];
  snprintf(text, sizeof(text), "%d:%02d", local.tm_hour, local.tm_min);
  return text;
}

// "Sunday, 8 November"
static String dateText(const struct tm &local) {
  char weekday[16], month[16], text[40];
  strftime(weekday, sizeof(weekday), "%A", &local);
  strftime(month, sizeof(month), "%B", &local);
  snprintf(text, sizeof(text), "%s, %d %s", weekday, local.tm_mday, month);
  return text;
}
