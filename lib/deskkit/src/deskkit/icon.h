#pragma once
#include <stdint.h>

/*
 * A small square icon as coverage, 0-255 per pixel, rows top to bottom.
 * tools/icons.py makes a header of them from SVGs; drawIcon (display.h) blends
 * one onto the black screen in any color, which keeps its edges smooth.
 */

namespace deskkit {

struct Icon {
  int size;  // width and height in pixels
  const uint8_t *coverage;
};

}  // namespace deskkit
