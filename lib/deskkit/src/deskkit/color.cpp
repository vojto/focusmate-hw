#include "color.h"
#include <Arduino.h>

namespace deskkit {

static int stopBefore(int count, float position);
static bool lmsToRgb(float l, float m, float s, uint32_t &rgb);

// MARK: Mixing

uint32_t blend(uint32_t from, uint32_t to, float amount) {
  uint32_t color = 0;
  for (int shift : {16, 8, 0}) {
    int a = from >> shift & 0xFF, b = to >> shift & 0xFF;
    color |= (uint32_t)lroundf(a + (b - a) * amount) << shift;
  }
  return color;
}

uint32_t gradient(const uint32_t stops[], int count, float amount) {
  float position = amount * (count - 1);
  int from = stopBefore(count, position);
  return blend(stops[from], stops[from + 1], position - from);
}

Oklch gradient(const Oklch stops[], int count, float amount) {
  float position = amount * (count - 1);
  int from = stopBefore(count, position);
  float t = position - from;
  const Oklch &a = stops[from], &b = stops[from + 1];
  return {a.lightness + (b.lightness - a.lightness) * t, a.chroma + (b.chroma - a.chroma) * t, a.hue + (b.hue - a.hue) * t};
}

// The stop a position falls after; the last one counts as the end of the one before
static int stopBefore(int count, float position) { return min((int)position, count - 2); }

// MARK: OKLCH

// Lowers chroma until the color fits in sRGB, keeping lightness and hue
uint32_t Oklch::rgb() const {
  uint32_t rgb = 0;
  for (float c = chroma; c >= 0; c -= 0.002f) {
    float a = c * cosf(hue * PI / 180), b = c * sinf(hue * PI / 180);
    float l = powf(lightness + 0.3963377774f * a + 0.2158037573f * b, 3);
    float m = powf(lightness - 0.1055613458f * a - 0.0638541728f * b, 3);
    float s = powf(lightness - 0.0894841775f * a - 1.2914855480f * b, 3);
    if (lmsToRgb(l, m, s, rgb)) break;
  }
  return rgb;
}

// OKLab's LMS to 0xRRGGBB; false when it is outside sRGB
static bool lmsToRgb(float l, float m, float s, uint32_t &rgb) {
  float linear[3] = {
      4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
      -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
      -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
  };
  rgb = 0;
  for (float v : linear) {
    if (v < -1e-4f || v > 1 + 1e-4f) return false;
    v = constrain(v, 0.0f, 1.0f);
    float srgb = v <= 0.0031308f ? 12.92f * v : 1.055f * powf(v, 1 / 2.4f) - 0.055f;
    rgb = rgb << 8 | (uint32_t)lroundf(255 * srgb);
  }
  return true;
}

}  // namespace deskkit
