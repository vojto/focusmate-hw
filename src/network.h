#pragma once

/*
 * Wi-Fi and the device clock. startNetwork() joins the Wi-Fi from secrets.h and
 * keeps the time synced from the internet; the rest of the firmware asks here
 * whether either is ready yet.
 */

// Europe/Bratislava, including daylight saving time changes
#define TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"

void startNetwork();
bool isWifiConnected();
bool isTimeSynced();
