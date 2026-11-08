#pragma once
#include "data/sessions.h"

/*
 * The Focusmate widget's two looks (focusmate.cpp): a countdown before a
 * session and a pie during it.
 */

// Everything the screen shows, for isNewFrame in deskkit/display.h
String sessionScreenKey(const Session &session, time_t now);

// While the session runs, a pie that starts full and empties clockwise a
// minute at a time, inside a ring with a tick per minute. The time left is in
// the bottom left corner, like 0:15, and a session in a block of several gets
// its position in the bottom right, like 3⁄4. Vojto's age is in the top left,
// like 35.24, and a skull in the top right, as a memento mori.
//
// Before it, the time left as minutes:seconds in the middle, with "Session
// in" in the bottom left corner and the block position in the bottom right.
void drawSessionScreen(const Session &session, time_t now);
