#pragma once
#include "sessions.h"

/*
 * The three screens the main loop chooses from. Each show function can be
 * called on every loop: it clears the display when coming from another screen
 * and afterwards redraws only what changed.
 */

// Time, date and when the next session starts, plus a notice on top if there is one
void showClock(time_t now, const Session &next, const char *notice);

// "Session in" and the time left as minutes:seconds
void showCountdown(int32_t seconds);

// A pie that starts full and empties clockwise as usedFraction goes from 0 to 1
void showPie(float usedFraction);

// 0-255
void setScreenBrightness(int brightness);
