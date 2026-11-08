#include <M5Unified.h>
#include <deskkit/sound.h>
#include "data/sessions.h"
#include "focusmate_screens.h"
#include "widgets.h"

/*
 * Vojto's Focusmate sessions. The widget is urgent from COUNTDOWN_S before a
 * session to its end, showing a countdown and then a pie, and hidden the rest
 * of the time. It chimes before, at and near the end of each session, and
 * ticks quietly every second of it, shown or not.
 */

using namespace deskkit;

#define COUNTDOWN_S 600     // a session starting within this gets the countdown
#define WARNING_S 60        // the first chime plays this long before the start
#define END_WARNING_S 60    // two descending notes this long before the end
#define START_CHIME_S 10    // the start chime is skipped once the session is further in than this
#define TICK_VOLUME 20      // 0-255, on top of the app's volume
#define TICK_HZ 1500
#define TICK_MS 4
#define TICK_CHANNEL 0      // the chimes take free channels from 7 down

static void startFocusmate();
static Priority focusmatePriority(time_t now);
static void updateFocusmate(time_t now);
static void playTick(const Session &session, time_t now);
static void playChimes(const Session &session, time_t now);

const Widget FOCUSMATE_WIDGET = {
    [](time_t now) { return sessionScreenKey(currentSession(now), now); },
    [](time_t now) { drawSessionScreen(currentSession(now), now); },
    startFocusmate,
    focusmatePriority,
    updateFocusmate,
};

static void startFocusmate() {
  M5.Speaker.setChannelVolume(TICK_CHANNEL, TICK_VOLUME);
  startFetchingSessions();
}

static Priority focusmatePriority(time_t now) {
  Session session = currentSession(now);
  bool isNear = session.isRunning(now) || session.isStartingWithin(COUNTDOWN_S, now);
  return isNear ? URGENT : HIDDEN;
}

// Runs before the drawing, which takes a while when the pie moves, so the
// ticks land on time
static void updateFocusmate(time_t now) {
  Session session = currentSession(now);
  playTick(session, now);
  playChimes(session, now);
}

// MARK: Sound

// A quiet click every second of a session
static void playTick(const Session &session, time_t now) {
  static time_t tickedAt = 0;
  if (!session.isRunning(now) || now == tickedAt) return;
  tickedAt = now;
  M5.Speaker.tone(TICK_HZ, TICK_MS, TICK_CHANNEL);
}

// Start chimes and an end warning, each once per session
static void playChimes(const Session &session, time_t now) {
  static time_t warnedStart = 0, chimedStart = 0, warnedEnd = 0;

  if (session.isStartingWithin(WARNING_S, now) && warnedStart != session.start) {
    warnedStart = session.start;
    playNotes({784, 1047});
  }
  // Not when the device boots mid-session or the session only shows up after it began
  bool hasJustStarted = session.isRunning(now) && now - session.start < START_CHIME_S;
  if (hasJustStarted && chimedStart != session.start) {
    chimedStart = session.start;
    playNotes({1047, 1319, 1568});
  }
  bool isEndingSoon = session.isRunning(now) && session.start + session.seconds - now <= END_WARNING_S;
  if (isEndingSoon && warnedEnd != session.start) {
    warnedEnd = session.start;
    playNotes({1047, 784});
  }
}
