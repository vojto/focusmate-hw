#include "screens.h"
#include <M5Unified.h>
#include "icons.h"
#include "network.h"

/*
 * Draws the dashboard, countdown and pie screens on the Core2's 320x240
 * display. Text is laid out as fixed lines and cells, described in the tables
 * below; each is redrawn only when its text changes, which keeps the screen
 * from flickering. The dashboard's icons come from icons.h, made by
 * tools/icons.py from the sketch in tools/sketch/dashboard.html.
 * The pie is drawn several times too big into an off-screen canvas and shrunk
 * onto the display, which smooths its edges.
 */

// Colors are 0xRRGGBB; M5GFX reads uint32_t that way
const uint32_t BACKGROUND = 0x000000, TEXT_WHITE = 0xF1EFE8, TEXT_GRAY = 0xB4B2A9;
const uint32_t PIE_RED = 0xE24B4A, PIE_USED = 0x2C2C2A;
const uint32_t COUNTDOWN_GREEN = 0x5DCAA5, NOTICE_ORANGE = 0xEF9F27, DIVIDER_GRAY = 0x444441;
const uint32_t STEPS_GREEN = 0x5DCAA5, SLEEP_BLUE = 0x85B7EB, HRV_PINK = 0xED93B1, RHR_RED = 0xE24B4A;

#define CENTER_X 160
#define CENTER_Y 120
#define SLOT_COUNT 6

// The pie and its ticks, in screen pixels and degrees
#define PIE_RADIUS 106
#define TICK_INNER 111          // the ticks sit between these two radii
#define TICK_OUTER 117
#define TICK_DEGREES 0.6f       // how wide a tick is, about 1px
#define BOLD_TICK_DEGREES 1.6f  // about 3px
#define BOLD_TICK_MINUTES 5
#define PIE_SCALE 2             // how many times too big the pie is drawn; a redraw takes ~0.6s at 2, ~1s at 3
#define PIE_CANVAS (240 * PIE_SCALE)

// The bottom corners of the pie and countdown screens: the time left or a
// caption on the left, the session's position in its block on the right. The
// box is what gets cleared in each corner; it stays clear of the ticks.
#define CORNER_LEFT 8
#define CORNER_RIGHT 312
#define CORNER_BASELINE 231
#define CORNER_BOX_WIDTH 68
#define CORNER_BOX_HEIGHT 28
#define SLASH_WIDTH 9           // a fraction slash leans further than a normal one
#define SLASH_RISE 19           // how far above the baseline it reaches
#define SLASH_DROP 3            // and how far below
#define SLASH_GAP 2             // between the slash and the digits
#define SLASH_THICKNESS 0.7f    // half the line's width

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
#define NOTICE_HEIGHT 20        // a bar across the top, above the first row's text

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

// A centered line of text. Lines of one screen each need their own slot,
// which is where the text now on the display is remembered.
struct TextLine {
  int slot;
  int y;
  const lgfx::IFont *font;
  float size;
  uint32_t color;
};

const TextLine COUNTDOWN_TIME = {0, CENTER_Y, &fonts::Font7, 2, COUNTDOWN_GREEN};

enum Screen { SCREEN_NONE, SCREEN_DASHBOARD, SCREEN_COUNTDOWN, SCREEN_PIE };

static bool enterScreen(Screen screen);
static void clearScreen();
static void drawDashboardFrame(const char *notice);
static void drawIcon(const Cell &cell);
static void drawCell(const Cell &cell, const String &value, const char *unit = "");
static String formatDate(time_t when);
static String formatAge(time_t now);
static String formatSteps(int steps);
static String formatSleep(int minutes);
static String formatNumber(int number);
static void drawLine(const TextLine &line, const String &text);
static String formatLocalTime(time_t when, const char *format);
static void drawTimeLeft(int minutes);
static void drawCornerLabel(const char *text);
static void drawBlockPosition(BlockPosition block);
static void startCornerText(int boxLeft, lgfx::textdatum_t datum);
static void clearPieCanvas();
static void drawSlices(float usedDegrees);
static void drawTick(float angle, bool isActive, bool isBold);
static void fillArc(int inner, int outer, float from, float to, uint32_t color);

static String shownTexts[SLOT_COUNT];
static M5Canvas pieCanvas;

// MARK: Screens

