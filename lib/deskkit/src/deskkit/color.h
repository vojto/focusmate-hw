#pragma once
#include <stdint.h>

/*
 * Color math for the screens: blending, gradients through a few stops, and
 * OKLCH, a color space where equal steps look equally far apart, so a
 * gradient made in it stays evenly bright. Colors are 0xRRGGBB (see theme.h).
 */

namespace deskkit {

struct Oklch {
  float lightness;  // 0-1
  float chroma;
  float hue;        // degrees

  // 0xRRGGBB, with chroma lowered until it fits in sRGB
  uint32_t rgb() const;
};

// amount 0 is from, 1 is to; mixed in RGB
uint32_t blend(uint32_t from, uint32_t to, float amount);

// A color along stops spread evenly from amount 0 (the first) to 1 (the last),
// mixed in RGB
uint32_t gradient(const uint32_t stops[], int count, float amount);

// The same, mixed in OKLCH
Oklch gradient(const Oklch stops[], int count, float amount);

}  // namespace deskkit
