#include "weight.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "network.h"
#include "secrets.h"

/*
 * Asks Foodmark for this week's weigh-ins and the averages of the weeks before
 * it every few minutes, in a background task like the health fetch, and keeps
 * today's weight and the weekly averages in grams. A failed fetch keeps the
 * numbers from the last one that worked.
 */

#define WEIGHTS_URL "https://next.foodmark.app/external/weights"  // ~4KB
#define FETCH_INTERVAL_MS (5 * 60000)
#define FETCH_RETRY_MS 30000
#define TASK_STACK_BYTES 12288  // enough for the TLS handshake

static void fetchTask(void *);
static bool fetchWeight();
static bool requestWeights(String &json);
static int grams(JsonVariant kilograms);

// Written by the fetch task, read by the main loop
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static Weight weight = {WEIGHT_MISSING,
                        {WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING}};

void startFetchingWeight() { xTaskCreate(fetchTask, "weight", TASK_STACK_BYTES, nullptr, 1, nullptr); }

Weight latestWeight() {
  portENTER_CRITICAL(&lock);
  Weight latest = weight;
  portEXIT_CRITICAL(&lock);
  return latest;
}

static void fetchTask(void *) {
  while (true) {
    if (!isWifiConnected() || !isTimeSynced()) {
      delay(1000);
      continue;
    }
    delay(fetchWeight() ? FETCH_INTERVAL_MS : FETCH_RETRY_MS);
  }
}

// Today is found by the device's date, so it needs the clock synced
static bool fetchWeight() {
  String json;
  if (!requestWeights(json)) return false;
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;

  char today[11];
  time_t now = time(nullptr);
  struct tm local;
  localtime_r(&now, &local);
  strftime(today, sizeof(today), "%Y-%m-%d", &local);

  Weight found = {WEIGHT_MISSING, {grams(doc["current_week"]["average_weight_kg"])}};
  for (JsonVariant day : doc["current_week"]["days"].as<JsonArray>()) {
    if (strcmp(day["date"] | "", today) == 0) found.todayGrams = grams(day["weight_kg"]);
  }
  JsonArray pastWeeks = doc["past_weeks"];  // newest first
  for (int week = 1; week < WEIGHT_WEEKS; week++) found.weeklyGrams[week] = grams(pastWeeks[week - 1]["average_weight_kg"]);

  portENTER_CRITICAL(&lock);
  weight = found;
  portEXIT_CRITICAL(&lock);
  return true;
}

// The request lives in its own function so its objects are freed between fetches
static bool requestWeights(String &json) {
  WiFiClientSecure client;
  client.setInsecure();  // no certificate check
  HTTPClient http;
  http.begin(client, WEIGHTS_URL);
  http.addHeader("Authorization", "Bearer " FOODMARK_API_TOKEN);
  if (http.GET() != HTTP_CODE_OK) return false;
  json = http.getString();
  return true;
}

// Null or absent becomes WEIGHT_MISSING
static int grams(JsonVariant kilograms) {
  if (kilograms.isNull()) return WEIGHT_MISSING;
  return (int)lroundf(kilograms.as<float>() * 1000);
}