void showDashboard(time_t now, const Health &health, const char *notice) {
  static String shownNotice;
  bool isFirstDraw = enterScreen(SCREEN_DASHBOARD);
  // The notice bar crosses the grid, so a change redraws everything
  if (shownNotice != notice) {
    shownNotice = notice;
    clearScreen();
    isFirstDraw = true;
  }
  if (isFirstDraw) drawDashboardFrame(notice);

  bool isSynced = isTimeSynced();
  drawCell(DATE_CELL, isSynced ? formatDate(now) : "--");
  drawCell(AGE_CELL, isSynced ? formatAge(now) : "--");
  drawCell(STEPS_CELL, formatSteps(health.steps));
  drawCell(SLEEP_CELL, formatSleep(health.sleepMinutes));
  drawCell(HRV_CELL, formatNumber(health.hrvMs), "ms");
  drawCell(RHR_CELL, formatNumber(health.restingHeartRate), "bpm");
}

void showCountdown(int32_t seconds, BlockPosition block) {
  static BlockPosition shownBlock;
  bool isFirstDraw = enterScreen(SCREEN_COUNTDOWN);
  char text[8];
  snprintf(text, sizeof(text), "%d:%02d", (int)(seconds / 60), (int)(seconds % 60));
  drawLine(COUNTDOWN_TIME, text);

  if (isFirstDraw) drawCornerLabel("Session in");
  if (!isFirstDraw && block == shownBlock) return;
  shownBlock = block;
  drawBlockPosition(block);
}

// The used-up part grows clockwise from 12 o'clock, like on a Time Timer. Its
// edge always points at the tick of the minute that is running, which is red.
void showPie(int usedMinutes, int totalMinutes, BlockPosition block) {
  static int shownUsed, shownTotal;
  static BlockPosition shownBlock;
  bool isFirstDraw = enterScreen(SCREEN_PIE);
  if (!isFirstDraw && usedMinutes == shownUsed && totalMinutes == shownTotal && block == shownBlock) return;
  shownUsed = usedMinutes;
  shownTotal = totalMinutes;
  shownBlock = block;

  float minuteDegrees = 360.0f / totalMinutes;
  clearPieCanvas();
  drawSlices(usedMinutes * minuteDegrees);
  for (int minute = 0; minute < totalMinutes; minute++) {
    bool isActive = minute == usedMinutes;
    drawTick(minute * minuteDegrees, isActive, isActive || minute % BOLD_TICK_MINUTES == 0);
  }
  // Shrinking averages the canvas's pixels, which is what smooths the edges
  pieCanvas.pushRotateZoomWithAA(&M5.Display, CENTER_X, CENTER_Y, 0, 1.0f / PIE_SCALE, 1.0f / PIE_SCALE);
  // After the pie, whose square canvas reaches into the corners
  drawTimeLeft(totalMinutes - usedMinutes);
  drawBlockPosition(block);
}

// MARK: Drawing helpers

// Clears the display when another screen was showing, and says whether it did
static bool enterScreen(Screen screen) {
  static Screen shownScreen = SCREEN_NONE;
  if (screen == shownScreen) return false;
  shownScreen = screen;
  clearScreen();
  return true;
}

static void clearScreen() {
  M5.Display.fillScreen(BACKGROUND);
  for (String &text : shownTexts) text = "";
}

static void drawLine(const TextLine &line, const String &text) {
  if (shownTexts[line.slot] == text) return;
  shownTexts[line.slot] = text;
  M5.Display.setFont(line.font);
  M5.Display.setTextSize(line.size);
  M5.Display.setTextColor(line.color, BACKGROUND);
  M5.Display.setTextDatum(middle_center);
  // Clears the full width, so a shorter text leaves nothing behind from a longer one
  M5.Display.setTextPadding(M5.Display.width());
  M5.Display.drawString(text, CENTER_X, line.y);
}

static String formatLocalTime(time_t when, const char *format) {
  char text[32];
  struct tm local;
  localtime_r(&when, &local);
  strftime(text, sizeof(text), format, &local);
  return text;
}

// MARK: Dashboard

