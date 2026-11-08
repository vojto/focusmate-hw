#ifndef ESP_PLATFORM
#include <M5Unified.h>

/*
 * Starts the app in a window on the computer. SDL needs the main thread for
 * the window, so the app's setup() and loop() run in a thread of their own, as
 * in M5GFX's PlatformIO_SDL example. A click on the window is a touch; closing
 * the window quits.
 */

void setup();
void loop();

static int runApp(bool *isRunning) {
  setup();
  while (*isRunning) loop();
  return 0;
}

int main(int, char **) { return lgfx::Panel_sdl::main(runApp); }
#endif
