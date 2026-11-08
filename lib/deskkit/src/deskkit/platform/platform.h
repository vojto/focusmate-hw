#pragma once
#include <Arduino.h>

/*
 * The only part of deskkit that differs between the device and the desktop
 * simulator: Wi-Fi, background tasks, web requests and the serial port.
 * esp32.cpp implements it with the ESP32's Arduino core, desktop.cpp with
 * threads, libcurl and the terminal; each compiles only for its own platform.
 * Everything else in deskkit is built on these and runs the same on both.
 */

namespace deskkit {
namespace platform {

// Joins the Wi-Fi and keeps the clock synced from the internet, in the time
// zone (a POSIX TZ string). On the desktop it only sets the time zone.
void startNetwork(const char *ssid, const char *password, const char *timeZone);

bool isWifiConnected();

// Runs run(argument) in the background, forever
void startTask(void (*run)(void *argument), void *argument, const char *name);

// An HTTPS GET with one header; the body of a 200 response, false otherwise
bool httpGet(const String &url, const char *header, const char *headerValue, String &body);

void startSerial();

// The next byte that came in, or -1 when none did
int readSerial();

void writeSerial(const uint8_t *data, size_t length);

}  // namespace platform
}  // namespace deskkit
