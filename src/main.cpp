#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

/*
 * Focusmate session display for an M5Stack Core2. A background task fetches the
 * booked sessions once a minute; the main loop picks the screen from them and
 * the clock: a shrinking pie while a session runs, a countdown when one starts
 * within the hour, otherwise the time, date and next start time. The speaker
 * chimes a minute before a session and when it starts.
 */

// Europe/Bratislava, including daylight saving time changes
#define TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"

#define SESSIONS_URL "https://api.focusmate.com/v1/sessions"
#define FETCH_INTERVAL_MS 60000
#define FETCH_RETRY_MS 10000
#define LOOKAHEAD_S (24 * 3600)
#define MAX_SESSIONS 16
#define COUNTDOWN_S 3600  // a session starting within this gets the countdown
#define WARNING_S 60      // the first chime plays this long before the start

#define CENTER_X 160
#define CENTER_Y 120
#define PIE_RADIUS 104

// Colors are 0xRRGGBB; M5GFX reads uint32_t that way
const uint32_t BACKGROUND = 0x000000, TEXT_WHITE = 0xF1EFE8, TEXT_GRAY = 0xB4B2A9;
const uint32_t PIE_RED = 0xE24B4A, PIE_USED = 0x2C2C2A;
const uint32_t COUNTDOWN_GREEN = 0x5DCAA5, NOTICE_ORANGE = 0xEF9F27;

struct Session {
  time_t start;
  int32_t seconds;
};

// Written by fetchTask, read by the main loop
portMUX_TYPE sessionsLock = portMUX_INITIALIZER_UNLOCKED;
Session sessions[MAX_SESSIONS];
int sessionCount = 0;
volatile bool hasFetchFailed = false;

enum Screen { SCREEN_NONE, SCREEN_CLOCK, SCREEN_COUNTDOWN, SCREEN_PIE };
enum Line { LINE_TOP, LINE_BIG, LINE_MIDDLE, LINE_BOTTOM, LINE_COUNT };
String shownLines[LINE_COUNT];

bool isTimeSynced() { return time(nullptr) > 1600000000; }

// Seconds since 1970 for a UTC date and time. The ESP32's C library has no timegm.
time_t utcSeconds(int year, int month, int day, int hour, int minute, int second) {
  year -= month <= 2;
  int era = year / 400;
  int yearOfEra = year - era * 400;
  int dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
  int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
  return (era * 146097L + dayOfEra - 719468L) * 86400L + hour * 3600L + minute * 60 + second;
}

