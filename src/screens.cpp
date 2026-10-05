#include "screens.h"
#include <M5Unified.h>
#include "network.h"

/*
 * Draws the clock, countdown and pie screens on the Core2's 320x240 display.
 * Text is laid out as fixed lines, described in the tables below; a line is
 * redrawn only when its text changes, which keeps the screen from flickering.
 * The pie is drawn several times too big into an off-screen canvas and shrunk
 * onto the display, which smooths its edges.
 */

// Colors are 0xRRGGBB; M5GFX reads uint32_t that way
const uint32_t BACKGROUND = 0x000000, TEXT_WHITE = 0xF1EFE8, TEXT_GRAY = 0xB4B2A9;
const uint32_t PIE_RED = 0xE24B4A, PIE_USED = 0x2C2C2A;
const uint32_t COUNTDOWN_GREEN = 0x5DCAA5, NOTICE_ORANGE = 0xEF9F27;

#define CENTER_X 160
#define CENTER_Y 120
#define SLOT_COUNT 4

// The pie and its ticks, in screen pixels and degrees
#define PIE_RADIUS 106
#define TICK_INNER 111          // the ticks sit between these two radii
#define TICK_OUTER 117
#define TICK_DEGREES 0.6f       // how wide a tick is, about 1px
#define BOLD_TICK_DEGREES 1.6f  // about 3px
#define BOLD_TICK_MINUTES 5
#define PIE_SCALE 2             // how many times too big the pie is drawn; a redraw takes ~0.6s at 2, ~1s at 3
#define PIE_CANVAS (240 * PIE_SCALE)

// A centered line of text. Lines of one screen each need their own slot,
// which is where the text now on the display is remembered.
struct TextLine {
  int slot;
  int y;
  const lgfx::IFont *font;
  float size;
  uint32_t color;
};

const TextLine CLOCK_NOTICE = {0, 18, &fonts::FreeSans9pt7b, 1, NOTICE_ORANGE};
const TextLine CLOCK_TIME = {1, 88, &fonts::Font7, 1.4f, TEXT_WHITE};
const TextLine CLOCK_DATE = {2, 150, &fonts::FreeSans12pt7b, 1, TEXT_GRAY};
const TextLine CLOCK_NEXT = {3, 195, &fonts::FreeSans12pt7b, 1, TEXT_WHITE};

const TextLine COUNTDOWN_CAPTION = {0, 52, &fonts::FreeSans12pt7b, 1, TEXT_GRAY};
const TextLine COUNTDOWN_TIME = {1, 132, &fonts::Font7, 2, COUNTDOWN_GREEN};

enum Screen { SCREEN_NONE, SCREEN_CLOCK, SCREEN_COUNTDOWN, SCREEN_PIE };

static bool enterScreen(Screen screen);
static void drawLine(const TextLine &line, const String &text);
static String formatLocalTime(time_t when, const char *format);
static void clearPieCanvas();
static void drawSlices(float usedDegrees);
static void drawTick(float angle, bool isActive, bool isBold);
static void fillArc(int inner, int outer, float from, float to, uint32_t color);

static String shownTexts[SLOT_COUNT];
static M5Canvas pieCanvas;

// MARK: Screens

void showClock(time_t now, const Session &next, const char *notice) {
  enterScreen(SCREEN_CLOCK);
  drawLine(CLOCK_NOTICE, notice);
  if (!isTimeSynced()) return drawLine(CLOCK_DATE, "Syncing time...");

  drawLine(CLOCK_TIME, formatLocalTime(now, "%H:%M:%S"));
  drawLine(CLOCK_DATE, formatLocalTime(now, "%a %d %b"));
  drawLine(CLOCK_NEXT, next.isBooked() ? formatLocalTime(next.start, "Next session %H:%M") : "");
}

void showCountdown(int32_t seconds) {
  enterScreen(SCREEN_COUNTDOWN);
  char text[8];
  snprintf(text, sizeof(text), "%d:%02d", (int)(seconds / 60), (int)(seconds % 60));
  drawLine(COUNTDOWN_CAPTION, "Session in");
  drawLine(COUNTDOWN_TIME, text);
}

// The used-up part grows clockwise from 12 o'clock, like on a Time Timer. Its
// edge always points at the tick of the minute that is running, which is red.
void showPie(int usedMinutes, int totalMinutes) {
  static int shownUsed, shownTotal;
  bool isFirstDraw = enterScreen(SCREEN_PIE);
  if (!isFirstDraw && usedMinutes == shownUsed && totalMinutes == shownTotal) return;
  shownUsed = usedMinutes;
  shownTotal = totalMinutes;

  float minuteDegrees = 360.0f / totalMinutes;
  clearPieCanvas();
  drawSlices(usedMinutes * minuteDegrees);
  for (int minute = 0; minute < totalMinutes; minute++) {
    bool isActive = minute == usedMinutes;
    drawTick(minute * minuteDegrees, isActive, isActive || minute % BOLD_TICK_MINUTES == 0);
  }
  // Shrinking averages the canvas's pixels, which is what smooths the edges
  pieCanvas.pushRotateZoomWithAA(&M5.Display, CENTER_X, CENTER_Y, 0, 1.0f / PIE_SCALE, 1.0f / PIE_SCALE);
}

void setScreenBrightness(int brightness) {
  static int shownBrightness = -1;
  if (brightness == shownBrightness) return;
  shownBrightness = brightness;
  M5.Display.setBrightness(brightness);
}

// MARK: Drawing helpers

// Clears the display when another screen was showing, and says whether it did
static bool enterScreen(Screen screen) {
  static Screen shownScreen = SCREEN_NONE;
  if (screen == shownScreen) return false;
  shownScreen = screen;
  M5.Display.fillScreen(BACKGROUND);
  for (String &text : shownTexts) text = "";
  return true;
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
