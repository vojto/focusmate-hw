#include "display.h"

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
