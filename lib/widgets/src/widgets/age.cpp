#include "age.h"
#include <deskkit/network.h>

#define BIRTH_TIME 681343200     // 5 Aug 1991, midnight in Bratislava
#define SECONDS_PER_YEAR (365.2425 * 86400)

String ageText(time_t now) {
  if (!deskkit::isTimeSynced()) return "--";
  return String((now - BIRTH_TIME) / SECONDS_PER_YEAR, 2);
}
