#pragma once
#include <Arduino.h>
#include <time.h>

/*
 * Times and dates as the screens show them, short and in local time.
 */

namespace deskkit {

// "7:24"; "--" when negative, which apps use for a missing number
String hoursAndMinutes(int minutes);

// "7:42"
String minutesAndSeconds(int32_t seconds);

// "16 Oct", "6 Oct"; "--" until the clock is synced
String dayAndMonth(time_t now);

// The ISO week number of now, or of weeksAgo weeks before it: "W43". Weeks
// start on Monday. "" until the clock is synced.
String weekLabel(time_t now, int weeksAgo);

}  // namespace deskkit
