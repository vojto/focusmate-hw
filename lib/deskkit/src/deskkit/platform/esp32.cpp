#ifdef ESP_PLATFORM
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "platform.h"

/*
 * The platform on the device: the ESP32's Wi-Fi, SNTP clock, FreeRTOS tasks,
 * HTTP client and USB serial.
 */

#define TIME_SERVER "pool.ntp.org"
#define TASK_STACK_BYTES 12288  // enough for a TLS handshake
#define SERIAL_BAUD 460800

namespace deskkit {
namespace platform {

static const char *timeZone;

// MARK: Network

// Started before Wi-Fi is up, the first sync fails and the retry comes 30s
// later, so it starts when Wi-Fi gets its address; then it repeats every hour
void startNetwork(const char *ssid, const char *password, const char *zone) {
  timeZone = zone;
  WiFi.onEvent([](WiFiEvent_t) { configTzTime(timeZone, TIME_SERVER); }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.begin(ssid, password);
}

bool isWifiConnected() { return WiFi.status() == WL_CONNECTED; }

// MARK: Tasks and requests

void startTask(void (*run)(void *argument), void *argument, const char *name) {
  xTaskCreate(run, name, TASK_STACK_BYTES, argument, 1, nullptr);
}

// The TLS client lives only for the request: kept alive between requests, it leaks
bool httpGet(const String &url, const char *header, const char *headerValue, String &body) {
  WiFiClientSecure client;
  client.setInsecure();  // no certificate check
  HTTPClient http;
  http.begin(client, url);
  http.addHeader(header, headerValue);
  if (http.GET() != HTTP_CODE_OK) return false;
  body = http.getString();
  return true;
}

// MARK: Serial

void startSerial() { Serial.begin(SERIAL_BAUD); }

int readSerial() { return Serial.available() ? Serial.read() : -1; }

void writeSerial(const uint8_t *data, size_t length) { Serial.write(data, length); }

}  // namespace platform
}  // namespace deskkit
#endif
