#pragma once
#include "sessions.h"

/*
 * What the screen shows between sessions. There are several dashboards, the
 * clock among them, and they take turns: tapping the screen moves to the next
 * one, and each also gives way on its own after a while.
 */

// Call on every loop while no session is running or about to start
void showDashboard(time_t now, const Session &next);
