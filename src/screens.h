#pragma once
#include "sessions.h"

/*
 * The screens the main loop and the dashboards draw. Each show function can be
 * called on every loop: it clears the display when coming from another screen
 * and afterwards redraws only what changed.
 */

// A dashboard: time, date and when the next session starts, plus a notice on top if there is one
void showClock(time_t now, const Session &next, const char *notice);

// "Session in" and the time left as minutes:seconds
void showCountdown(int32_t seconds);

// A pie that starts full and empties clockwise a minute at a time, inside a
// ring with a tick per minute. The time left is in the bottom left corner, like
// 0:15, and a session in a block of several gets its position in the bottom
// right, like 3⁄4.
void showPie(int usedMinutes, int totalMinutes, BlockPosition block);
