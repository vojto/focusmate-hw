#include "display.h"

namespace deskkit {

M5Canvas frame(&M5.Display);
static String shownKey;

// A full-screen frame is too big for the normal heap (~150KB on a Core2), so
// it lives in PSRAM
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

// M5GFX takes String only on Arduino, so the text goes in as a C string
int drawText(const String &text, int x, int y, const TextStyle &style, lgfx::textdatum_t datum) {
  frame.setFont(style.font);
  frame.setTextSize(1);
  frame.setTextColor(style.color);
  frame.setTextDatum(datum);
  return frame.drawString(text.c_str(), x, y);
}

void drawIcon(const Icon &icon, int left, int top, uint32_t color) {
  uint8_t red = color >> 16, green = color >> 8, blue = color;
  for (int y = 0; y < icon.size; y++) {
    for (int x = 0; x < icon.size; x++) {
      int alpha = icon.coverage[y * icon.size + x];
      if (alpha) frame.drawPixel(left + x, top + y, frame.color888(red * alpha / 255, green * alpha / 255, blue * alpha / 255));
    }
  }
}

void setBrightness(int brightness) {
  static int shown = -1;
  if (brightness == shown) return;
  shown = brightness;
  M5.Display.setBrightness(brightness);
}

}  // namespace deskkit
