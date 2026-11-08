#include "grid.h"

#define COLUMNS 2
#define CELL_PADDING 12
#define ICON_GAP 8              // between the icon and the value
#define UNIT_GAP 3              // between a value and its unit, like "48 ms"
#define VALUE_BASELINE 12       // below a cell's center, about half the cap height

namespace deskkit {

const TextStyle SMALL_TEXT = {&fonts::FreeSans9pt7b, TEXT_GRAY};
const lgfx::IFont *const VALUE_FONT = &fonts::FreeSans18pt7b;

static int rows = 3;  // set by drawGridLines; on a 320x240 screen 3 rows are 80px, 4 are 60px

static int cellWidth();
static int cellHeight();
static void drawCellIcon(int index, const GridCell &cell);
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
    if (cell.icon) drawCellIcon(i, cell);
    int textLeft = CELL_PADDING + (cell.icon ? cell.icon->size + ICON_GAP : 0);
    drawValue(i, textLeft, cell.value(now), TEXT_WHITE, cell.unit ? cell.unit : "", "");
  }
}

void fillGridCell(int index, uint32_t background) {
  // A cell's line is on its left and top edge, so the fill starts past it
  int column = index % COLUMNS, row = index / COLUMNS;
  int left = column * cellWidth() + (column ? 1 : 0), top = row * cellHeight() + (row ? 1 : 0);
  frame.fillRect(left, top, (column + 1) * cellWidth() - left, (row + 1) * cellHeight() - top, background);
}

void drawGridValue(int index, const String &value, uint32_t color, const String &label) {
  drawValue(index, CELL_PADDING, value, color, "", label);
}

void drawGridLines(int rowCount) {
  rows = rowCount;
  for (int x = cellWidth(); x < COLUMNS * cellWidth(); x += cellWidth()) frame.drawFastVLine(x, 0, frame.height(), DIVIDER_GRAY);
  for (int row = 1; row < rows; row++) frame.drawFastHLine(0, row * cellHeight(), frame.width(), DIVIDER_GRAY);
}

static int cellWidth() { return frame.width() / COLUMNS; }

static int cellHeight() { return frame.height() / rows; }

static void drawCellIcon(int index, const GridCell &cell) {
  int left = index % COLUMNS * cellWidth() + CELL_PADDING;
  int top = index / COLUMNS * cellHeight() + (cellHeight() - cell.icon->size) / 2;
  drawIcon(*cell.icon, left, top, cell.iconColor);
}

// The value at textLeft, the unit right after it and the label at the cell's
// right edge
static void drawValue(int index, int textLeft, const String &value, uint32_t color, const char *unit, const String &label) {
  int cellLeft = index % COLUMNS * cellWidth();
  int left = cellLeft + textLeft;
  int baseline = index / COLUMNS * cellHeight() + cellHeight() / 2 + VALUE_BASELINE;
  int valueWidth = drawText(value, left, baseline, {VALUE_FONT, color}, baseline_left);
  if (*unit) drawText(unit, left + valueWidth + UNIT_GAP, baseline, SMALL_TEXT, baseline_left);
  if (label.length()) drawText(label, cellLeft + cellWidth() - CELL_PADDING, baseline, SMALL_TEXT, baseline_right);
}

}  // namespace deskkit