// Replaces the session list with what Focusmate has booked for the next day
bool fetchSessions() {
  char start[24], end[24], url[128];
  time_t now = time(nullptr), later = now + LOOKAHEAD_S;
  struct tm utc;
  strftime(start, sizeof(start), "%Y-%m-%dT%H:%M:%SZ", gmtime_r(&now, &utc));
  strftime(end, sizeof(end), "%Y-%m-%dT%H:%M:%SZ", gmtime_r(&later, &utc));
  snprintf(url, sizeof(url), SESSIONS_URL "?start=%s&end=%s", start, end);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("X-API-KEY", FOCUSMATE_API_KEY);
  if (http.GET() != HTTP_CODE_OK) return false;

  JsonDocument doc;
  if (deserializeJson(doc, http.getString())) return false;
  JsonArray list = doc["sessions"];

  Session found[MAX_SESSIONS];
  int count = 0;
  // The list is newest first; walking it backwards keeps the soonest if there are too many
  for (int i = (int)list.size() - 1; i >= 0 && count < MAX_SESSIONS; i--) {
    int year, month, day, hour, minute, second;
    const char *startTime = list[i]["startTime"] | "";
    if (sscanf(startTime, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) continue;
    found[count].start = utcSeconds(year, month, day, hour, minute, second);
    found[count].seconds = list[i]["duration"].as<int32_t>() / 1000;
    count++;
  }

  portENTER_CRITICAL(&sessionsLock);
  memcpy(sessions, found, sizeof(found));
  sessionCount = count;
  portEXIT_CRITICAL(&sessionsLock);
  return true;
}

// The request runs in fetchSessions() so its objects are freed between fetches
void fetchTask(void *) {
  while (true) {
    if (WiFi.status() != WL_CONNECTED || !isTimeSynced()) {
      delay(1000);
      continue;
    }
    hasFetchFailed = !fetchSessions();
    delay(hasFetchFailed ? FETCH_RETRY_MS : FETCH_INTERVAL_MS);
  }
}

// Finds the session that is running or comes next
bool findSession(time_t now, Session &session) {
  bool isFound = false;
  portENTER_CRITICAL(&sessionsLock);
  for (int i = 0; i < sessionCount; i++) {
    if (sessions[i].start + sessions[i].seconds <= now) continue;
    if (isFound && sessions[i].start >= session.start) continue;
    session = sessions[i];
    isFound = true;
  }
  portEXIT_CRITICAL(&sessionsLock);
  return isFound;
}

#ifdef DEBUG_TOOLS
#include "debug.h"
#endif

void clearScreen() {
  M5.Display.fillScreen(BACKGROUND);
  for (String &line : shownLines) line = "";
}

// Draws a centered line of text, only when it changed. It clears the full
// screen width, so a shorter text leaves nothing behind from a longer one.
void drawLine(Line line, const char *text, int y, const lgfx::IFont *font, float size, uint32_t color) {
  if (shownLines[line] == text) return;
  shownLines[line] = text;
  M5.Display.setFont(font);
  M5.Display.setTextSize(size);
  M5.Display.setTextColor(color, BACKGROUND);
  M5.Display.setTextPadding(M5.Display.width());
  M5.Display.drawString(text, CENTER_X, y);
}

void drawClock(time_t now, bool hasSession, const Session &session) {
  const char *notice = "";
  if (WiFi.status() != WL_CONNECTED) notice = "No Wi-Fi";
  else if (hasFetchFailed) notice = "Can't reach Focusmate";
  drawLine(LINE_TOP, notice, 18, &fonts::FreeSans9pt7b, 1, NOTICE_ORANGE);

  if (!isTimeSynced()) {
    drawLine(LINE_MIDDLE, "Syncing time...", 150, &fonts::FreeSans12pt7b, 1, TEXT_GRAY);
    return;
  }

  char text[32];
  struct tm local;
  localtime_r(&now, &local);
  strftime(text, sizeof(text), "%H:%M:%S", &local);
  drawLine(LINE_BIG, text, 88, &fonts::Font7, 1.4f, TEXT_WHITE);
  strftime(text, sizeof(text), "%a %d %b", &local);
  drawLine(LINE_MIDDLE, text, 150, &fonts::FreeSans12pt7b, 1, TEXT_GRAY);

  text[0] = 0;
  if (hasSession) {
    localtime_r(&session.start, &local);
    strftime(text, sizeof(text), "Next session %H:%M", &local);
  }
  drawLine(LINE_BOTTOM, text, 195, &fonts::FreeSans12pt7b, 1, TEXT_WHITE);
}

void drawCountdown(int32_t secondsLeft) {
  char text[8];
  snprintf(text, sizeof(text), "%d:%02d", (int)(secondsLeft / 60), (int)(secondsLeft % 60));
  drawLine(LINE_TOP, "Session in", 52, &fonts::FreeSans12pt7b, 1, TEXT_GRAY);
  drawLine(LINE_BIG, text, 132, &fonts::Font7, 2, COUNTDOWN_GREEN);
}

// The used-up part grows clockwise from 12 o'clock, like on a Time Timer.
// fillArc angles run clockwise from 3 o'clock, so 270 is the top.
void drawPie(const Session &session, time_t now, bool isFirstDraw) {
  static float shownAngle = 0;
  float angle = 360.0f * (now - session.start) / session.seconds;
  if (isFirstDraw) {
    M5.Display.fillArc(CENTER_X, CENTER_Y, 0, PIE_RADIUS, 270 + angle, 270 + 360, PIE_RED);
  } else if (angle - shownAngle < 0.25f) {
    return;
  }
  M5.Display.fillArc(CENTER_X, CENTER_Y, 0, PIE_RADIUS, 270, 270 + angle, PIE_USED);
  shownAngle = angle;
}

void playNotes(std::initializer_list<int> frequencies) {
  for (int frequency : frequencies) {
    M5.Speaker.tone(frequency, 140);
    delay(160);
  }
}

void setup() {
  auto cfg = M5.config();
  // Otherwise the clock starts from whatever the Core2's clock chip holds
  cfg.internal_rtc = false;
  M5.begin(cfg);
  M5.Display.setBrightness(100);  // 0-255
  M5.Display.setTextDatum(middle_center);
  M5.Speaker.setVolume(128);  // 0-255

  // Syncs time from the internet on every connect, then again every hour.
  // Started earlier, the first request fails and the retry comes 30s later.
  WiFi.onEvent([](WiFiEvent_t) { configTzTime(TIME_ZONE, "pool.ntp.org"); }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  xTaskCreate(fetchTask, "fetch", 12288, nullptr, 1, nullptr);
#ifdef DEBUG_TOOLS
  Serial.begin(DEBUG_BAUD);
#endif
}

void loop() {
#ifdef DEBUG_TOOLS
  debugLoop();
#endif
  static Screen shownScreen = SCREEN_NONE;
  static time_t warnedStart = 0, chimedStart = 0;

  time_t now = time(nullptr);
  Session session = {};
  bool hasSession = isTimeSynced() && findSession(now, session);
  int32_t secondsLeft = session.start - now;  // until the start; negative once running

  Screen screen = SCREEN_CLOCK;
  if (hasSession && secondsLeft <= 0) screen = SCREEN_PIE;
  else if (hasSession && secondsLeft <= COUNTDOWN_S) screen = SCREEN_COUNTDOWN;

  bool isFirstDraw = screen != shownScreen;
  if (isFirstDraw) clearScreen();
  shownScreen = screen;

  if (screen == SCREEN_PIE) drawPie(session, now, isFirstDraw);
  else if (screen == SCREEN_COUNTDOWN) drawCountdown(secondsLeft);
  else drawClock(now, hasSession, session);

  if (screen == SCREEN_COUNTDOWN && secondsLeft <= WARNING_S && warnedStart != session.start) {
    warnedStart = session.start;
    playNotes({784, 1047});
  }
  // Not when the device boots or the session only shows up after it began
  if (screen == SCREEN_PIE && -secondsLeft < 10 && chimedStart != session.start) {
    chimedStart = session.start;
    playNotes({1047, 1319, 1568});
  }
  delay(100);
}
