#include "grid.h"
#include "icons.h"

const uint32_t DIVIDER_GRAY = 0x444441;

#define COLUMNS 2
#define CELL_WIDTH 160
#define CELL_PADDING 12
#define ICON_GAP 8              // between the icon and the value
#define UNIT_GAP 3              // between a value and its unit, like "48 ms"
#define VALUE_BASELINE 12       // below a cell's center, about half the cap height

const TextStyle SMALL_TEXT = {&fonts::FreeSans9pt7b, TEXT_GRAY};

static int rows = 3;  // set by drawGridLines; 3 rows are 80px, 4 are 60px

static int cellHeight();
static void drawIcon(int index, const GridCell &cell);
static void drawValue(int index, int textLeft, const String &value, uint32_t color, const char *unit, const String &label);

String gridKey(const GridCell (&cells)[GRID_CELLS], time_t now) {
  String key;
  for (const GridCell &cell : cells) key += cell.value(now) + "|";
  return key;
}

void drawGrid(const GridCell (&cells)[GRID_CELLS], time_t now) {
  drawGridLines();
  for (int i = 0; i < GRID_CELLS; i++) {
    const GridCell &cell = cells[i];
    if (cell.icon) drawIcon(i, cell);
    int textLeft = CELL_PADDING + (cell.icon ? ICON_SIZE + ICON_GAP : 0);
    drawValue(i, textLeft, cell.value(now), TEXT_WHITE, cell.unit ? cell.unit : "", "");
  }
}

void fillGridCell(int index, uint32_t background) {
  // A cell's line is on its left and top edge, so the fill starts past it
  int column = index % COLUMNS, row = index / COLUMNS;
  int left = column * CELL_WIDTH + (column ? 1 : 0), top = row * cellHeight() + (row ? 1 : 0);
  frame.fillRect(left, top, (column + 1) * CELL_WIDTH - left, (row + 1) * cellHeight() - top, background);
}

void drawGridValue(int index, const String &value, uint32_t color, const String &label) {
  drawValue(index, CELL_PADDING, value, color, "", label);
}

void drawGridLines(int rowCount) {
  rows = rowCount;
  for (int x = CELL_WIDTH; x < COLUMNS * CELL_WIDTH; x += CELL_WIDTH) frame.drawFastVLine(x, 0, frame.height(), DIVIDER_GRAY);
  for (int row = 1; row < rows; row++) frame.drawFastHLine(0, row * cellHeight(), frame.width(), DIVIDER_GRAY);
}

static int cellHeight() { return frame.height() / rows; }

// The icon's coverage blended onto the black background in its color
static void drawIcon(int index, const GridCell &cell) {
  int left = index % COLUMNS * CELL_WIDTH + CELL_PADDING;
  int top = index / COLUMNS * cellHeight() + (cellHeight() - ICON_SIZE) / 2;
  uint8_t red = cell.iconColor >> 16, green = cell.iconColor >> 8, blue = cell.iconColor;
  for (int y = 0; y < ICON_SIZE; y++) {
    for (int x = 0; x < ICON_SIZE; x++) {
      int alpha = cell.icon[y * ICON_SIZE + x];
      if (alpha) frame.drawPixel(left + x, top + y, frame.color888(red * alpha / 255, green * alpha / 255, blue * alpha / 255));
    }
  }
}

// The value at textLeft, the unit right after it and the label at the cell's
// right edge
static void drawValue(int index, int textLeft, const String &value, uint32_t color, const char *unit, const String &label) {
  int cellLeft = index % COLUMNS * CELL_WIDTH;
  int left = cellLeft + textLeft;
  int baseline = index / COLUMNS * cellHeight() + cellHeight() / 2 + VALUE_BASELINE;
  int valueWidth = drawText(value, left, baseline, {&fonts::FreeSans18pt7b, color}, baseline_left);
  if (*unit) drawText(unit, left + valueWidth + UNIT_GAP, baseline, SMALL_TEXT, baseline_left);
  if (label.length()) drawText(label, cellLeft + CELL_WIDTH - CELL_PADDING, baseline, SMALL_TEXT, baseline_right);
}
