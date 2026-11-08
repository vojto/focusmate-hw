#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <mutex>

/*
 * Data from web APIs, kept fresh in the background. Each source is a Fetch: a
 * task asks its HTTPS JSON API every few minutes, once Wi-Fi and the clock
 * are up, and hands the parsed JSON to the source's read function. The source
 * keeps what it needs in a Shared value, which the main loop reads.
 */

namespace deskkit {

struct Fetch {
  const char *name;  // the task's
  String (*url)();
  const char *header;  // the one request header, like "Authorization"
  const char *headerValue;
  uint32_t intervalMs;
  uint32_t retryMs;                   // after a failed fetch
  bool (*read)(JsonDocument &json);   // keeps what it needs; false if the JSON is unusable
  volatile bool hasFailed;            // the last fetch; leave this and the rest out when filling it in
  bool hasStarted;
};

// The task keeps using the fetch, so it must be static. Starts it only once,
// however many widgets ask.
void startFetching(Fetch &fetch);

// A lock that the main loop and a fetch task can share
#ifdef ESP_PLATFORM
class Lock {
 public:
  void lock() { portENTER_CRITICAL(&mux); }
  void unlock() { portEXIT_CRITICAL(&mux); }

 private:
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
};
#else
typedef std::mutex Lock;
#endif

// A value written by a fetch task and read by the main loop
template <class T>
class Shared {
 public:
  explicit Shared(const T &initial) : value(initial) {}

  T get() {
    std::lock_guard<Lock> guard(lock);
    return value;
  }

  void set(const T &newValue) {
    std::lock_guard<Lock> guard(lock);
    value = newValue;
  }

 private:
  Lock lock;
  T value;
};

}  // namespace deskkit
