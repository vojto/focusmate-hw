#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <algorithm>
#include <chrono>
#include <string>
#include <thread>

/*
 * The few bits of the Arduino core that apps and deskkit use, for the
 * desktop build, where there is no Arduino. Only that build has this folder on
 * its include path (see boards/desktop.ini). String is a std::string that can
 * be built from numbers like Arduino's.
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
