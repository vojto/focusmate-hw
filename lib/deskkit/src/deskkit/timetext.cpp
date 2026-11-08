#include "timetext.h"
#include "network.h"

namespace deskkit {

static String unitsAndSixtieths(int32_t units);

String hoursAndMinutes(int minutes) { return minutes < 0 ? "--" : unitsAndSixtieths(minutes); }

String minutesAndSeconds(int32_t seconds) { return unitsAndSixtieths(seconds); }

String dayAndMonth(time_t now) {
  if (!isTimeSynced()) return "--";
  char text[16];
  struct tm local;
  localtime_r(&now, &local);
  strftime(text, sizeof(text), "%d %b", &local);
  return text[0] == '0' ? text + 1 : text;
}

String weekLabel(time_t now, int weeksAgo) {
  if (!isTimeSynced()) return "";
  time_t then = now - weeksAgo * 7 * 86400;
  struct tm local;
  localtime_r(&then, &local);
  char number[4];
  strftime(number, sizeof(number), "%V", &local);
  return "W" + String(atoi(number));
}

// "7:05" from 425
static String unitsAndSixtieths(int32_t units) {
  char text[12];
  snprintf(text, sizeof(text), "%d:%02d", (int)(units / 60), (int)(units % 60));
  return text;
}

}  // namespace deskkit
