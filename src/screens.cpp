#include "screens.h"
#include <M5Unified.h>
#include "network.h"

/*
 * Draws the clock, countdown and pie screens on the Core2's 320x240 display.
 * Text is laid out as fixed lines, described in the tables below; a line is
 * redrawn only when its text changes, which keeps the screen from flickering.
 */

// Colors are 0xRRGGBB; M5GFX reads uint32_t that way
const uint32_t BACKGROUND = 0x000000, TEXT_WHITE = 0xF1EFE8, TEXT_GRAY = 0xB4B2A9;
const uint32_t PIE_RED = 0xE24B4A, PIE_USED = 0x2C2C2A;
const uint32_t COUNTDOWN_GREEN = 0x5DCAA5, NOTICE_ORANGE = 0xEF9F27;

#define CENTER_X 160
#define CENTER_Y 120
#define PIE_RADIUS 104
#define PIE_STEP_DEGREES 0.25f  // the pie is redrawn once it has moved this far
#define SLOT_COUNT 4

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
static void fillSlice(float from, float to, uint32_t color);
static String formatLocalTime(time_t when, const char *format);

static String shownTexts[SLOT_COUNT];

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

// The used-up part grows clockwise from 12 o'clock, like on a Time Timer
void showPie(float usedFraction) {
  static float shownAngle = 0;
  bool isFirstDraw = enterScreen(SCREEN_PIE);
  float angle = 360 * usedFraction;
  if (!isFirstDraw && angle - shownAngle < PIE_STEP_DEGREES) return;

  if (isFirstDraw) fillSlice(angle, 360, PIE_RED);
  fillSlice(0, angle, PIE_USED);
  shownAngle = angle;
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

// Fills the pie between two angles, in degrees clockwise from 12 o'clock
static void fillSlice(float from, float to, uint32_t color) {
  // fillArc counts clockwise from 3 o'clock, so its 270 is the top
  M5.Display.fillArc(CENTER_X, CENTER_Y, 0, PIE_RADIUS, 270 + from, 270 + to, color);
}

static String formatLocalTime(time_t when, const char *format) {
  char text[32];
  struct tm local;
  localtime_r(&when, &local);
  strftime(text, sizeof(text), format, &local);
  return text;
}
