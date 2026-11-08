#pragma once
#include <Arduino.h>
#include <time.h>

/*
 * ISO 8601 times, as web APIs send and take them, to and from seconds since
 * 1970. Always UTC. The ESP32's C library has no timegm, so these do the
 * calendar math themselves.
 */

namespace deskkit {

// "2026-10-02T19:30:00Z"
String formatIsoTime(time_t when);

// Reads "2026-10-02T19:30:00+00:00" or "...Z" as UTC; 0 if it isn't a time
time_t parseIsoTime(const char *text);

// Seconds since 1970 for a UTC date and time
time_t utcSeconds(int year, int month, int day, int hour, int minute, int second);

}  // namespace deskkit
