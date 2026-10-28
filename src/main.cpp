#include <M5Unified.h>
#include "debug.h"
#include "display.h"
#include "health.h"
#include "network.h"
#include "screens.h"
#include "sessions.h"
#include "slides.h"
#include "weight.h"

/*
 * Focusmate session display for an M5Stack Core2. The loop looks at the
 * session that is running or comes next and decides everything from it: which
 * screen to show (a pie during a session, a countdown before one, otherwise
 * the slides) and when to chime and tick. Taps go through the slides, and
 * around a session the session's own screen takes a turn among them.
 */

#define COUNTDOWN_S 600     // a session starting within this gets the countdown
#define WARNING_S 60        // the first chime plays this long before the start
#define END_WARNING_S 60    // two descending notes this long before the end
#define START_CHIME_S 10    // the start chime is skipped once the session is further in than this
#define BRIGHTNESS 100      // 0-255
#define VOLUME 128          // 0-255
#define TICK_VOLUME 20      // 0-255, on top of VOLUME
#define TICK_HZ 1500
#define TICK_MS 4
#define TICK_CHANNEL 0      // the chimes take free channels from 7 down
#define LOOP_MS 20          // short, so the ticks land evenly
#define PEEK_MS 10000       // a session's screen comes back this long after the last tap
#define TAP_HZ 2000         // the click that confirms a tap
#define TAP_MS 15

static bool isShowingSlides(bool isSessionNear);
static void playChimes(const Session &session, time_t now);
static void playTick(const Session &session, time_t now);
static void playNotes(std::initializer_list<int> frequencies);

void setup() {
  auto cfg = M5.config();
  // Otherwise the clock starts from whatever the Core2's clock chip holds
  cfg.internal_rtc = false;
  M5.begin(cfg);
  M5.Display.setBrightness(BRIGHTNESS);
  M5.Speaker.setVolume(VOLUME);
  M5.Speaker.setChannelVolume(TICK_CHANNEL, TICK_VOLUME);

  startDisplay();
  startNetwork();
  startFetchingSessions();
  startFetchingHealth();
  startFetchingWeight();
  debugSetup();
}

void loop() {
  M5.update();
  debugLoop();
  time_t now = time(nullptr);
  Session session = currentSession(now);
  // Before the drawing, which takes a while when the pie moves
  playTick(session, now);

  bool isSessionNear = session.isRunning(now) || session.isStartingWithin(COUNTDOWN_S, now);
  if (isShowingSlides(isSessionNear)) showSlides(now, fetchProblem());
  else if (session.isRunning(now)) showPie(session.usedMinutes(now), session.minutes(), blockPosition(session));
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
  if (M5.Touch.getDetail().wasPressed()) {
    M5.Speaker.tone(TAP_HZ, TAP_MS);
    tappedAt = millis();
    if (isSessionNear && !isPeeking) isPeeking = true;
    else if (isSessionNear && isLastSlide()) isPeeking = false;
    else nextSlide();
  }
  if (!isSessionNear || millis() - tappedAt >= PEEK_MS) isPeeking = false;
  return !isSessionNear || isPeeking;
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

// Frequencies in Hz
static void playNotes(std::initializer_list<int> frequencies) {
  for (int frequency : frequencies) {
    M5.Speaker.tone(frequency, 140);
    delay(160);
  }
}
