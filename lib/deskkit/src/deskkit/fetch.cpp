#include "fetch.h"
#include "network.h"
#include "platform/platform.h"

#define OFFLINE_WAIT_MS 1000

namespace deskkit {

static void fetchLoop(void *parameter);
static bool fetchOnce(Fetch &fetch);

void startFetching(Fetch &fetch) { platform::startTask(fetchLoop, &fetch, fetch.name); }

// Waits for the clock too: sources ask for data around today
static void fetchLoop(void *parameter) {
  Fetch &fetch = *(Fetch *)parameter;
  while (true) {
    if (!isWifiConnected() || !isTimeSynced()) {
      delay(OFFLINE_WAIT_MS);
      continue;
    }
    fetch.hasFailed = !fetchOnce(fetch);
    delay(fetch.hasFailed ? fetch.retryMs : fetch.intervalMs);
  }
}

static bool fetchOnce(Fetch &fetch) {
  String body;
  if (!platform::httpGet(fetch.url(), fetch.header, fetch.headerValue, body)) return false;
  JsonDocument json;
  if (deserializeJson(json, body.c_str(), body.length())) return false;
  return fetch.read(json);
}

}  // namespace deskkit
