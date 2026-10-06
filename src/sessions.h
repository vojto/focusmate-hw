#pragma once
#include <Arduino.h>

/*
 * The Focusmate sessions booked for the next day, plus the ones of the last
 * half day. A background task keeps the list fresh; the main loop asks for the
 * session that matters right now, where it stands in its block, and for what
 * to tell the user when the list can't be fetched.
 */

struct Session {
  time_t start;
  int32_t seconds;  // 0 means there is no session

  bool isBooked() const { return seconds > 0; }
  bool isRunning(time_t now) const { return now >= start && now < start + seconds; }
  bool isStartingWithin(int32_t limit, time_t now) const { return now < start && start - now <= limit; }
  int32_t minutes() const { return seconds / 60; }
  // Whole minutes since the start
  int32_t usedMinutes(time_t now) const { return (now - start) / 60; }
};

// Where a session stands among the sessions booked back to back with it: 3 of 4
struct BlockPosition {
  int number;
  int total;
};

void startFetchingSessions();

// The session that is running or comes next; not booked if there is none
Session currentSession(time_t now);

// A session with nothing booked right before or after it is 1 of 1
BlockPosition blockPosition(const Session &session);

// "" while all is well, otherwise a short reason the sessions may be out of date
const char *fetchProblem();

// Replaces the whole list, which must be oldest first. The fetch uses it, and so do the debug tools to fake a session.
void setSessions(const Session *list, int count);
