#include "sessions.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "network.h"
#include "secrets.h"

/*
 * Fetches the booked sessions from Focusmate's API once a minute, in a
 * background task so the screen never waits for the network. The list is
 * shared with the main loop under a lock.
 */

#define SESSIONS_URL "https://api.focusmate.com/v1/sessions"
#define FETCH_INTERVAL_MS 60000
#define FETCH_RETRY_MS 10000
#define LOOKAHEAD_S (24 * 3600)
#define MAX_SESSIONS 16
#define TASK_STACK_BYTES 12288  // enough for the TLS handshake

static void fetchTask(void *);
static bool fetchSessions();
static bool requestSessions(String &json);
static int parseSessions(const String &json, Session *found);
static String formatIsoTime(time_t when);
static time_t parseIsoTime(const char *text);
static time_t utcSeconds(int year, int month, int day, int hour, int minute, int second);

// Written by the fetch task, read by the main loop
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static Session sessions[MAX_SESSIONS];
static int sessionCount = 0;
static volatile bool hasFetchFailed = false;

// MARK: Reading and replacing the list

void startFetchingSessions() { xTaskCreate(fetchTask, "fetch", TASK_STACK_BYTES, nullptr, 1, nullptr); }

Session currentSession(time_t now) {
  Session current = {};
  portENTER_CRITICAL(&lock);
  for (int i = 0; i < sessionCount; i++) {
    bool isOver = sessions[i].start + sessions[i].seconds <= now;
    bool isLater = current.isBooked() && sessions[i].start >= current.start;
    if (!isOver && !isLater) current = sessions[i];
  }
  portEXIT_CRITICAL(&lock);
  return current;
}

const char *fetchProblem() {
  if (!isWifiConnected()) return "No Wi-Fi";
  if (hasFetchFailed) return "Can't reach Focusmate";
  return "";
}

void setSessions(const Session *list, int count) {
  portENTER_CRITICAL(&lock);
  sessionCount = min(count, MAX_SESSIONS);
  for (int i = 0; i < sessionCount; i++) sessions[i] = list[i];
  portEXIT_CRITICAL(&lock);
}

// MARK: Fetching

static void fetchTask(void *) {
  while (true) {
    if (!isWifiConnected() || !isTimeSynced()) {
      delay(1000);
      continue;
    }
    // The request lives in its own function so its objects are freed between fetches
    hasFetchFailed = !fetchSessions();
    delay(hasFetchFailed ? FETCH_RETRY_MS : FETCH_INTERVAL_MS);
  }
}

static bool fetchSessions() {
  String json;
  Session found[MAX_SESSIONS];
  if (!requestSessions(json)) return false;
  int count = parseSessions(json, found);
  if (count < 0) return false;
  setSessions(found, count);
  return true;
}

// Asks for every session between now and a day from now
static bool requestSessions(String &json) {
  time_t now = time(nullptr);
  String url = String(SESSIONS_URL) + "?start=" + formatIsoTime(now) + "&end=" + formatIsoTime(now + LOOKAHEAD_S);

  WiFiClientSecure client;
  client.setInsecure();  // no certificate check
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("X-API-KEY", FOCUSMATE_API_KEY);
  if (http.GET() != HTTP_CODE_OK) return false;
  json = http.getString();
  return true;
}

// Returns how many sessions it put into `found`, or -1 if the JSON can't be read
static int parseSessions(const String &json, Session *found) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return -1;
  JsonArray list = doc["sessions"];

  int count = 0;
  // The list is newest first; walking it backwards keeps the soonest if there are too many
  for (int i = (int)list.size() - 1; i >= 0 && count < MAX_SESSIONS; i--) {
    time_t start = parseIsoTime(list[i]["startTime"] | "");
    int32_t milliseconds = list[i]["duration"] | 0;
    if (start > 0) found[count++] = {start, milliseconds / 1000};
  }
  return count;
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
