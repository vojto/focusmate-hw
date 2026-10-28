#include "weight.h"
#include "fetcher.h"
#include "secrets.h"

/*
 * Asks Foodmark for this week's weigh-ins and the averages of the weeks before
 * it every few minutes, and keeps today's weight and the weekly averages in
 * grams. A failed fetch keeps the numbers from the last one that worked.
 */

#define WEIGHTS_URL "https://next.foodmark.app/external/weights"  // ~4KB
#define FETCH_INTERVAL_MS (5 * 60000)
#define FETCH_RETRY_MS 30000

static bool readWeights(JsonDocument &json);
static int grams(JsonVariant kilograms);

static Shared<Weight> weight({WEIGHT_MISSING,
                              {WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING, WEIGHT_MISSING}});
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

  Weight found = {WEIGHT_MISSING, {grams(json["current_week"]["average_weight_kg"])}};
  for (JsonVariant day : json["current_week"]["days"].as<JsonArray>()) {
    if (strcmp(day["date"] | "", today) == 0) found.todayGrams = grams(day["weight_kg"]);
  }
  JsonArray pastWeeks = json["past_weeks"];  // newest first
  for (int week = 1; week < WEIGHT_WEEKS; week++) found.weeklyGrams[week] = grams(pastWeeks[week - 1]["average_weight_kg"]);
  weight.set(found);
  return true;
}

// Null or absent becomes WEIGHT_MISSING
static int grams(JsonVariant kilograms) {
  if (kilograms.isNull()) return WEIGHT_MISSING;
  return (int)lroundf(kilograms.as<float>() * 1000);
}
