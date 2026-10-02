#pragma once

/*
 * Template for src/secrets.h, which main.cpp includes and git ignores.
 * Copy this file to src/secrets.h and fill in your own values.
 */

// The ESP32 only joins 2.4 GHz networks
#define WIFI_SSID "your-2.4-ghz-network"
#define WIFI_PASSWORD "your-password"

// From Focusmate: Settings > API key
#define FOCUSMATE_API_KEY "your-api-key"
