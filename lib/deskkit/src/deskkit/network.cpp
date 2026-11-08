#include "network.h"
#include <time.h>
#include "platform/platform.h"

#define SYNCED_AFTER 1600000000  // September 2020; the clock jumps past it once synced

namespace deskkit {

bool isWifiConnected() { return platform::isWifiConnected(); }

bool isTimeSynced() { return time(nullptr) > SYNCED_AFTER; }

}  // namespace deskkit
