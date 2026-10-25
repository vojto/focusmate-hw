#include "display.h"

static int screenShown = SCREEN_NONE;
static String shownTexts[SLOT_COUNT];

bool enterScreen(int screen) {
  if (screen == screenShown) return false;
  screenShown = screen;
  clearScreen();
  return true;
}

int shownScreen() { return screenShown; }

void clearScreen() {
  M5.Display.fillScreen(BACKGROUND);
  for (String &text : shownTexts) text = "";
}

bool isShown(int slot, const String &text) {
  if (shownTexts[slot] == text) return true;
  shownTexts[slot] = text;
  return false;
}

void drawLine(const TextLine &line, const String &text) {
  if (isShown(line.slot, text)) return;
  M5.Display.setFont(line.font);
  M5.Display.setTextSize(line.size);
  M5.Display.setTextColor(line.color, BACKGROUND);
  M5.Display.setTextDatum(middle_center);
  // Clears the full width, so a shorter text leaves nothing behind from a longer one
  M5.Display.setTextPadding(M5.Display.width());
  M5.Display.drawString(text, CENTER_X, line.y);
}
