#pragma once

/*
 * Template for include/secrets.h, which the app and the widgets (lib/widgets)
 * include and git ignores. Copy this file to include/secrets.h and fill in
 * your own values. Every widget's keys are needed, even for widgets the app
 * doesn't show, because the whole widgets library is built.
 */

// The Tab5's Wi-Fi chip joins 2.4 GHz networks (and 5 GHz on Wi-Fi 6)
#define WIFI_SSID "your-network"
#define WIFI_PASSWORD "your-password"

// From Focusmate: Settings > API key
#define FOCUSMATE_API_KEY "your-api-key"

// From Moonlite: Settings > API token
#define MOONLITE_API_TOKEN "your-api-token"

// From Foodmark: the token for the external weights API
#define FOODMARK_API_TOKEN "your-api-token"
