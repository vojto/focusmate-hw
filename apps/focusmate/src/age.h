#pragma once
#include <Arduino.h>
#include <time.h>

/*
 * Vojto's age in years, shown on the pie screen and the dashboard as a
 * memento mori.
 */

// "35.21"; "--" until the clock is synced
String ageText(time_t now);
