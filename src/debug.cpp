#ifdef DEBUG_TOOLS
#include "debug.h"
#include <M5Unified.h>
#include "network.h"
#include "sessions.h"

/*
 * Lets tools/screenshot.py drive the device over serial, so the drawing can be
 * checked from the computer. One-letter commands put a fake session on screen
 * or send the screen's pixels back. A real fetch replaces the fake session
 * within a minute.
 */

#define BAUD 460800

static void fakeSession(int32_t secondsUntilStart, int32_t seconds);
static void sendScreenshot();

void debugSetup() { Serial.begin(BAUD); }

void debugLoop() {
  if (!Serial.available()) return;
  char command = Serial.read();
  if (!isTimeSynced()) {
    Serial.println("WAIT");
    return;
  }
  if (command == 'p') fakeSession(-18 * 60, 50 * 60);                // pie, 32 of 50 minutes left
  else if (command == 'q') fakeSession(-74 * 60, 75 * 60);           // pie, last minute of 75
  else if (command == 'c') fakeSession(7 * 60 + 42, 50 * 60);        // countdown
  else if (command == 'w') fakeSession(63, 25 * 60);                 // warning chime in 3s, start in 63s
  else if (command == 'n') fakeSession(2 * 3600 + 23 * 60, 50 * 60); // clock with a next session
  else if (command == 'e') setSessions(nullptr, 0);                  // clock, nothing booked
  else if (command == 's') return sendScreenshot();
  else return;
  Serial.printf("OK %c\n", command);
}

static void fakeSession(int32_t secondsUntilStart, int32_t seconds) {
  Session session = {time(nullptr) + secondsUntilStart, seconds};
  setSessions(&session, 1);
}

// Sends "SHOT <width> <height>" and then the screen as rows of R, G, B bytes
static void sendScreenshot() {
  static uint8_t row[320 * 3];
  Serial.printf("SHOT %d %d\n", (int)M5.Display.width(), (int)M5.Display.height());
  for (int y = 0; y < M5.Display.height(); y++) {
    M5.Display.readRectRGB(0, y, M5.Display.width(), 1, row);
    Serial.write(row, M5.Display.width() * 3);
  }
}
#endif
