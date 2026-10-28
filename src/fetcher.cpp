#include "fetcher.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "network.h"

#define TASK_STACK_BYTES 12288  // enough for the TLS handshake

static void fetchTask(void *parameter);
static bool fetchOnce(Fetch &fetch);
static bool request(const Fetch &fetch, String &json);

void startFetching(Fetch &fetch) { xTaskCreate(fetchTask, fetch.name, TASK_STACK_BYTES, &fetch, 1, nullptr); }

// Waits for the clock too: the sources ask for today's data
static void fetchTask(void *parameter) {
  Fetch &fetch = *(Fetch *)parameter;
  while (true) {
    if (!isWifiConnected() || !isTimeSynced()) {
      delay(1000);
      continue;
    }
    fetch.hasFailed = !fetchOnce(fetch);
    delay(fetch.hasFailed ? fetch.retryMs : fetch.intervalMs);
  }
}

static bool fetchOnce(Fetch &fetch) {
  String json;
  if (!request(fetch, json)) return false;
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  return fetch.read(doc);
}

// In its own function so the TLS client is freed between fetches; kept alive,
// it leaks
static bool request(const Fetch &fetch, String &json) {
  WiFiClientSecure client;
  client.setInsecure();  // no certificate check
  HTTPClient http;
  http.begin(client, fetch.url());
  http.addHeader(fetch.header, fetch.headerValue);
  if (http.GET() != HTTP_CODE_OK) return false;
  json = http.getString();
  return true;
}
