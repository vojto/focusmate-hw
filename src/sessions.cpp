#include "sessions.h"
#include "fetcher.h"
#include "network.h"
#include "secrets.h"

/*
 * Fetches the booked sessions from Focusmate's API once a minute, in a
 * background task so the screen never waits for the network. The list also
 * holds the sessions of the last half day, so a running session can be
 * counted within its block.
 */

#define SESSIONS_URL "https://api.focusmate.com/v1/sessions"
#define LOOKAHEAD_S (24 * 3600)
#define LOOKBACK_S (12 * 3600)  // far enough to reach the start of the running block
#define BLOCK_GAP_S (10 * 60)   // the longest break between back-to-back sessions, after a 50-minute one
#define MAX_SESSIONS 32

struct SessionList {
  Session items[MAX_SESSIONS];  // oldest first
  int count;
};

static bool isBackToBack(const Session &earlier, const Session &later);
static String sessionsUrl();
static bool readSessions(JsonDocument &json);
static String formatIsoTime(time_t when);
static time_t parseIsoTime(const char *text);
static time_t utcSeconds(int year, int month, int day, int hour, int minute, int second);

static Shared<SessionList> sessions(SessionList{});
static Fetch fetch = {"sessions", sessionsUrl, "X-API-KEY", FOCUSMATE_API_KEY, 60000, 10000, readSessions};

// MARK: Reading and replacing the list

void startFetchingSessions() { startFetching(fetch); }

Session currentSession(time_t now) {
  SessionList list = sessions.get();
  Session current = {};
  for (int i = 0; i < list.count; i++) {
    bool isOver = list.items[i].start + list.items[i].seconds <= now;
    bool isLater = current.isBooked() && list.items[i].start >= current.start;
    if (!isOver && !isLater) current = list.items[i];
  }
  return current;
}

BlockPosition blockPosition(const Session &session) {
  SessionList list = sessions.get();
  BlockPosition position = {1, 1};
  int index = 0;
  while (index < list.count && list.items[index].start != session.start) index++;
  if (index == list.count) return position;
  for (int i = index; i > 0 && isBackToBack(list.items[i - 1], list.items[i]); i--) position.number++;
  position.total = position.number;
  for (int i = index; i + 1 < list.count && isBackToBack(list.items[i], list.items[i + 1]); i++) position.total++;
  return position;
}

const char *fetchProblem() {
  if (!isWifiConnected()) return "No Wi-Fi";
  if (fetch.hasFailed) return "Can't reach Focusmate";
  return "";
}

void setSessions(const Session *list, int count) {
  SessionList replaced = {};
  replaced.count = min(count, MAX_SESSIONS);
  for (int i = 0; i < replaced.count; i++) replaced.items[i] = list[i];
  sessions.set(replaced);
}

static bool isBackToBack(const Session &earlier, const Session &later) {
  return later.start - (earlier.start + earlier.seconds) <= BLOCK_GAP_S;
}

// MARK: Fetching

// Every session between half a day ago and a day from now
static String sessionsUrl() {
  time_t now = time(nullptr);
  return String(SESSIONS_URL) + "?start=" + formatIsoTime(now - LOOKBACK_S) + "&end=" + formatIsoTime(now + LOOKAHEAD_S);
}

static bool readSessions(JsonDocument &json) {
  JsonArray list = json["sessions"];
  SessionList found = {};
  // The list is newest first; walking it backwards keeps the soonest if there are too many
  for (int i = (int)list.size() - 1; i >= 0 && found.count < MAX_SESSIONS; i--) {
    time_t start = parseIsoTime(list[i]["startTime"] | "");
    int32_t milliseconds = list[i]["duration"] | 0;
    if (start > 0) found.items[found.count++] = {start, milliseconds / 1000};
  }
  sessions.set(found);
  return true;
}

// MARK: Time helpers

// "2026-10-02T19:30:00Z"
static String formatIsoTime(time_t when) {
  char text[24];
  struct tm utc;
  strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", gmtime_r(&when, &utc));
  return text;
}

// Reads Focusmate's "2026-10-02T19:30:00+00:00", which is always UTC. 0 if it isn't a time.
static time_t parseIsoTime(const char *text) {
  int year, month, day, hour, minute, second;
  if (sscanf(text, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) return 0;
  return utcSeconds(year, month, day, hour, minute, second);
}

// Seconds since 1970 for a UTC date and time. The ESP32's C library has no timegm.
static time_t utcSeconds(int year, int month, int day, int hour, int minute, int second) {
  year -= month <= 2;
  int era = year / 400;
  int yearOfEra = year - era * 400;
  int dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
  int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
  return (era * 146097L + dayOfEra - 719468L) * 86400L + hour * 3600L + minute * 60 + second;
}
