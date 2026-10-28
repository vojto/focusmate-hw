#include "slides.h"
#include "grid.h"
#include "health.h"
#include "icons.h"
#include "network.h"
#include "weight.h"

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
// Weight bars, lightest week shown to heaviest: the sleep colors from green
// to red (see tools/sketch/weight-bars.html)
const uint32_t WEIGHT_COLORS[] = {0x59DE65, 0xADE74E, 0xFBD530, 0xFF9044, 0xFC5855};
const int WEIGHT_COLOR_COUNT = sizeof(WEIGHT_COLORS) / sizeof(WEIGHT_COLORS[0]);
const uint32_t RECENT_GRAY = 0x222220;  // behind the first row, last night and this week

#define SLIDE_MS 60000          // how long a slide stays
#define TAP_HZ 2000             // the click that confirms a tap
#define TAP_MS 15
#define NOTICE_HEIGHT 20        // a bar across the top, above the first row's text

// The weight slide: a row per week, a bar of half-kilo segments and the weight
#define WEIGHT_ROW_HEIGHT 30    // 240 / WEIGHT_WEEKS
#define WEIGHT_BAR_HEIGHT 26
#define WEIGHT_PADDING 12
#define WEIGHT_BAR_LEFT 56      // after the week labels
#define WEIGHT_VALUE_GAP 8      // between a bar and its weight
#define WEIGHT_SEGMENT_GRAMS 500
#define WEIGHT_SEGMENT_GAP 2
#define WEIGHT_SEGMENT_RADIUS 3
#define CHEVRON_SIZE 4          // half its height
#define CHEVRON_GAP 7           // on each side of it

#define BIRTH_TIME 681343200     // 5 Aug 1991, midnight in Bratislava
#define SECONDS_PER_YEAR (365.2425 * 86400)

static void showSlide(int index);
static void redrawSlide();
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
static void showWeight(bool isFirstDraw, time_t now);
static void drawWeightRow(int row, int grams, int todayGrams, int lightest, int heaviest, time_t now);
static int drawWeightBar(int y, int grams, int lightest, int heaviest);
static int drawWeightValue(int x, int y, int grams, uint32_t color);
static void drawChevron(int x, int y);
static String kilograms(int grams);
static uint32_t weightColor(int grams, int lightest, int heaviest);
static uint32_t blend(uint32_t from, uint32_t to, float amount);

typedef void (*Slide)(bool isFirstDraw, time_t now);
const Slide SLIDES[] = {showDashboard, showSleep, showWeight};
const int SLIDE_COUNT = sizeof(SLIDES) / sizeof(SLIDES[0]);

static int slide = 0;
static bool isRedrawWanted = false;
static uint32_t slideShownAt = 0;

// MARK: Taking turns

void showSlides(time_t now, const char *notice) {
  static String shownNotice;
  bool isBackFromSession = shownScreen() < SCREEN_SLIDE;
  if (isBackFromSession) showSlide(0);
  else if (M5.Touch.getDetail().wasPressed()) {
    M5.Speaker.tone(TAP_HZ, TAP_MS);
    nextSlide();
  } else if (millis() - slideShownAt >= SLIDE_MS) nextSlide();

  bool isFirstDraw = enterScreen(SCREEN_SLIDE + slide);
  // The notice bar crosses the slide, so a change redraws everything
  if (shownNotice != notice || isRedrawWanted) {
    shownNotice = notice;
    isRedrawWanted = false;
    clearScreen();
    isFirstDraw = true;
  }
  SLIDES[slide](isFirstDraw, now);
  if (isFirstDraw) drawNotice(notice);
}

void nextSlide() { showSlide((slide + 1) % SLIDE_COUNT); }

static void showSlide(int index) {
  slide = index;
  slideShownAt = millis();
}

// For a slide whose drawing can't be updated in place: everything is drawn
// again on the next loop, the notice too
static void redrawSlide() { isRedrawWanted = true; }

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

// MARK: Weight

static Weight shownWeight;

// The average weight of this week and the seven before it, as bars that start
// a kilo under the lightest week (from zero they would all look the same),
// colored from green for the lightest to red for the heaviest. This week's
// average is followed by a chevron and today's weigh-in.
static void showWeight(bool isFirstDraw, time_t now) {
  Weight weight = latestWeight();
  if (!isFirstDraw) {
    if (memcmp(&weight, &shownWeight, sizeof(weight)) != 0) redrawSlide();
    return;
  }
  shownWeight = weight;
  int lightest = INT_MAX, heaviest = 0;
  for (int grams : weight.weeklyGrams) {
    if (grams == WEIGHT_MISSING) continue;
    lightest = min(lightest, grams);
    heaviest = max(heaviest, grams);
  }
  for (int week = 0; week < WEIGHT_WEEKS; week++) {
    drawWeightRow(week, weight.weeklyGrams[week], week == 0 ? weight.todayGrams : WEIGHT_MISSING, lightest, heaviest, now);
  }
}

