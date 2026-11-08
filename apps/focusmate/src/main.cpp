#include <Arduino.h>
#include <M5Unified.h>
#include <deskkit/app.h>
#include <deskkit/display.h>
#include <deskkit/network.h>
#include <deskkit/sound.h>
#include "data/health.h"
#include "data/sessions.h"
#include "data/weight.h"
#include "debug.h"
#include "screens/session.h"
#include "screens/slides.h"
#include "secrets.h"

/*
 * Focusmate session display for an M5Stack Core2. The loop looks at the
 * session that is running or comes next and decides everything from it: which
 * screen to show (a pie during a session, a countdown before one, otherwise
 * the slides) and when to chime and tick. Taps go through the slides, and
 * around a session the session's own screen takes a turn among them.
 */

using namespace deskkit;

#define COUNTDOWN_S 600     // a session starting within this gets the countdown
#define WARNING_S 60        // the first chime plays this long before the start
#define END_WARNING_S 60    // two descending notes this long before the end
#define START_CHIME_S 10    // the start chime is skipped once the session is further in than this
#define BRIGHTNESS 255      // 0-255, the most
#define NIGHT_BRIGHTNESS 1  // the dimmest that still shows anything
#define NIGHT_START_HOUR 23 // night runs from here to NIGHT_END_HOUR, local time
#define NIGHT_END_HOUR 7
#define VOLUME 128          // 0-255
#define TICK_VOLUME 20      // 0-255, on top of VOLUME
#define TICK_HZ 1500
#define TICK_MS 4
#define TICK_CHANNEL 0      // the chimes take free channels from 7 down
#define LOOP_MS 20          // short, so the ticks land evenly
#define PEEK_MS 10000       // a session's screen comes back this long after the last tap
#define TAP_HZ 2000         // the click that confirms a tap
#define TAP_MS 15

// Europe/Bratislava, including daylight saving time changes
const Config CONFIG = {WIFI_SSID, WIFI_PASSWORD, "CET-1CEST,M3.5.0,M10.5.0/3"};

static bool isShowingSlides(bool isSessionNear);
static void updateBrightness(bool isSessionNear, time_t now);
static void playChimes(const Session &session, time_t now);
static void playTick(const Session &session, time_t now);

void setup() {
  begin(CONFIG);
  M5.Speaker.setVolume(VOLUME);
  M5.Speaker.setChannelVolume(TICK_CHANNEL, TICK_VOLUME);
  startFetchingSessions();
  startFetchingHealth();
  startFetchingWeight();
  addDebugCommands();
}

void loop() {
  update();
  time_t now = time(nullptr);
  Session session = currentSession(now);
  // Before the drawing, which takes a while when the pie moves
  playTick(session, now);

  bool isSessionNear = session.isRunning(now) || session.isStartingWithin(COUNTDOWN_S, now);
  updateBrightness(isSessionNear, now);
  if (isShowingSlides(isSessionNear)) slideShow.show(now, fetchProblem());
  else if (session.isRunning(now)) showPie(session.usedMinutes(now), session.minutes(), blockPosition(session), now);
  else showCountdown(session.start - now, blockPosition(session));

  playChimes(session, now);
  delay(LOOP_MS);
}

// Between sessions a tap moves to the next slide. Around a session the pie or
// countdown takes a turn too: a tap swaps it for the first slide, and a tap on
// the last slide brings it back, as does PEEK_MS without a tap.
static bool isShowingSlides(bool isSessionNear) {
  static bool isPeeking = false;
  static uint32_t tappedAt = 0;
  if (wasTapped()) {
    M5.Speaker.tone(TAP_HZ, TAP_MS);
    tappedAt = millis();
    if (isSessionNear && !isPeeking) isPeeking = true;
    else if (isSessionNear && slideShow.isLast()) isPeeking = false;
    else slideShow.next();
  }
  if (!isSessionNear || millis() - tappedAt >= PEEK_MS) isPeeking = false;
  return !isSessionNear || isPeeking;
}

// Dim at night, unless a session is near; full before the clock is set
static void updateBrightness(bool isSessionNear, time_t now) {
  struct tm local;
  localtime_r(&now, &local);
  bool isNight = isTimeSynced() && (local.tm_hour >= NIGHT_START_HOUR || local.tm_hour < NIGHT_END_HOUR);
  setBrightness(isNight && !isSessionNear ? NIGHT_BRIGHTNESS : BRIGHTNESS);
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

// A quiet click every second of a session
static void playTick(const Session &session, time_t now) {
  static time_t tickedAt = 0;
  if (!session.isRunning(now) || now == tickedAt) return;
  tickedAt = now;
  M5.Speaker.tone(TICK_HZ, TICK_MS, TICK_CHANNEL);
}
