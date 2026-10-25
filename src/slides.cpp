#include "slides.h"
#include "display.h"
#include "health.h"
#include "icons.h"
#include "network.h"

/*
 * The slides and the turns they take. Each slide is a function drawn like the
 * screens in screens.cpp: everything on its first draw, afterwards only what
 * changed. The dashboard's icons come from icons.h, made by tools/icons.py
 * from the sketch in tools/sketch/dashboard.html.
 */

const uint32_t NOTICE_ORANGE = 0xEF9F27, DIVIDER_GRAY = 0x444441;
const uint32_t STEPS_GREEN = 0x5DCAA5, SLEEP_BLUE = 0x85B7EB, HRV_PINK = 0xED93B1, RHR_RED = 0xE24B4A;

#define SLIDE_MS 60000          // how long a slide stays
#define TAP_HZ 2000             // the click that confirms a tap
#define TAP_MS 15
#define NOTICE_HEIGHT 20        // a bar across the top, above the first row's text

// The dashboard is a grid of two columns and three rows. Each cell has an icon
// on the left and its value next to it, vertically centered.
#define CELL_WIDTH 160
#define CELL_HEIGHT 80
#define CELL_PADDING 12
#define ICON_GAP 8              // between the icon and the value
#define TEXT_LEFT (CELL_PADDING + ICON_SIZE + ICON_GAP)
#define TEXT_BOX_HEIGHT 40      // what gets cleared around a value
#define UNIT_GAP 3              // between a value and its unit, like "48 ms"
#define VALUE_BASELINE 12       // below a cell's center, about half the cap height
#define BIRTH_TIME 681343200     // 5 Aug 1991, midnight in Bratislava
#define SECONDS_PER_YEAR (365.2425 * 86400)

// A cell of the dashboard's grid. Each needs its own slot, like a TextLine.
struct Cell {
  int slot;
  int column;
  int row;
  const uint8_t *icon;
  uint32_t iconColor;
};

const Cell DATE_CELL = {0, 0, 0, ICON_CALENDAR, TEXT_GRAY};
const Cell AGE_CELL = {1, 1, 0, ICON_CLOCK, TEXT_GRAY};
const Cell STEPS_CELL = {2, 0, 1, ICON_STEPS, STEPS_GREEN};
const Cell SLEEP_CELL = {3, 1, 1, ICON_SLEEP, SLEEP_BLUE};
const Cell HRV_CELL = {4, 0, 2, ICON_HRV, HRV_PINK};
const Cell RHR_CELL = {5, 1, 2, ICON_RHR, RHR_RED};

const TextLine PLACEHOLDER_TEXT = {0, CENTER_Y, &fonts::FreeSans18pt7b, 1, TEXT_GRAY};

static void showSlide(int index);
static void drawNotice(const char *notice);
static void showDashboard(bool isFirstDraw, time_t now);
static void showPlaceholder(bool isFirstDraw, time_t now);
static void drawGrid();
static void drawIcon(const Cell &cell);
static void drawCell(const Cell &cell, const String &value, const char *unit = "");
static String formatDate(time_t when);
static String formatAge(time_t now);
static String formatSteps(int steps);
static String formatSleep(int minutes);
static String formatNumber(int number);

typedef void (*Slide)(bool isFirstDraw, time_t now);
const Slide SLIDES[] = {showDashboard, showPlaceholder};
const int SLIDE_COUNT = sizeof(SLIDES) / sizeof(SLIDES[0]);

