#include "dashboards.h"
#include <M5Unified.h>
#include "screens.h"

/*
 * The list of dashboards and whose turn it is. A dashboard is a function that
 * draws itself, and like the screens in screens.cpp it is called on every loop
 * and redraws only what changed. Dashboards draw straight onto the display,
 * not through an enlarged canvas like the pie, so a tap shows the next one
 * without a wait.
 */

#define TURN_S 60  // how long a dashboard stays before the next one takes over

typedef void (*Dashboard)(time_t now, const Session &next);

static void showClockDashboard(time_t now, const Session &next) { showClock(now, next, fetchProblem()); }

static const Dashboard DASHBOARDS[] = {showClockDashboard};
static const int DASHBOARD_COUNT = sizeof(DASHBOARDS) / sizeof(DASHBOARDS[0]);

void showDashboard(time_t now, const Session &next) {
  static int shownIndex = 0;
  static time_t turnStart = now;
  if (M5.Touch.getDetail().wasPressed() || now - turnStart >= TURN_S) {
    shownIndex = (shownIndex + 1) % DASHBOARD_COUNT;
    turnStart = now;
  }
  DASHBOARDS[shownIndex](now, next);
}
