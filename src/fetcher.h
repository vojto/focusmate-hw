#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

/*
 * What the data modules (sessions, health, weight) share: a background task
 * per source that asks an HTTPS JSON API every few minutes, once Wi-Fi and the
 * clock are up, and hands the parsed JSON to the source. The source keeps
 * what it needs in a Shared value, which the main loop reads.
 */

struct Fetch {
  const char *name;  // the task's
  String (*url)();
  const char *header;  // the one request header, like "Authorization"
  const char *headerValue;
  uint32_t intervalMs;
  uint32_t retryMs;                   // after a failed fetch
  bool (*read)(JsonDocument &json);   // keeps what it needs; false if the JSON is unusable
  volatile bool hasFailed;            // the last fetch; leave it out when filling in the rest
};

// The task keeps using the fetch, so it must be static
void startFetching(Fetch &fetch);

// A value written by a fetch task and read by the main loop, under a lock
template <class T>
class Shared {
 public:
  explicit Shared(const T &initial) : value(initial) {}

  T get() {
    portENTER_CRITICAL(&lock);
    T copy = value;
    portEXIT_CRITICAL(&lock);
    return copy;
  }

  void set(const T &newValue) {
    portENTER_CRITICAL(&lock);
    value = newValue;
    portEXIT_CRITICAL(&lock);
  }

 private:
  portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
  T value;
};
