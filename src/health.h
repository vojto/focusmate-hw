#pragma once

/*
 * Today's health numbers from Moonlite, which syncs them from Garmin. A
 * background task keeps them fresh; the dashboard asks for the latest.
 */

#define HEALTH_MISSING -1  // a number Moonlite doesn't have (yet) for today

struct Health {
  int steps;
  int sleepMinutes;
  int hrvMs;
  int restingHeartRate;  // beats per minute
};

void startFetchingHealth();

// All HEALTH_MISSING until the first fetch succeeds
Health todaysHealth();
