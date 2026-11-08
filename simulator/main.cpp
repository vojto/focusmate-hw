#include <M5Unified.h>

/*
 * Starts the firmware in a desktop window (the `native` environment). SDL needs
 * the main thread for the window, so setup() and loop() run in a thread of
 * their own, as M5GFX's PlatformIO_SDL example does. A click on the window is a
 * tap; closing it quits.
 */

void setup();
void loop();

static int runFirmware(bool *isRunning) {
  setup();
  while (*isRunning) loop();
  return 0;
}

int main(int, char **) { return lgfx::Panel_sdl::main(runFirmware); }
