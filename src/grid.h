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

// The values of a table's cells, as a frame key
String gridKey(const GridCell (&cells)[GRID_CELLS], time_t now);

// The lines, icons and values of a table's cells
void drawGrid(const GridCell (&cells)[GRID_CELLS], time_t now);

// For a slide that draws its own cells: the lines, which also set how many
// rows the cells are laid out in
void drawGridLines(int rowCount = 3);

// Fills a cell, short of its lines, for a slide that sets one apart
void fillGridCell(int index, uint32_t background);

// A colored value at the cell's left edge, without an icon, and a small gray
// label at its right edge
void drawGridValue(int index, const String &value, uint32_t color, const String &label);