static int slide = 0;
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
  if (shownNotice != notice) {
    shownNotice = notice;
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

// The date, Vojto's age and today's steps, sleep, HRV and resting heart rate,
// each with an icon
static void showDashboard(bool isFirstDraw, time_t now) {
  if (isFirstDraw) drawGrid();
  Health health = todaysHealth();
  bool isSynced = isTimeSynced();
  drawCell(DATE_CELL, isSynced ? formatDate(now) : "--");
  drawCell(AGE_CELL, isSynced ? formatAge(now) : "--");
  drawCell(STEPS_CELL, formatSteps(health.steps));
  drawCell(SLEEP_CELL, formatSleep(health.sleepMinutes));
  drawCell(HRV_CELL, formatNumber(health.hrvMs), "ms");
  drawCell(RHR_CELL, formatNumber(health.restingHeartRate), "bpm");
}

// The grid lines and the icons
static void drawGrid() {
  M5.Display.drawFastVLine(CELL_WIDTH, 0, M5.Display.height(), DIVIDER_GRAY);
  for (int y : {CELL_HEIGHT, 2 * CELL_HEIGHT}) M5.Display.drawFastHLine(0, y, M5.Display.width(), DIVIDER_GRAY);
  for (const Cell *cell : {&DATE_CELL, &AGE_CELL, &STEPS_CELL, &SLEEP_CELL, &HRV_CELL, &RHR_CELL}) drawIcon(*cell);
}
// The icon's coverage blended onto the black background in its color
static void drawIcon(const Cell &cell) {
  int left = cell.column * CELL_WIDTH + CELL_PADDING;
  int top = cell.row * CELL_HEIGHT + (CELL_HEIGHT - ICON_SIZE) / 2;
  uint8_t red = cell.iconColor >> 16, green = cell.iconColor >> 8, blue = cell.iconColor;
  M5.Display.startWrite();
  for (int y = 0; y < ICON_SIZE; y++) {
    for (int x = 0; x < ICON_SIZE; x++) {
      int alpha = cell.icon[y * ICON_SIZE + x];
      if (alpha) M5.Display.drawPixel(left + x, top + y, M5.Display.color888(red * alpha / 255, green * alpha / 255, blue * alpha / 255));
    }
  }
  M5.Display.endWrite();
}

// The value left-aligned after the icon, and the unit in small gray after it
static void drawCell(const Cell &cell, const String &value, const char *unit) {
  if (isShown(cell.slot, value + unit)) return;

  int left = cell.column * CELL_WIDTH + TEXT_LEFT;
  int centerY = cell.row * CELL_HEIGHT + CELL_HEIGHT / 2;
  // Up to the cell's edge, short of the grid line
  M5.Display.fillRect(left, centerY - TEXT_BOX_HEIGHT / 2, CELL_WIDTH - TEXT_LEFT - 2, TEXT_BOX_HEIGHT, BACKGROUND);
  int baseline = centerY + VALUE_BASELINE;
  M5.Display.setFont(&fonts::FreeSans18pt7b);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TEXT_WHITE, BACKGROUND);
  M5.Display.setTextDatum(baseline_left);
  M5.Display.setTextPadding(0);
  int valueWidth = M5.Display.drawString(value, left, baseline);
  if (!*unit) return;

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(TEXT_GRAY, BACKGROUND);
  M5.Display.drawString(unit, left + valueWidth + UNIT_GAP, baseline);
}

// "16 Oct", "6 Oct"
static String formatDate(time_t when) {
  char text[16];
  struct tm local;
  localtime_r(&when, &local);
  strftime(text, sizeof(text), "%d %b", &local);
  return text[0] == '0' ? text + 1 : text;
}

// Vojto's age in years: "35.21"
static String formatAge(time_t now) {
  return String((now - BIRTH_TIME) / SECONDS_PER_YEAR, 2);
}

// "950", "8.4k"
static String formatSteps(int steps) {
  if (steps == HEALTH_MISSING) return "--";
  if (steps < 1000) return String(steps);
  char text[12];
  snprintf(text, sizeof(text), "%d.%dk", steps / 1000, steps % 1000 / 100);
  return text;
}

// Hours and minutes: "7:24"
static String formatSleep(int minutes) {
  if (minutes == HEALTH_MISSING) return "--";
  char text[12];
  snprintf(text, sizeof(text), "%d:%02d", minutes / 60, minutes % 60);
  return text;
}

static String formatNumber(int number) { return number == HEALTH_MISSING ? "--" : String(number); }

// MARK: Placeholder

static void showPlaceholder(bool isFirstDraw, time_t now) { drawLine(PLACEHOLDER_TEXT, "Slide 2"); }
