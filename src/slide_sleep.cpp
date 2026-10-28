#include "grid.h"
#include "health.h"
#include "slide.h"

/*
 * Last night, then the average night of this week so far and of the six weeks
 * before it, each labeled with its week number, in a 2x4 grid. The first row
 * has a dark gray background.
 */

// Sleep in half-hour buckets: warm for short nights, yellow in the middle
// (6:00-6:30), green from 6:30 and sky blue from 8:00, each about as bright
// on black (made in OKLCH; see tools/sketch/sleep-colors.html)
const int SLEEP_BUCKET_MINUTES[] = {330, 360, 390, 420, 450, 480};
const uint32_t SLEEP_COLORS[] = {0xFC5855, 0xFF9044, 0xFBD530, 0xADE74E, 0x59DE65, 0x29D5A5, 0x30C0F8};
const uint32_t RECENT_GRAY = 0x222220;  // behind the first row, last night and this week

static String sleepKey(time_t now);
static void drawSleep(time_t now);
static uint32_t sleepColor(int minutes);

const Slide SLEEP_SLIDE = {
    sleepKey,
    drawSleep,
};

// Changes with the numbers and the week
static String sleepKey(time_t now) { return keyOf(todaysHealth()) + weekLabel(now, 0); }

static void drawSleep(time_t now) {
  Health health = todaysHealth();
  drawGridLines(4);
  for (int cell : {0, 1}) fillGridCell(cell, RECENT_GRAY);
  drawGridValue(0, hoursAndMinutes(health.sleepMinutes), sleepColor(health.sleepMinutes), "night");
  for (int week = 0; week < SLEEP_WEEKS; week++) {
    int minutes = health.weeklySleepMinutes[week];
    drawGridValue(week + 1, hoursAndMinutes(minutes), sleepColor(minutes), weekLabel(now, week));
  }
}

static uint32_t sleepColor(int minutes) {
  if (minutes == HEALTH_MISSING) return TEXT_WHITE;
  int bucket = 0;
  while (bucket < 6 && minutes >= SLEEP_BUCKET_MINUTES[bucket]) bucket++;
  return SLEEP_COLORS[bucket];
}
