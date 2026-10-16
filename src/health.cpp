#include "health.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "network.h"
#include "secrets.h"

/*
 * Asks Moonlite's device API for today's steps, sleep, HRV and resting heart
 * rate every few minutes, in a background task like the session fetch. A
 * failed fetch keeps the numbers from the last one that worked.
 */

#define STATS_URL "https://moonlite.im/api/v1/stats/today"
#define FETCH_INTERVAL_MS (5 * 60000)  // Moonlite syncs Garmin every 10 minutes, so a new number shows within 15
#define FETCH_RETRY_MS 30000
#define TASK_STACK_BYTES 12288         // enough for the TLS handshake

static void fetchTask(void *);
static bool fetchHealth();
static bool requestStats(String &json);

// Written by the fetch task, read by the main loop
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static Health health = {HEALTH_MISSING, HEALTH_MISSING, HEALTH_MISSING, HEALTH_MISSING};

void startFetchingHealth() { xTaskCreate(fetchTask, "health", TASK_STACK_BYTES, nullptr, 1, nullptr); }

Health todaysHealth() {
  portENTER_CRITICAL(&lock);
  Health latest = health;
  portEXIT_CRITICAL(&lock);
  return latest;
}

static void fetchTask(void *) {
  while (true) {
    if (!isWifiConnected()) {
      delay(1000);
      continue;
    }
    delay(fetchHealth() ? FETCH_INTERVAL_MS : FETCH_RETRY_MS);
  }
}

// Null values in the JSON become HEALTH_MISSING
static bool fetchHealth() {
  String json;
  if (!requestStats(json)) return false;
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;

  int sleepSeconds = doc["sleep_seconds"] | HEALTH_MISSING;
  float hrv = doc["hrv_ms"] | (float)HEALTH_MISSING;
  Health found = {
    doc["steps"] | HEALTH_MISSING,
    sleepSeconds == HEALTH_MISSING ? HEALTH_MISSING : sleepSeconds / 60,
    hrv < 0 ? HEALTH_MISSING : (int)lroundf(hrv),
    doc["resting_heart_rate_bpm"] | HEALTH_MISSING,
  };
  portENTER_CRITICAL(&lock);
  health = found;
  portEXIT_CRITICAL(&lock);
  return true;
}

// The request lives in its own function so its objects are freed between fetches
static bool requestStats(String &json) {
  WiFiClientSecure client;
  client.setInsecure();  // no certificate check
  HTTPClient http;
  http.begin(client, STATS_URL);
  http.addHeader("Authorization", "Bearer " MOONLITE_API_TOKEN);
  if (http.GET() != HTTP_CODE_OK) return false;
  json = http.getString();
  return true;
}
