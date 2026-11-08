#include "display.h"
#include "icons.h"
#include "network.h"

#define BIRTH_TIME 681343200     // 5 Aug 1991, midnight in Bratislava
#define SECONDS_PER_YEAR (365.2425 * 86400)

M5Canvas frame(&M5.Display);
static String shownKey;

static bool linearToRgb(float l, float m, float s, uint32_t& rgb);

// ~150KB, too big for the normal heap, so it lives in PSRAM
void startDisplay() {
  frame.setPsram(true);
  frame.createSprite(M5.Display.width(), M5.Display.height());
}

bool isNewFrame(const String &key) {
  if (key == shownKey) return false;
  shownKey = key;
  frame.fillScreen(BACKGROUND);
  return true;
}

void pushFrame() { frame.pushSprite(0, 0); }

const String &shownFrame() { return shownKey; }

int drawText(const String &text, int x, int y, const TextStyle &style, lgfx::textdatum_t datum) {
  frame.setFont(style.font);
  frame.setTextSize(1);
  frame.setTextColor(style.color);
  frame.setTextDatum(datum);
  return frame.drawString(text.c_str(), x, y);
}

void drawIcon(const uint8_t *icon, int left, int top, uint32_t color) {
  uint8_t red = color >> 16, green = color >> 8, blue = color;
  for (int y = 0; y < ICON_SIZE; y++) {
    for (int x = 0; x < ICON_SIZE; x++) {
      int alpha = icon[y * ICON_SIZE + x];
      if (alpha) frame.drawPixel(left + x, top + y, frame.color888(red * alpha / 255, green * alpha / 255, blue * alpha / 255));
    }
  }
}

String ageText(time_t now) {
  if (!isTimeSynced()) return "--";
  return String((now - BIRTH_TIME) / SECONDS_PER_YEAR, 2);
}

// Lowers chroma until the color fits in sRGB, keeping lightness and hue
uint32_t oklchToRgb(float lightness, float chroma, float hue) {
  uint32_t rgb = 0;
  for (; chroma >= 0; chroma -= 0.002f) {
    float a = chroma * cosf(hue * PI / 180), b = chroma * sinf(hue * PI / 180);
    float l = powf(lightness + 0.3963377774f * a + 0.2158037573f * b, 3);
    float m = powf(lightness - 0.1055613458f * a - 0.0638541728f * b, 3);
    float s = powf(lightness - 0.0894841775f * a - 1.2914855480f * b, 3);
    if (linearToRgb(l, m, s, rgb)) break;
  }
  return rgb;
}

// OKLab's LMS to 0xRRGGBB; false when it is outside sRGB
static bool linearToRgb(float l, float m, float s, uint32_t& rgb) {
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
