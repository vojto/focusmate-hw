#include <deskkit/grid.h>
#include <deskkit/timetext.h>
#include "age.h"
#include "data/health.h"
#include "icons.h"
#include "widgets.h"

/*
 * The date, Vojto's age and today's steps, sleep, HRV and resting heart rate
 * in a 2x3 grid, each marked by an icon. The icons come from icons.h, made by
 * tools/icons.py from the sketch in tools/sketch/dashboard.html.
 */

using namespace deskkit;

const uint32_t STEPS_GREEN = 0x5DCAA5, SLEEP_BLUE = 0x85B7EB, HRV_PINK = 0xED93B1, RHR_RED = 0xE24B4A;

static String stepsText(time_t now);
static String sleepText(time_t now);
static String hrvText(time_t now);
static String restingHeartRateText(time_t now);
static String healthNumber(int number);

const GridCell DASHBOARD[GRID_CELLS] = {
    {&ICON_CALENDAR, TEXT_GRAY, dayAndMonth},
    {&ICON_CLOCK, TEXT_GRAY, ageText},
    {&ICON_STEPS, STEPS_GREEN, stepsText},
    {&ICON_SLEEP, SLEEP_BLUE, sleepText},
    {&ICON_HRV, HRV_PINK, hrvText, "ms"},
    {&ICON_RHR, RHR_RED, restingHeartRateText, "bpm"},
};

const Widget DASHBOARD_WIDGET = {
    [](time_t now) { return gridKey(DASHBOARD, now); },
    [](time_t now) { drawGrid(DASHBOARD, now); },
    startFetchingHealth,
};

// "950", "8.4k"
static String stepsText(time_t now) {
  int steps = todaysHealth().steps;
  if (steps == HEALTH_MISSING) return "--";
  if (steps < 1000) return String(steps);
  char text[12];
  snprintf(text, sizeof(text), "%d.%dk", steps / 1000, steps % 1000 / 100);
  return text;
}

static String sleepText(time_t now) { return hoursAndMinutes(todaysHealth().sleepMinutes); }

static String hrvText(time_t now) { return healthNumber(todaysHealth().hrvMs); }

static String restingHeartRateText(time_t now) { return healthNumber(todaysHealth().restingHeartRate); }

static String healthNumber(int number) { return number == HEALTH_MISSING ? "--" : String(number); }
