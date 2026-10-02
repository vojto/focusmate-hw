/*
 * Development tools, compiled only into the `debug` environment and driven by
 * tools/screenshot.py over serial. One-letter commands put a fake session on
 * screen or send the screen's pixels back, so the drawing can be checked from
 * the computer. A real fetch replaces the fake session within a minute.
 */

#define DEBUG_BAUD 460800

void debugFakeSession(int32_t secondsUntilStart, int32_t seconds) {
  portENTER_CRITICAL(&sessionsLock);
  sessions[0] = {time(nullptr) + secondsUntilStart, seconds};
  sessionCount = 1;
  portEXIT_CRITICAL(&sessionsLock);
}

// Sends "SHOT <width> <height>" and then the screen as rows of R, G, B bytes
void debugSendScreenshot() {
  static uint8_t row[320 * 3];
  Serial.printf("SHOT %d %d\n", (int)M5.Display.width(), (int)M5.Display.height());
  for (int y = 0; y < M5.Display.height(); y++) {
    M5.Display.readRectRGB(0, y, M5.Display.width(), 1, row);
    Serial.write(row, M5.Display.width() * 3);
  }
}

void debugLoop() {
  if (!Serial.available()) return;
  char command = Serial.read();
  if (!isTimeSynced()) {
    Serial.println("WAIT");
    return;
  }
  if (command == 'p') debugFakeSession(-18 * 60, 50 * 60);       // pie, 32 of 50 minutes left
  else if (command == 'q') debugFakeSession(-74 * 60, 75 * 60);  // pie, last minute of 75
  else if (command == 'c') debugFakeSession(7 * 60 + 42, 50 * 60);    // countdown
  else if (command == 'w') debugFakeSession(WARNING_S + 3, 25 * 60);  // chimes in 3s, starts in 63s
  else if (command == 'n') debugFakeSession(2 * 3600 + 23 * 60, 50 * 60);  // clock with a next session
  else if (command == 'e') sessionCount = 0;                     // clock, nothing booked
  else if (command == 's') return debugSendScreenshot();
  else return;
  Serial.printf("OK %c\n", command);
}
