#include "weight.h"
#include "fetcher.h"
#include "secrets.h"

/*
 * Asks Foodmark for the weigh-ins of this week and the weeks before it every
 * few minutes, and keeps today's weight and each week's average, lightest and
 * heaviest in grams. A failed fetch keeps the numbers from the last one that worked.
 */

#define WEIGHTS_URL "https://next.foodmark.app/external/weights"  // ~20KB
#define FETCH_INTERVAL_MS (5 * 60000)
#define FETCH_RETRY_MS 30000

static bool readWeights(JsonDocument &json);
static void readWeek(JsonVariant json, int week, Weight &found);
static int grams(JsonVariant kilograms);
static Weight missingWeight();

static Shared<Weight> weight(missingWeight());
static Fetch fetch = {"weight", [] { return String(WEIGHTS_URL); }, "Authorization", "Bearer " FOODMARK_API_TOKEN,
                      FETCH_INTERVAL_MS, FETCH_RETRY_MS, readWeights};

void startFetchingWeight() { startFetching(fetch); }

Weight latestWeight() { return weight.get(); }

// Today is found by the device's date
static bool readWeights(JsonDocument &json) {
  char today[11];
  time_t now = time(nullptr);
  struct tm local;
  localtime_r(&now, &local);
  strftime(today, sizeof(today), "%Y-%m-%d", &local);

  Weight found = missingWeight();
  readWeek(json["current_week"], 0, found);
  for (JsonVariant day : json["current_week"]["days"].as<JsonArray>()) {
    if (strcmp(day["date"] | "", today) == 0) found.todayGrams = grams(day["weight_kg"]);
  }
  JsonArray pastWeeks = json["past_weeks"];  // newest first
  for (int week = 1; week < WEIGHT_WEEKS; week++) readWeek(pastWeeks[week - 1], week, found);
  found.earlierGrams = grams(pastWeeks[WEIGHT_WEEKS - 1]["average_weight_kg"]);
  weight.set(found);
  return true;
}

static void readWeek(JsonVariant json, int week, Weight &found) {
  found.weeklyGrams[week] = grams(json["average_weight_kg"]);
  for (JsonVariant day : json["days"].as<JsonArray>()) {
    int dayGrams = grams(day["weight_kg"]);
    if (dayGrams == WEIGHT_MISSING) continue;
    if (found.lightestGrams[week] == WEIGHT_MISSING || dayGrams < found.lightestGrams[week]) found.lightestGrams[week] = dayGrams;
    found.heaviestGrams[week] = max(found.heaviestGrams[week], dayGrams);
  }
}

// Null or absent becomes WEIGHT_MISSING
static int grams(JsonVariant kilograms) {
  if (kilograms.isNull()) return WEIGHT_MISSING;
  return (int)lroundf(kilograms.as<float>() * 1000);
}

static Weight missingWeight() {
  Weight missing;
  missing.todayGrams = missing.earlierGrams = WEIGHT_MISSING;
  for (int week = 0; week < WEIGHT_WEEKS; week++) {
    missing.weeklyGrams[week] = missing.lightestGrams[week] = missing.heaviestGrams[week] = WEIGHT_MISSING;
  }
  return missing;
}