// The week's label, bar and weight. With today's weigh-in, the week's weight
// turns gray and today's follows it after a chevron.
static void drawWeightRow(int row, int grams, int todayGrams, int lightest, int heaviest, time_t now) {
  int y = row * WEIGHT_ROW_HEIGHT + WEIGHT_ROW_HEIGHT / 2;
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(TEXT_GRAY, BACKGROUND);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextPadding(0);
  M5.Display.drawString(weekLabel(now, row), WEIGHT_PADDING, y);
  int x = drawWeightBar(y, grams, lightest, heaviest);
  if (todayGrams == WEIGHT_MISSING) {
    drawWeightValue(x, y, grams, TEXT_WHITE);
    return;
  }
  x = drawWeightValue(x, y, grams, TEXT_GRAY) + CHEVRON_GAP;
  drawChevron(x, y);
  drawWeightValue(x + CHEVRON_SIZE + CHEVRON_GAP - WEIGHT_VALUE_GAP, y, todayGrams, TEXT_WHITE);
}

// Half-kilo segments, the last one cut at the weight rounded to 100 g.
// Returns where the bar ends.
static int drawWeightBar(int y, int grams, int lightest, int heaviest) {
  if (grams == WEIGHT_MISSING) return WEIGHT_BAR_LEFT;
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  int start = (lightest / 1000 - 1) * 1000;
  int end = (heaviest + 999) / 1000 * 1000;
  int right = M5.Display.width() - WEIGHT_PADDING - WEIGHT_VALUE_GAP - M5.Display.textWidth("109.0");
  auto x = [&](int g) { return WEIGHT_BAR_LEFT + (int)lroundf((g - start) * (right - WEIGHT_BAR_LEFT) / (float)(end - start)); };

  int rounded = (grams + 50) / 100 * 100;
  uint32_t color = weightColor(grams, lightest, heaviest);
  for (int from = start; from < rounded; from += WEIGHT_SEGMENT_GRAMS) {
    int left = x(from) + WEIGHT_SEGMENT_GAP / 2;
    int width = max(x(min(from + WEIGHT_SEGMENT_GRAMS, rounded)) - left - WEIGHT_SEGMENT_GAP / 2, 1);
    M5.Display.fillSmoothRoundRect(left, y - WEIGHT_BAR_HEIGHT / 2, width, WEIGHT_BAR_HEIGHT, WEIGHT_SEGMENT_RADIUS, color);
  }
  return x(rounded);
}

// After a gap; returns where the text ends
static int drawWeightValue(int x, int y, int grams, uint32_t color) {
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(color, BACKGROUND);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextPadding(0);
  x += WEIGHT_VALUE_GAP;
  return x + M5.Display.drawString(kilograms(grams), x, y);
}

static void drawChevron(int x, int y) {
  M5.Display.drawWideLine(x, y - CHEVRON_SIZE, x + CHEVRON_SIZE, y, 1, TEXT_GRAY);
  M5.Display.drawWideLine(x + CHEVRON_SIZE, y, x, y + CHEVRON_SIZE, 1, TEXT_GRAY);
}

// "107.1"
static String kilograms(int grams) {
  if (grams == WEIGHT_MISSING) return "--";
  int tenths = (grams + 50) / 100;  // rounded like the bar's end
  return String(tenths / 10) + "." + String(tenths % 10);
}

static uint32_t weightColor(int grams, int lightest, int heaviest) {
  if (heaviest == lightest) return WEIGHT_COLORS[0];
  float position = (grams - lightest) / (float)(heaviest - lightest) * (WEIGHT_COLOR_COUNT - 1);
  int from = min((int)position, WEIGHT_COLOR_COUNT - 2);
  return blend(WEIGHT_COLORS[from], WEIGHT_COLORS[from + 1], position - from);
}

// amount 0 is from, 1 is to
static uint32_t blend(uint32_t from, uint32_t to, float amount) {
  uint32_t color = 0;
  for (int shift : {16, 8, 0}) {
    int a = from >> shift & 0xFF, b = to >> shift & 0xFF;
    color |= (uint32_t)lroundf(a + (b - a) * amount) << shift;
  }
  return color;
}
