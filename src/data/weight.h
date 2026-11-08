#pragma once

/*
 * Vojto's weight from Foodmark: this morning's weigh-in and the average of
 * the recent weeks. A background task keeps them fresh; the weight slide asks
 * for the latest.
 */

#define WEIGHT_MISSING -1  // no weigh-in (yet)
#define WEIGHT_WEEKS 8     // this week and the seven before it

struct Weight {
  int todayGrams;
  // Average weight, Monday to Sunday: this week so far, then the weeks
  // before it
  int weeklyGrams[WEIGHT_WEEKS];
  // The week's lightest and heaviest weigh-ins, in the same order
  int lightestGrams[WEIGHT_WEEKS];
  int heaviestGrams[WEIGHT_WEEKS];
  // The average of the week before the oldest, to tell how that one changed
  int earlierGrams;
};

void startFetchingWeight();

// All WEIGHT_MISSING until the first fetch succeeds
Weight latestWeight();