// The grid lines, the icons and the notice bar, which change only with the notice
static void drawDashboardFrame(const char *notice) {
  M5.Display.drawFastVLine(CELL_WIDTH, 0, M5.Display.height(), DIVIDER_GRAY);
  for (int y : {CELL_HEIGHT, 2 * CELL_HEIGHT}) M5.Display.drawFastHLine(0, y, M5.Display.width(), DIVIDER_GRAY);
  for (const Cell *cell : {&DATE_CELL, &AGE_CELL, &STEPS_CELL, &SLEEP_CELL, &HRV_CELL, &RHR_CELL}) drawIcon(*cell);
  if (!*notice) return;

  M5.Display.fillRect(0, 0, M5.Display.width(), NOTICE_HEIGHT, NOTICE_ORANGE);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(BACKGROUND, NOTICE_ORANGE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextPadding(0);
  M5.Display.drawString(notice, CENTER_X, NOTICE_HEIGHT / 2);
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
  String text = value + unit;
  if (shownTexts[cell.slot] == text) return;
  shownTexts[cell.slot] = text;

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
  String text = formatLocalTime(when, "%d %b");
  return text.startsWith("0") ? text.substring(1) : text;
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

// MARK: Corner labels

// Hours and minutes: "0:15", "1:15"
static void drawTimeLeft(int minutes) {
  char text[8];
  snprintf(text, sizeof(text), "%d:%02d", minutes / 60, minutes % 60);
  drawCornerLabel(text);
}

// In the bottom left corner. Only its box is cleared, so a longer text must not change.
static void drawCornerLabel(const char *text) {
  startCornerText(CORNER_LEFT, baseline_left);
  M5.Display.drawString(text, CORNER_LEFT, CORNER_BASELINE);
}

// "3⁄4", right-aligned and drawn from the right: the total, the slash, the number.
// A session on its own gets nothing.
static void drawBlockPosition(BlockPosition block) {
  startCornerText(CORNER_RIGHT - CORNER_BOX_WIDTH, baseline_right);
  if (block.total < 2) return;

  // The fonts have no fraction slash, so it is a line
  int slashRight = CORNER_RIGHT - M5.Display.drawString(String(block.total), CORNER_RIGHT, CORNER_BASELINE) - SLASH_GAP;
  int slashLeft = slashRight - SLASH_WIDTH;
  M5.Display.drawWideLine(slashLeft, CORNER_BASELINE + SLASH_DROP, slashRight, CORNER_BASELINE - SLASH_RISE, SLASH_THICKNESS, TEXT_GRAY);
  M5.Display.drawString(String(block.number), slashLeft - SLASH_GAP, CORNER_BASELINE);
}

// Clears a corner's box and sets up its text
static void startCornerText(int boxLeft, lgfx::textdatum_t datum) {
  M5.Display.fillRect(boxLeft, M5.Display.height() - CORNER_BOX_HEIGHT, CORNER_BOX_WIDTH, CORNER_BOX_HEIGHT, BACKGROUND);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TEXT_GRAY, BACKGROUND);
  M5.Display.setTextDatum(datum);
  M5.Display.setTextPadding(0);
}

// MARK: Pie canvas

// The canvas is too big for the normal heap, so it lives in PSRAM
static void clearPieCanvas() {
  if (!pieCanvas.getBuffer()) {
    pieCanvas.setPsram(true);
    pieCanvas.createSprite(PIE_CANVAS, PIE_CANVAS);
  }
  pieCanvas.fillScreen(BACKGROUND);
}

// The used part from 12 o'clock to usedDegrees, the rest red
static void drawSlices(float usedDegrees) {
  fillArc(0, PIE_RADIUS, usedDegrees, 360, PIE_RED);
  // Equal angles would fill the whole circle
  if (usedDegrees > 0) fillArc(0, PIE_RADIUS, 0, usedDegrees, PIE_USED);
}

static void drawTick(float angle, bool isActive, bool isBold) {
  float halfWidth = (isBold ? BOLD_TICK_DEGREES : TICK_DEGREES) / 2;
  fillArc(TICK_INNER, TICK_OUTER, angle - halfWidth, angle + halfWidth, isActive ? PIE_RED : TEXT_GRAY);
}

// Fills part of a ring on the pie canvas. Radii are in screen pixels, angles in
// degrees clockwise from 12 o'clock.
static void fillArc(int inner, int outer, float from, float to, uint32_t color) {
  // The canvas counts clockwise from 3 o'clock, so its 270 is the top
  pieCanvas.fillArc(PIE_CANVAS / 2, PIE_CANVAS / 2, inner * PIE_SCALE, outer * PIE_SCALE, 270 + from, 270 + to, color);
}
