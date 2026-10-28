#pragma once
#include <M5Unified.h>

/*
 * What the session screens and the slides share. Every screen is drawn whole
 * into an off-screen frame and pushed to the display in one go, so the display
 * never shows a half-drawn frame. A screen draws only when what it shows
 * changed, which it says with a key: a string of everything on it.
 */

// Colors are 0xRRGGBB; M5GFX reads uint32_t that way
const uint32_t BACKGROUND = 0x000000, TEXT_WHITE = 0xF1EFE8, TEXT_GRAY = 0xB4B2A9;

#define CENTER_X 160
#define CENTER_Y 120

struct TextStyle {
  const lgfx::IFont *font;
  uint32_t color;
};

// Where the screens draw, the size of the display
extern M5Canvas frame;

// Makes the frame; after M5.begin()
void startDisplay();

// Whether the frame must be drawn: true, with the frame cleared, when the key
// differs from the one on the display. Push it with pushFrame() when done.
bool isNewFrame(const String &key);

void pushFrame();

// The key of the frame on the display
const String &shownFrame();

// For a key made of a struct's numbers
template <class T>
String keyOf(const T &value) {
  char hex[2 * sizeof(T) + 1];
  const uint8_t *bytes = (const uint8_t *)&value;
  for (size_t i = 0; i < sizeof(T); i++) snprintf(hex + 2 * i, 3, "%02x", bytes[i]);
  return hex;
}

// Returns the text's width
int drawText(const String &text, int x, int y, const TextStyle &style, lgfx::textdatum_t datum);
