#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include "icon.h"
#include "theme.h"

/*
 * The screen, drawn a whole frame at a time. Every screen draws into an
 * off-screen frame the size of the display and pushes it in one go, so the
 * display never shows a half-drawn frame. A screen draws only when what it
 * shows changed, which it says with a key: a string of everything on it.
 */

namespace deskkit {

struct TextStyle {
  const lgfx::IFont *font;
  uint32_t color;
};

// Where the screens draw, the size of the display
extern M5Canvas frame;

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

// The icon's coverage blended onto the black background in its color
void drawIcon(const Icon &icon, int left, int top, uint32_t color);

// 0-255; sets it only when it changes
void setBrightness(int brightness);

// MARK: For begin() in app.h

// Makes the frame; after M5.begin()
void startDisplay();

}  // namespace deskkit
