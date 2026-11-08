#include "fetcher.h"
#include <curl/curl.h>

/*
 * The simulator's version of src/fetcher.cpp: the same fetch loop, but in a
 * thread and over libcurl instead of a FreeRTOS task and the ESP32's HTTP
 * client. The Mac is always online, so it starts fetching right away.
 */

#define USER_AGENT "ESP32HTTPClient"  // what the device sends; Foodmark's Cloudflare refuses requests without one
#define TIMEOUT_S 30L

static void fetchLoop(Fetch &fetch);
static bool fetchOnce(Fetch &fetch);
static bool request(const Fetch &fetch, std::string &body);
static size_t appendBody(char *data, size_t size, size_t count, void *body);

void startFetching(Fetch &fetch) { std::thread(fetchLoop, std::ref(fetch)).detach(); }

static void fetchLoop(Fetch &fetch) {
  while (true) {
    fetch.hasFailed = !fetchOnce(fetch);
    delay(fetch.hasFailed ? fetch.retryMs : fetch.intervalMs);
  }
}

static bool fetchOnce(Fetch &fetch) {
  std::string body;
  if (!request(fetch, body)) return false;
  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;
  return fetch.read(doc);
}

static bool request(const Fetch &fetch, std::string &body) {
  String url = fetch.url();
  std::string header = std::string(fetch.header) + ": " + fetch.headerValue;
  curl_slist *headers = curl_slist_append(nullptr, header.c_str());
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
  ((std::string *)body)->append(data, size * count);
  return size * count;
}
