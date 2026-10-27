#include "slides.h"
#include "grid.h"
#include "health.h"
#include "icons.h"
#include "network.h"

/*
 * The slides and the turns they take. Each slide is a function drawn like the
 * screens in screens.cpp: everything on its first draw, afterwards only what
 * changed. Most are grids (grid.h), described by a table of cells. The icons
 * come from icons.h, made by tools/icons.py from the sketch in
 * tools/sketch/dashboard.html.
 */

const uint32_t NOTICE_ORANGE = 0xEF9F27;
const uint32_t STEPS_GREEN = 0x5DCAA5, SLEEP_BLUE = 0x85B7EB, HRV_PINK = 0xED93B1, RHR_RED = 0xE24B4A;
// Sleep in half-hour buckets: warm for short nights, yellow in the middle
// (6:00-6:30), green from 6:30 and sky blue from 8:00, each about as bright
// on black (made in OKLCH; see tools/sketch/sleep-colors.html)
const int SLEEP_BUCKET_MINUTES[] = {330, 360, 390, 420, 450, 480};
const uint32_t SLEEP_COLORS[] = {0xFC5855, 0xFF9044, 0xFBD530, 0xADE74E, 0x59DE65, 0x29D5A5, 0x30C0F8};
const uint32_t RECENT_GRAY = 0x222220;  // behind the first row, last night and this week

#define SLIDE_MS 60000          // how long a slide stays
#define NOTICE_HEIGHT 20        // a bar across the top, above the first row's text

#define BIRTH_TIME 681343200     // 5 Aug 1991, midnight in Bratislava
#define SECONDS_PER_YEAR (365.2425 * 86400)

static void showSlide(int index);
static void drawNotice(const char *notice);
static void showDashboard(bool isFirstDraw, time_t now);
static void showSleep(bool isFirstDraw, time_t now);
static String dateText(time_t now);
static String ageText(time_t now);
static String stepsText(time_t now);
static String sleepText(time_t now);
static String hrvText(time_t now);
static String restingHeartRateText(time_t now);
static String healthNumber(int number);
static String weekLabel(time_t now, int week);
static uint32_t sleepColor(int minutes);
static String hoursAndMinutes(int minutes);

typedef void (*Slide)(bool isFirstDraw, time_t now);
const Slide SLIDES[] = {showDashboard, showSleep};
const int SLIDE_COUNT = sizeof(SLIDES) / sizeof(SLIDES[0]);

static int slide = 0;
static uint32_t slideShownAt = 0;

// MARK: Taking turns

void showSlides(time_t now, const char *notice) {
  static String shownNotice;
  bool isBackFromSession = shownScreen() < SCREEN_SLIDE;
  if (isBackFromSession) showSlide(0);
  else if (millis() - slideShownAt >= SLIDE_MS) nextSlide();

  bool isFirstDraw = enterScreen(SCREEN_SLIDE + slide);
  // The notice bar crosses the slide, so a change redraws everything
  if (shownNotice != notice) {
    shownNotice = notice;
    clearScreen();
    isFirstDraw = true;
  }
  SLIDES[slide](isFirstDraw, now);
  if (isFirstDraw) drawNotice(notice);
}

void nextSlide() { showSlide((slide + 1) % SLIDE_COUNT); }

bool isLastSlide() { return slide == SLIDE_COUNT - 1; }

static void showSlide(int index) {
  slide = index;
  slideShownAt = millis();
}

static void drawNotice(const char *notice) {
  if (!*notice) return;
  M5.Display.fillRect(0, 0, M5.Display.width(), NOTICE_HEIGHT, NOTICE_ORANGE);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(BACKGROUND, NOTICE_ORANGE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextPadding(0);
  M5.Display.drawString(notice, CENTER_X, NOTICE_HEIGHT / 2);
}

// MARK: Dashboard

// The date, Vojto's age and today's steps, sleep, HRV and resting heart rate
const GridCell DASHBOARD[GRID_CELLS] = {
    {ICON_CALENDAR, TEXT_GRAY, dateText},
    {ICON_CLOCK, TEXT_GRAY, ageText},
    {ICON_STEPS, STEPS_GREEN, stepsText},
    {ICON_SLEEP, SLEEP_BLUE, sleepText},
    {ICON_HRV, HRV_PINK, hrvText, "ms"},
    {ICON_RHR, RHR_RED, restingHeartRateText, "bpm"},
};

static void showDashboard(bool isFirstDraw, time_t now) { showGrid(DASHBOARD, isFirstDraw, now); }

// "16 Oct", "6 Oct"
static String dateText(time_t now) {
  if (!isTimeSynced()) return "--";
  char text[16];
  struct tm local;
  localtime_r(&now, &local);
  strftime(text, sizeof(text), "%d %b", &local);
  return text[0] == '0' ? text + 1 : text;
}

// Vojto's age in years: "35.21"
static String ageText(time_t now) {
  if (!isTimeSynced()) return "--";
  return String((now - BIRTH_TIME) / SECONDS_PER_YEAR, 2);
}

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

// MARK: Sleep

// Last night, then the average night of this week so far and of the six weeks
// before it, each labeled with its week number, in four rows
static void showSleep(bool isFirstDraw, time_t now) {
  if (isFirstDraw) {
    drawGridLines(4);
    for (int cell : {0, 1}) fillGridCell(cell, RECENT_GRAY);
  }
  Health health = todaysHealth();
  drawGridValue(0, hoursAndMinutes(health.sleepMinutes), sleepColor(health.sleepMinutes), "night", RECENT_GRAY);
  for (int week = 0; week < SLEEP_WEEKS; week++) {
    int minutes = health.weeklySleepMinutes[week];
    drawGridValue(week + 1, hoursAndMinutes(minutes), sleepColor(minutes), weekLabel(now, week), week == 0 ? RECENT_GRAY : BACKGROUND);
  }
}

// The ISO week number: "W43". 0 is this week, 1 the week before.
static String weekLabel(time_t now, int week) {
  if (!isTimeSynced()) return "";
  time_t then = now - week * 7 * 86400;
  struct tm local;
  localtime_r(&then, &local);
  char number[4];
  strftime(number, sizeof(number), "%V", &local);
  return "W" + String(atoi(number));
}

static uint32_t sleepColor(int minutes) {
  if (minutes == HEALTH_MISSING) return TEXT_WHITE;
  int bucket = 0;
  while (bucket < 6 && minutes >= SLEEP_BUCKET_MINUTES[bucket]) bucket++;
  return SLEEP_COLORS[bucket];
}

// "7:24"
static String hoursAndMinutes(int minutes) {
  if (minutes == HEALTH_MISSING) return "--";
  char text[12];
  snprintf(text, sizeof(text), "%d:%02d", minutes / 60, minutes % 60);
  return text;
}
