#include "screens.h"
#include "display.h"
#include "icons.h"
#include <math.h>

/*
 * Draws the countdown and pie screens into the frame (display.h). The pie is
 * drawn several times too big into its own canvas and shrunk onto the frame,
 * which smooths its edges.
 */

const uint32_t PIE_USED = 0x2C2C2A, COUNTDOWN_GREEN = 0x5DCAA5;

// The pie's color runs through these OKLCH colors (lightness, chroma, hue),
// evenly spaced over the session: green, yellow, orange, then red (0xE24B4A)
// in the last minute. tools/sketch/session-colors.html shows them.
const float PIE_COLORS[][3] = {{0.72f, 0.17f, 150}, {0.84f, 0.17f, 95}, {0.73f, 0.17f, 55}, {0.6234f, 0.1877f, 24.72f}};
const int PIE_COLOR_COUNT = sizeof(PIE_COLORS) / sizeof(PIE_COLORS[0]);

// The pie and its ticks, in screen pixels and degrees
#define PIE_RADIUS 106
#define TICK_INNER 111          // the ticks sit between these two radii
#define TICK_OUTER 117
#define TICK_DEGREES 0.6f       // how wide a tick is, about 1px
#define BOLD_TICK_DEGREES 1.6f  // about 3px
#define BOLD_TICK_MINUTES 5
#define PIE_SCALE 2             // how many times too big the pie is drawn; a redraw takes ~0.6s at 2, ~1s at 3
#define PIE_CANVAS (240 * PIE_SCALE)

// The corners of the pie and countdown screens: Vojto's age top left and a
// skull top right (pie only), the time left or a caption bottom left, the session's position in its
// block bottom right. All are drawn after the pie, whose square canvas reaches
// into the corners.
#define CORNER_LEFT 8
#define CORNER_RIGHT 312
#define CORNER_TOP_BASELINE 26
#define SKULL_TOP 4             // centers the skull on the age's digits
#define CORNER_BASELINE 231
#define SLASH_WIDTH 9           // a fraction slash leans further than a normal one
#define SLASH_RISE 19           // how far above the baseline it reaches
#define SLASH_DROP 3            // and how far below
#define SLASH_GAP 2             // between the slash and the digits
#define SLASH_THICKNESS 0.7f    // half the line's width

const TextStyle CORNER_TEXT = {&fonts::FreeSans12pt7b, TEXT_GRAY};

static String minutesAndSeconds(int32_t seconds);
static String blockKey(BlockPosition block);
static void drawTimeLeft(int minutes);
static void drawBlockPosition(BlockPosition block);
static uint32_t pieColor(int usedMinutes, int totalMinutes);
static void clearPieCanvas();
static void drawSlices(float usedDegrees, uint32_t color);
static void drawTick(float angle, bool isBold, uint32_t color);
static void fillArc(int inner, int outer, float from, float to, uint32_t color);

static M5Canvas pieCanvas;

// MARK: Screens

void showCountdown(int32_t seconds, BlockPosition block) {
  String time = minutesAndSeconds(seconds);
  if (!isNewFrame("countdown " + time + " " + blockKey(block))) return;
  frame.setFont(&fonts::Font7);
  frame.setTextSize(2);
  frame.setTextColor(COUNTDOWN_GREEN);
  frame.setTextDatum(middle_center);
  frame.drawString(time.c_str(), CENTER_X, CENTER_Y);
  drawText("Session in", CORNER_LEFT, CORNER_BASELINE, CORNER_TEXT, baseline_left);
  drawBlockPosition(block);
  pushFrame();
}

