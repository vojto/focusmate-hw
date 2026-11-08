#include "console.h"
#include "display.h"
#include "network.h"
#include "platform/platform.h"

#define MAX_COMMANDS 26
#define MAX_SCREEN_WIDTH 320

namespace deskkit {

struct Command {
  char letter;
  void (*run)();
};

static void reply(const char *text);
static void sendScreenshot();

static Command commands[MAX_COMMANDS];
static int commandCount = 0;

void addCommand(char letter, void (*run)()) {
  if (commandCount < MAX_COMMANDS) commands[commandCount++] = {letter, run};
}

void startConsole() {
#ifdef DEBUG_TOOLS
  platform::startSerial();
#endif
}

void updateConsole() {
#ifdef DEBUG_TOOLS
  int letter = platform::readSerial();
  if (letter < 0) return;
  // Commands fake states relative to now, which means nothing before the sync
  if (!isTimeSynced()) return reply("WAIT\n");
  if (letter == 's') return sendScreenshot();
  for (int i = 0; i < commandCount; i++) {
    if (commands[i].letter != letter) continue;
    commands[i].run();
    char text[8];
    snprintf(text, sizeof(text), "OK %c\n", letter);
    return reply(text);
  }
#endif
}

static void reply(const char *text) { platform::writeSerial((const uint8_t *)text, strlen(text)); }

// Read back from the panel, so it is what is really on the screen
static void sendScreenshot() {
  static uint8_t row[MAX_SCREEN_WIDTH * 3];
  int width = min((int)M5.Display.width(), MAX_SCREEN_WIDTH), height = M5.Display.height();
  char header[24];
  snprintf(header, sizeof(header), "SHOT %d %d\n", width, height);
  reply(header);
  for (int y = 0; y < height; y++) {
    M5.Display.readRectRGB(0, y, width, 1, row);
    platform::writeSerial(row, width * 3);
  }
}

}  // namespace deskkit
