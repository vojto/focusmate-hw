#pragma once

/*
 * Whether the device is online and its clock is right. begin() in app.h joins
 * the Wi-Fi and starts syncing the clock; screens and fetches ask here.
 */

namespace deskkit {

bool isWifiConnected();

// The clock starts at 1970 on the device and is right once synced from the internet
bool isTimeSynced();

}  // namespace deskkit
