#include "display.h"
#include "icons.h"
#include "network.h"

#define BIRTH_TIME 681343200     // 5 Aug 1991, midnight in Bratislava
#define SECONDS_PER_YEAR (365.2425 * 86400)

M5Canvas frame(&M5.Display);
static String shownKey;

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
  return frame.drawString(text, x, y);
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
