#include "debug.h"
#include <deskkit/console.h>
#include "data/sessions.h"
#include "screens/slides.h"

/*
 * Each command fakes a session relative to now. A real fetch replaces it
 * within a minute.
 */

static void fakeSession(int32_t secondsUntilStart, int32_t seconds);
static void fakeBlock(int32_t secondsUntilThird);

void addDebugCommands() {
  deskkit::addCommand('p', [] { fakeSession(-18 * 60, 50 * 60); });                // pie, 32 of 50 minutes left
  deskkit::addCommand('q', [] { fakeSession(-74 * 60, 75 * 60); });                // pie, last minute of 75
  deskkit::addCommand('b', [] { fakeBlock(-18 * 60); });                           // pie, third of four sessions in a row
  deskkit::addCommand('d', [] { fakeBlock(7 * 60 + 42); });                        // countdown to the third of four
  deskkit::addCommand('c', [] { fakeSession(7 * 60 + 42, 50 * 60); });             // countdown
  deskkit::addCommand('w', [] { fakeSession(63, 25 * 60); });                      // warning chime in 3s, start in 63s
  deskkit::addCommand('n', [] { fakeSession(2 * 3600 + 23 * 60, 50 * 60); });      // slides with a later session
  deskkit::addCommand('e', [] { setSessions(nullptr, 0); });                       // slides, nothing booked
  deskkit::addCommand('t', [] { slideShow.next(); });                              // the next slide, like a tap
}

static void fakeSession(int32_t secondsUntilStart, int32_t seconds) {
  Session session = {time(nullptr) + secondsUntilStart, seconds};
  setSessions(&session, 1);
}

// Four 50-minute sessions an hour apart
static void fakeBlock(int32_t secondsUntilThird) {
  Session block[4];
  time_t firstStart = time(nullptr) + secondsUntilThird - 2 * 3600;
  for (int i = 0; i < 4; i++) block[i] = {firstStart + i * 3600, 50 * 60};
  setSessions(block, 4);
}
