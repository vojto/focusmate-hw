#include "health.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "network.h"
#include "secrets.h"

/*
 * Asks Moonlite's device API for twelve weeks of daily steps, sleep, HRV and
 * resting heart rate every few minutes, in a background task like the session
 * fetch, and keeps today's numbers and the recent weeks' average sleep. A
 * failed fetch keeps the numbers from the last one that worked.
 */

#define STATS_URL "https://moonlite.im/api/v1/stats"  // ~15KB, the days oldest first and ending today
#define FETCH_INTERVAL_MS (5 * 60000)  // Moonlite syncs Garmin every 10 minutes, so a new number shows within 15
#define FETCH_RETRY_MS 30000
#define TASK_STACK_BYTES 12288         // enough for the TLS handshake

static void fetchTask(void *);
static bool fetchHealth();
static bool requestStats(String &json);
static int lastValue(JsonArray days, const char *key);
static void averageSleep(JsonArray days, const char *today, int *weeklyMinutes);

// Written by the fetch task, read by the main loop
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static Health health = {HEALTH_MISSING, HEALTH_MISSING, HEALTH_MISSING, HEALTH_MISSING,
                        {HEALTH_MISSING, HEALTH_MISSING, HEALTH_MISSING, HEALTH_MISSING, HEALTH_MISSING}};

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

  int sleepSeconds = lastValue(doc["sleep"], "sleep_seconds");
  Health found = {
    lastValue(doc["steps"], "steps"),
    sleepSeconds == HEALTH_MISSING ? HEALTH_MISSING : sleepSeconds / 60,
    lastValue(doc["hrv"], "hrv_ms"),
    lastValue(doc["resting_heart_rate"], "resting_heart_rate_bpm"),
  };
  averageSleep(doc["sleep"], doc["today"], found.weeklySleepMinutes);
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

// Today's value, rounded
static int lastValue(JsonArray days, const char *key) {
  float value = days[days.size() - 1][key] | (float)HEALTH_MISSING;
  return value < 0 ? HEALTH_MISSING : (int)lroundf(value);
}

// Weeks start on Monday. today is "2026-10-25", the date of the last day.
static void averageSleep(JsonArray days, const char *today, int *weeklyMinutes) {
  struct tm date = {};
  sscanf(today, "%d-%d-%d", &date.tm_year, &date.tm_mon, &date.tm_mday);
  date.tm_year -= 1900;
  date.tm_mon -= 1;
  date.tm_hour = 12;
  mktime(&date);  // fills in the weekday
  int daysSinceMonday = (date.tm_wday + 6) % 7;

  int last = days.size() - 1;
  for (int week = 0; week < SLEEP_WEEKS; week++) {
    int monday = last - daysSinceMonday - week * 7;
    int seconds = 0, nights = 0;
    for (int day = max(monday, 0); day <= min(monday + 6, last); day++) {
      int night = days[day]["sleep_seconds"] | HEALTH_MISSING;
      if (night == HEALTH_MISSING) continue;
      seconds += night;
      nights++;
    }
    weeklyMinutes[week] = nights ? seconds / nights / 60 : HEALTH_MISSING;
  }
}
