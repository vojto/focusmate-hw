#include "grid.h"
#include "icons.h"

const uint32_t DIVIDER_GRAY = 0x444441;

#define COLUMNS 2
#define ROWS 3
#define CELL_WIDTH 160
#define CELL_HEIGHT 80
#define CELL_PADDING 12
#define ICON_GAP 8              // between the icon and the value
#define TEXT_BOX_HEIGHT 40      // what gets cleared around a value
#define UNIT_GAP 3              // between a value and its unit, like "48 ms"
#define VALUE_BASELINE 12       // below a cell's center, about half the cap height

static void drawLines();
static void drawIcon(int index, const GridCell &cell);
static void drawValue(int index, const GridCell &cell, const String &value);

void showGrid(const GridCell (&cells)[GRID_CELLS], bool isFirstDraw, time_t now) {
  if (isFirstDraw) drawLines();
  for (int i = 0; i < GRID_CELLS; i++) {
    if (isFirstDraw && cells[i].icon) drawIcon(i, cells[i]);
    drawValue(i, cells[i], cells[i].value(now));
  }
}

static void drawLines() {
  for (int x = CELL_WIDTH; x < COLUMNS * CELL_WIDTH; x += CELL_WIDTH) M5.Display.drawFastVLine(x, 0, M5.Display.height(), DIVIDER_GRAY);
  for (int y = CELL_HEIGHT; y < ROWS * CELL_HEIGHT; y += CELL_HEIGHT) M5.Display.drawFastHLine(0, y, M5.Display.width(), DIVIDER_GRAY);
}

// The icon's coverage blended onto the black background in its color
static void drawIcon(int index, const GridCell &cell) {
  int left = index % COLUMNS * CELL_WIDTH + CELL_PADDING;
  int top = index / COLUMNS * CELL_HEIGHT + (CELL_HEIGHT - ICON_SIZE) / 2;
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

// Left-aligned after the icon, or at the padding without one. The cell's index is its slot.
static void drawValue(int index, const GridCell &cell, const String &value) {
  const char *unit = cell.unit ? cell.unit : "";
  if (isShown(index, value + unit)) return;

  int textLeft = CELL_PADDING + (cell.icon ? ICON_SIZE + ICON_GAP : 0);
  int left = index % COLUMNS * CELL_WIDTH + textLeft;
  int centerY = index / COLUMNS * CELL_HEIGHT + CELL_HEIGHT / 2;
  // Up to the cell's edge, short of the grid line
  M5.Display.fillRect(left, centerY - TEXT_BOX_HEIGHT / 2, CELL_WIDTH - textLeft - 2, TEXT_BOX_HEIGHT, BACKGROUND);
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
