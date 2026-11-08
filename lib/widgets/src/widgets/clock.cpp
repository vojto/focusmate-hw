#include <deskkit/network.h>
#include "widgets.h"

/*
 * The time, large, and the date under it. Shows "--:--" until the clock is
 * synced, so it never shows a time that didn't come from the internet.
 */

using namespace deskkit;

#define TIME_SIZE 2             // Font7 is 48px tall, so 96px
#define TIME_RISE 20            // the time sits this far above the middle
#define DATE_GAP 28             // from the time's bottom to the date's middle

const uint32_t TIME_GREEN = 0x5DCAA5;
const TextStyle DATE_TEXT = {&fonts::FreeSans12pt7b, TEXT_GRAY};

static String clockKey(time_t now);
static void drawClock(time_t now);
static String timeText(time_t now);
static String dateText(time_t now);

const Widget CLOCK_WIDGET = {
    clockKey,
    drawClock,
};

// Changes once a minute
static String clockKey(time_t now) { return timeText(now) + " " + dateText(now); }

static void drawClock(time_t now) {
  int middle = frame.height() / 2 - TIME_RISE;
  frame.setFont(&fonts::Font7);
  frame.setTextSize(TIME_SIZE);
  frame.setTextColor(TIME_GREEN);
  frame.setTextDatum(middle_center);
  frame.drawString(timeText(now).c_str(), frame.width() / 2, middle);
  int dateMiddle = middle + frame.fontHeight() / 2 + DATE_GAP;
  drawText(dateText(now), frame.width() / 2, dateMiddle, DATE_TEXT, middle_center);
}

// "9:05", "14:30"
static String timeText(time_t now) {
  if (!isTimeSynced()) return "--:--";
  struct tm local;
  localtime_r(&now, &local);
  char text[8];
  snprintf(text, sizeof(text), "%d:%02d", local.tm_hour, local.tm_min);
  return text;
}

// "Sunday, 8 November"
static String dateText(time_t now) {
  if (!isTimeSynced()) return "";
  struct tm local;
  localtime_r(&now, &local);
  char weekday[16], month[16], text[40];
  strftime(weekday, sizeof(weekday), "%A", &local);
  strftime(month, sizeof(month), "%B", &local);
  snprintf(text, sizeof(text), "%s, %d %s", weekday, local.tm_mday, month);
  return text;
}
