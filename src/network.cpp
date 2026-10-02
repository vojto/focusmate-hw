#include "network.h"
#include <WiFi.h>
#include "secrets.h"

/*
 * Joins the home Wi-Fi and syncs the clock over the internet (SNTP). The sync
 * starts when Wi-Fi gets its address and then repeats every hour by itself.
 */

// Europe/Bratislava, including daylight saving time changes
#define TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#define TIME_SERVER "pool.ntp.org"
// The clock starts at 1970 and jumps past this (September 2020) once synced
#define SYNCED_AFTER 1600000000

void startNetwork() {
  // Started before Wi-Fi is up, the first request fails and the retry comes 30s later
  WiFi.onEvent([](WiFiEvent_t) { configTzTime(TIME_ZONE, TIME_SERVER); }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool isWifiConnected() { return WiFi.status() == WL_CONNECTED; }

bool isTimeSynced() { return time(nullptr) > SYNCED_AFTER; }
