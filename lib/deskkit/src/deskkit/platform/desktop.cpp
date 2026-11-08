#ifndef ESP_PLATFORM
#include <curl/curl.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include "platform.h"

/*
 * The platform in the desktop simulator. The computer is online and its clock
 * is set, so there is nothing to wait for. Tasks are threads, requests go
 * through libcurl, and the serial port is the terminal: stdin and stdout.
 */

#define USER_AGENT "ESP32HTTPClient"  // what the device sends; some servers refuse requests without one
#define TIMEOUT_S 30L

namespace deskkit {
namespace platform {

static size_t appendBody(char *data, size_t size, size_t count, void *body);

// MARK: Network

// The device's time zone, so the simulator shows its time wherever the computer is
void startNetwork(const char *, const char *, const char *timeZone) {
  setenv("TZ", timeZone, 1);
  tzset();
  curl_global_init(CURL_GLOBAL_DEFAULT);  // before any task makes a request
}

bool isWifiConnected() { return true; }

// MARK: Tasks and requests

void startTask(void (*run)(void *argument), void *argument, const char *) { std::thread(run, argument).detach(); }

bool httpGet(const String &url, const char *header, const char *headerValue, String &body) {
  String line = String(header) + ": " + headerValue;
  curl_slist *headers = curl_slist_append(nullptr, line.c_str());
  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT_S);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, appendBody);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
  long status = 0;
  bool isDone = curl_easy_perform(curl) == CURLE_OK;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  return isDone && status == 200;
}

static size_t appendBody(char *data, size_t size, size_t count, void *body) {
  ((String *)body)->append(data, size * count);
  return size * count;
}

// MARK: Serial

// Reads never wait, like Serial.available() on the device
void startSerial() { fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK); }

int readSerial() {
  unsigned char byte;
  return read(STDIN_FILENO, &byte, 1) == 1 ? byte : -1;
}

void writeSerial(const uint8_t *data, size_t length) {
  fwrite(data, 1, length, stdout);
  fflush(stdout);
}

}  // namespace platform
}  // namespace deskkit
#endif
