#pragma once
#include <deskkit/widgets.h>

/*
 * Every widget, for apps to pick from and list in a deskkit Rotation. Each is
 * its own .cpp in this folder; their data comes from data/. To add one, write
 * the file and declare it here.
 */

extern const deskkit::Widget CLOCK_WIDGET;         // the time and date
extern const deskkit::Widget FOCUSMATE_WIDGET;     // urgent around a session: a countdown, then a pie
extern const deskkit::Widget DASHBOARD_WIDGET;     // date, age, steps, sleep, HRV, resting heart rate
extern const deskkit::Widget SLEEP_WIDGET;         // last night and seven weekly averages
extern const deskkit::Widget WEIGHT_WIDGET;        // weekly averages as rows of bars
extern const deskkit::Widget WEIGHT_RANGE_WIDGET;  // weekly lightest to heaviest, as columns
