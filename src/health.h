#pragma once

/*
 * Today's health numbers from Moonlite, which syncs them from Garmin. A
 * background task keeps them fresh; the dashboard asks for the latest.
 */

#define HEALTH_MISSING -1  // a number Moonlite doesn't have (yet) for today
#define SLEEP_WEEKS 5      // this week and the four before it

struct Health {
  int steps;
  int sleepMinutes;
  int hrvMs;
  int restingHeartRate;  // beats per minute
  // Average sleep per night, Monday to Sunday: this week so far, then the
  // weeks before it. Nights without sleep data are left out.
  int weeklySleepMinutes[SLEEP_WEEKS];
};

void startFetchingHealth();

// All HEALTH_MISSING until the first fetch succeeds
Health todaysHealth();
