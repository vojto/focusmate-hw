#pragma once
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

/*
 * The few bits of the Arduino core the firmware uses, for the desktop
 * simulator (the `native` environment). String is a std::string that can be
 * built from numbers, the FreeRTOS lock is a mutex, and Serial is the
 * terminal: it reads commands from stdin and writes to stdout, so
 * src/debug.cpp and tools/screenshot.py work as they do over USB.
 */

#define PI 3.14159265358979323846

using std::max;
using std::min;

template <class T>
T constrain(T value, T low, T high) { return value < low ? low : value > high ? high : value; }

inline unsigned long millis() {
  using namespace std::chrono;
  static auto started = steady_clock::now();
  return duration_cast<milliseconds>(steady_clock::now() - started).count();
}

inline void delay(unsigned long milliseconds) { std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds)); }

class String : public std::string {
 public:
  String() {}
  String(const char *text) : std::string(text) {}
  String(const std::string &text) : std::string(text) {}
  String(int number) : std::string(std::to_string(number)) {}
  String(long number) : std::string(std::to_string(number)) {}
  String(unsigned number) : std::string(std::to_string(number)) {}
  String(unsigned long number) : std::string(std::to_string(number)) {}
  String(double number, int decimals) {
    char text[32];
    snprintf(text, sizeof(text), "%.*f", decimals, number);
    assign(text);
  }

  bool startsWith(const char *prefix) const { return rfind(prefix, 0) == 0; }
};

// Shared<T> in fetcher.h locks with these
typedef std::mutex portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED {}
#define portENTER_CRITICAL(mux) (mux)->lock()
#define portEXIT_CRITICAL(mux) (mux)->unlock()

class TerminalSerial {
 public:
  void begin(unsigned long) { fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK); }

  int available() {
    if (next < 0 && ::read(STDIN_FILENO, &byte, 1) == 1) next = byte;
    return next >= 0;
  }

  int read() {
    if (!available()) return -1;
    int value = next;
    next = -1;
    return value;
  }

  void println(const char *text) { printf("%s\n", text); }  // the template below, which flushes

  template <class... Args>
  void printf(const char *format, Args... args) {
    ::printf(format, args...);
    fflush(stdout);
  }

  size_t write(const uint8_t *data, size_t length) {
    size_t written = fwrite(data, 1, length, stdout);
    fflush(stdout);
    return written;
  }

 private:
  unsigned char byte;
  int next = -1;
};

inline TerminalSerial Serial;
