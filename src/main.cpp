#include <M5Unified.h>
#include "debug.h"
#include "network.h"
#include "screens.h"
#include "sessions.h"

/*
 * Focusmate session display for an M5Stack Core2. The loop looks at the
 * session that is running or comes next and decides everything from it: which
 * screen to show (a pie during a session, a countdown before one, otherwise a
 * clock), how bright the screen is, and when to chime.
 */

#define COUNTDOWN_S 3600    // a session starting within this gets the countdown
#define BRIGHT_S 600        // the screen is bright from this long before a session until it ends
#define WARNING_S 60        // the first chime plays this long before the start
#define START_CHIME_S 10    // the start chime is skipped once the session is further in than this
#define BRIGHTNESS 100      // 0-255
#define DIM_BRIGHTNESS 24
#define VOLUME 128          // 0-255

static void playChimes(const Session &session, time_t now);
static void playNotes(std::initializer_list<int> frequencies);

void setup() {
  auto cfg = M5.config();
  // Otherwise the clock starts from whatever the Core2's clock chip holds
  cfg.internal_rtc = false;
  M5.begin(cfg);
  M5.Speaker.setVolume(VOLUME);

  startNetwork();
  startFetchingSessions();
  debugSetup();
}

void loop() {
  debugLoop();
  time_t now = time(nullptr);
  Session session = currentSession(now);

  if (session.isRunning(now)) showPie(session.usedFraction(now));
  else if (session.isStartingWithin(COUNTDOWN_S, now)) showCountdown(session.start - now);
  else showClock(now, session, fetchProblem());

  bool isSessionNear = session.isRunning(now) || session.isStartingWithin(BRIGHT_S, now);
  setScreenBrightness(isSessionNear ? BRIGHTNESS : DIM_BRIGHTNESS);
  playChimes(session, now);
  delay(100);
}

// Two notes a minute before a session and three when it starts, once per session
static void playChimes(const Session &session, time_t now) {
  static time_t warnedStart = 0, chimedStart = 0;

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
}

// Frequencies in Hz
static void playNotes(std::initializer_list<int> frequencies) {
  for (int frequency : frequencies) {
    M5.Speaker.tone(frequency, 140);
    delay(160);
  }
}
