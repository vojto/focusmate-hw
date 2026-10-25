#pragma once
#include "display.h"

/*
 * A slide laid out as two columns and three rows (or four), split by gray
 * lines. Each cell has a value and, optionally, a unit after it in small gray
 * and a colored icon on its left. A slide describes its cells in a table, left to
 * right and top to bottom, or draws them one by one with drawGridLines and
 * drawGridValue.
 */

#define GRID_CELLS 6  // in a table; a slide drawing its own cells can have 4 rows, so 8

struct GridCell {
  const uint8_t *icon;  // from icons.h, or nullptr for none
  uint32_t iconColor;
  String (*value)(time_t now);
  const char *unit;
};

// Can be called on every loop: draws the lines and icons on the first draw and
// afterwards only the values that changed
void showGrid(const GridCell (&cells)[GRID_CELLS], bool isFirstDraw, time_t now);

// For a slide that draws its own cells: the lines on the first draw, which
// also set how many rows the cells are laid out in
void drawGridLines(int rowCount = 3);

// A colored value at the cell's left edge, without an icon, and a small gray
// label at its right edge. Redrawn only when it changes.
void drawGridValue(int index, const String &value, uint32_t color, const String &label);
