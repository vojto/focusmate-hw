#include "screens.h"
#include "display.h"

/*
 * Draws the countdown and pie screens into the frame (display.h). The pie is
 * drawn several times too big into its own canvas and shrunk onto the frame,
 * which smooths its edges.
 */

const uint32_t PIE_RED = 0xE24B4A, PIE_USED = 0x2C2C2A, COUNTDOWN_GREEN = 0x5DCAA5;

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
// caption on the left, the session's position in its block on the right. Both
// are drawn after the pie, whose square canvas reaches into the corners.
#define CORNER_LEFT 8
#define CORNER_RIGHT 312
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
static void clearPieCanvas();
static void drawSlices(float usedDegrees);
static void drawTick(float angle, bool isActive, bool isBold);
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
  frame.drawString(time, CENTER_X, CENTER_Y);
  drawText("Session in", CORNER_LEFT, CORNER_BASELINE, CORNER_TEXT, baseline_left);
  drawBlockPosition(block);
  pushFrame();
}

// The used-up part grows clockwise from 12 o'clock, like on a Time Timer. Its
// edge always points at the tick of the minute that is running, which is red.
void showPie(int usedMinutes, int totalMinutes, BlockPosition block) {
  if (!isNewFrame("pie " + String(usedMinutes) + "/" + String(totalMinutes) + " " + blockKey(block))) return;
  float minuteDegrees = 360.0f / totalMinutes;
  clearPieCanvas();
  drawSlices(usedMinutes * minuteDegrees);
  for (int minute = 0; minute < totalMinutes; minute++) {
    bool isActive = minute == usedMinutes;
    drawTick(minute * minuteDegrees, isActive, isActive || minute % BOLD_TICK_MINUTES == 0);
  }
  // Shrinking averages the canvas's pixels, which is what smooths the edges
  pieCanvas.pushRotateZoomWithAA(&frame, CENTER_X, CENTER_Y, 0, 1.0f / PIE_SCALE, 1.0f / PIE_SCALE);
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
