#include "network.h"
#include <Arduino.h>
#include <curl/curl.h>

/*
 * The simulator's version of src/network.cpp. The Mac is online and its clock
 * is set, so there is nothing to wait for; it only switches to the device's
 * time zone, so the simulator shows Bratislava time wherever the Mac is.
 */

void startNetwork() {
  setenv("TZ", TIME_ZONE, 1);
  tzset();
  curl_global_init(CURL_GLOBAL_DEFAULT);  // before the fetch threads start
}

bool isWifiConnected() { return true; }

bool isTimeSynced() { return true; }
