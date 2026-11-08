#include "app.h"
#include <M5Unified.h>
#include "console.h"
#include "display.h"
#include "platform/platform.h"

namespace deskkit {

void begin(const Config &config) {
  auto cfg = M5.config();
  // Otherwise M5.begin() sets the clock from the clock chip, if there is one
  cfg.internal_rtc = false;
  M5.begin(cfg);
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

}  // namespace deskkit