// The used-up part grows clockwise from 12 o'clock, like on a Time Timer. Its
// edge always points at the tick of the minute that is running, which has the
// pie's color. The color goes from green to red over the session.
void showPie(int usedMinutes, int totalMinutes, BlockPosition block, time_t now) {
  String age = ageText(now);
  if (!isNewFrame("pie " + String(usedMinutes) + "/" + String(totalMinutes) + " " + blockKey(block) + " " + age)) return;
  float minuteDegrees = 360.0f / totalMinutes;
  uint32_t color = pieColor(usedMinutes, totalMinutes);
  clearPieCanvas();
  drawSlices(usedMinutes * minuteDegrees, color);
  for (int minute = 0; minute < totalMinutes; minute++) {
    bool isActive = minute == usedMinutes;
    drawTick(minute * minuteDegrees, isActive || minute % BOLD_TICK_MINUTES == 0, isActive ? color : TEXT_GRAY);
  }
  // Shrinking averages the canvas's pixels, which is what smooths the edges
  pieCanvas.pushRotateZoomWithAA(&frame, CENTER_X, CENTER_Y, 0, 1.0f / PIE_SCALE, 1.0f / PIE_SCALE);
  drawText(age, CORNER_LEFT, CORNER_TOP_BASELINE, CORNER_TEXT, baseline_left);
  drawIcon(ICON_SKULL, CORNER_RIGHT - ICON_SIZE, SKULL_TOP, TEXT_GRAY);
  drawTimeLeft(totalMinutes - usedMinutes);
  drawBlockPosition(block);
  pushFrame();
}

// "7:42"
static String minutesAndSeconds(int32_t seconds) {
  char text[8];
  snprintf(text, sizeof(text), "%d:%02d", (int)(seconds / 60), (int)(seconds % 60));
  return text;
}

static String blockKey(BlockPosition block) { return String(block.number) + "/" + String(block.total); }

// MARK: Corner labels

// Hours and minutes, bottom left: "0:15", "1:15"
static void drawTimeLeft(int minutes) {
  char text[8];
  snprintf(text, sizeof(text), "%d:%02d", minutes / 60, minutes % 60);
  drawText(text, CORNER_LEFT, CORNER_BASELINE, CORNER_TEXT, baseline_left);
}

// "3⁄4" in the bottom right corner, drawn from the right: the total, the
// slash, the number. A session on its own gets nothing.
static void drawBlockPosition(BlockPosition block) {
  if (block.total < 2) return;
  // The fonts have no fraction slash, so it is a line
  int slashRight = CORNER_RIGHT - drawText(String(block.total), CORNER_RIGHT, CORNER_BASELINE, CORNER_TEXT, baseline_right) - SLASH_GAP;
  int slashLeft = slashRight - SLASH_WIDTH;
  frame.drawWideLine(slashLeft, CORNER_BASELINE + SLASH_DROP, slashRight, CORNER_BASELINE - SLASH_RISE, SLASH_THICKNESS, TEXT_GRAY);
  drawText(String(block.number), slashLeft - SLASH_GAP, CORNER_BASELINE, CORNER_TEXT, baseline_right);
}

// MARK: Pie color

// The first minute gets the first color, the last minute the last
static uint32_t pieColor(int usedMinutes, int totalMinutes) {
  float at = totalMinutes > 1 ? (float)usedMinutes / (totalMinutes - 1) * (PIE_COLOR_COUNT - 1) : 0;
  int from = min((int)at, PIE_COLOR_COUNT - 2);
  float t = at - from;
  float lch[3];
  for (int i = 0; i < 3; i++) lch[i] = PIE_COLORS[from][i] + (PIE_COLORS[from + 1][i] - PIE_COLORS[from][i]) * t;
  return oklchToRgb(lch[0], lch[1], lch[2]);
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

// The used part from 12 o'clock to usedDegrees, the rest in the session's color
static void drawSlices(float usedDegrees, uint32_t color) {
  fillArc(0, PIE_RADIUS, usedDegrees, 360, color);
  // Equal angles would fill the whole circle
  if (usedDegrees > 0) fillArc(0, PIE_RADIUS, 0, usedDegrees, PIE_USED);
}

static void drawTick(float angle, bool isBold, uint32_t color) {
  float halfWidth = (isBold ? BOLD_TICK_DEGREES : TICK_DEGREES) / 2;
  fillArc(TICK_INNER, TICK_OUTER, angle - halfWidth, angle + halfWidth, color);
}

// Fills part of a ring on the pie canvas. Radii are in screen pixels, angles in
// degrees clockwise from 12 o'clock.
static void fillArc(int inner, int outer, float from, float to, uint32_t color) {
  // The canvas counts clockwise from 3 o'clock, so its 270 is the top
  pieCanvas.fillArc(PIE_CANVAS / 2, PIE_CANVAS / 2, inner * PIE_SCALE, outer * PIE_SCALE, 270 + from, 270 + to, color);
}
