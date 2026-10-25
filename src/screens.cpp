#include "screens.h"
#include "display.h"

/*
 * Draws the countdown and pie screens on the Core2's 320x240 display. The pie
 * is drawn several times too big into an off-screen canvas and shrunk onto
 * the display, which smooths its edges.
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

const TextLine COUNTDOWN_TIME = {0, CENTER_Y, &fonts::Font7, 2, COUNTDOWN_GREEN};

static void drawTimeLeft(int minutes);
static void drawCornerLabel(const char *text);
static void drawBlockPosition(BlockPosition block);
static void startCornerText(int boxLeft, lgfx::textdatum_t datum);
static void clearPieCanvas();
static void drawSlices(float usedDegrees);
static void drawTick(float angle, bool isActive, bool isBold);
static void fillArc(int inner, int outer, float from, float to, uint32_t color);

static M5Canvas pieCanvas;

// MARK: Screens

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
