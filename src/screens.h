#pragma once
#include "health.h"
#include "sessions.h"

/*
 * The three screens the main loop chooses from. Each show function can be
 * called on every loop: it clears the display when coming from another screen
 * and afterwards redraws only what changed.
 */

// Between sessions: a grid of the date, the time and today's steps, sleep, HRV
// and resting heart rate, each with an icon. A notice, when there is one, is
// an orange bar across the top.
void showDashboard(time_t now, const Health &health, const char *notice);

// The time left as minutes:seconds in the middle, with "Session in" in the
// bottom left corner and the coming session's position in its block in the
// bottom right, as on the pie
void showCountdown(int32_t seconds, BlockPosition block);

// A pie that starts full and empties clockwise a minute at a time, inside a
// ring with a tick per minute. The time left is in the bottom left corner, like
// 0:15, and a session in a block of several gets its position in the bottom
// right, like 3⁄4.
void showPie(int usedMinutes, int totalMinutes, BlockPosition block);
