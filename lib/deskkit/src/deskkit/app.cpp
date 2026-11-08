#include "app.h"
#include <M5Unified.h>
#include "console.h"
#include "display.h"
#include "platform/platform.h"

namespace deskkit {

static void turnScreen(bool isPortrait);

void begin(const Config &config) {
  auto cfg = M5.config();
  // Otherwise M5.begin() sets the clock from the clock chip, if there is one
  cfg.internal_rtc = false;
  M5.begin(cfg);
  turnScreen(config.isPortrait);
  startDisplay();
  platform::startNetwork(config.wifiSsid, config.wifiPassword, config.timeZone);
  startConsole();
}

void update() {
  M5.update();
  updateConsole();
}

// On the Core2, M5Unified also turns touches on the strip under the screen
// into BtnA, so a touch screen counts only as touches
bool wasTapped() {
  if (M5.Touch.isEnabled()) return M5.Touch.getDetail().wasPressed();
  return M5.BtnA.wasPressed();
}

// A quarter turn when the panel is the other way; touches turn with it
static void turnScreen(bool isPortrait) {
  bool isPanelPortrait = M5.Display.height() > M5.Display.width();
  if (isPanelPortrait != isPortrait) M5.Display.setRotation((M5.Display.getRotation() + 1) % 4);
}

}  // namespace deskkit
