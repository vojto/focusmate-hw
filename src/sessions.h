#pragma once
#include <Arduino.h>

/*
 * The Focusmate sessions booked for the next day. A background task keeps the
 * list fresh; the main loop asks for the session that matters right now and
 * for what to tell the user when the list can't be fetched.
 */

struct Session {
  time_t start;
  int32_t seconds;  // 0 means there is no session

  bool isBooked() const { return seconds > 0; }
  bool isRunning(time_t now) const { return now >= start && now < start + seconds; }
  bool isStartingWithin(int32_t limit, time_t now) const { return now < start && start - now <= limit; }
  // 0 at the start, 1 at the end
  float usedFraction(time_t now) const { return (float)(now - start) / seconds; }
};

void startFetchingSessions();

// The session that is running or comes next; not booked if there is none
Session currentSession(time_t now);

// "" while all is well, otherwise a short reason the sessions may be out of date
const char *fetchProblem();

// Replaces the whole list. The fetch uses it, and so do the debug tools to fake a session.
void setSessions(const Session *list, int count);
