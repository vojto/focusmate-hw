#include "isotime.h"

namespace deskkit {

String formatIsoTime(time_t when) {
  char text[24];
  struct tm utc;
  strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", gmtime_r(&when, &utc));
  return text;
}

time_t parseIsoTime(const char *text) {
  int year, month, day, hour, minute, second;
  if (sscanf(text, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) return 0;
  return utcSeconds(year, month, day, hour, minute, second);
}

// Counts days from 1 March of year 0, so the leap day comes last in a year
time_t utcSeconds(int year, int month, int day, int hour, int minute, int second) {
  year -= month <= 2;
  int era = year / 400;
  int yearOfEra = year - era * 400;
  int dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
  int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
  return (era * 146097L + dayOfEra - 719468L) * 86400L + hour * 3600L + minute * 60 + second;
}

}  // namespace deskkit
