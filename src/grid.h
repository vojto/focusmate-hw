#pragma once
#include "display.h"

/*
 * A slide laid out as two columns and three rows, split by gray lines. Each
 * cell has a value and, optionally, a unit after it in small gray and a
 * colored icon on its left. A slide describes its cells in a table, left to
 * right and top to bottom.
 */

#define GRID_CELLS 6

struct GridCell {
  const uint8_t *icon;  // from icons.h, or nullptr for none
  uint32_t iconColor;
  String (*value)(time_t now);
  const char *unit;
};

// Can be called on every loop: draws the lines and icons on the first draw and
// afterwards only the values that changed
void showGrid(const GridCell (&cells)[GRID_CELLS], bool isFirstDraw, time_t now);
