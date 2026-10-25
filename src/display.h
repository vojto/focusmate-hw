#pragma once
#include <M5Unified.h>

/*
 * What the session screens and the slides share: the colors every screen
 * uses, which screen is showing, and text that is redrawn only when it
 * changes, which keeps the screen from flickering.
 */

// Colors are 0xRRGGBB; M5GFX reads uint32_t that way
const uint32_t BACKGROUND = 0x000000, TEXT_WHITE = 0xF1EFE8, TEXT_GRAY = 0xB4B2A9;

#define CENTER_X 160
#define CENTER_Y 120
#define SLOT_COUNT 6

// Slide n is SCREEN_SLIDE + n
enum Screen { SCREEN_NONE, SCREEN_COUNTDOWN, SCREEN_PIE, SCREEN_SLIDE };

// A centered line of text. Lines of one screen each need their own slot,
// which is where the text now on the display is remembered.
struct TextLine {
  int slot;
  int y;
  const lgfx::IFont *font;
  float size;
  uint32_t color;
};

// Clears the display when another screen was showing, and says whether it did
bool enterScreen(int screen);

int shownScreen();

void clearScreen();

// Whether the slot already shows the text; if not, it is remembered as shown
bool isShown(int slot, const String &text);

void drawLine(const TextLine &line, const String &text);
