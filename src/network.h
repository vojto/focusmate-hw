#pragma once

/*
 * Wi-Fi and the device clock. startNetwork() joins the Wi-Fi from secrets.h and
 * keeps the time synced from the internet; the rest of the firmware asks here
 * whether either is ready yet.
 */

void startNetwork();
bool isWifiConnected();
bool isTimeSynced();
