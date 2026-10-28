#pragma once
#include "display.h"

/*
 * What a slide is, for slides.cpp and the slide_*.cpp files: a key of what it
 * shows (see isNewFrame in display.h) and a function that draws it into the
 * frame. To add one, write a slide_*.cpp and list it in SLIDES in slides.cpp.
 */

struct Slide {
  String (*key)(time_t now);
  void (*draw)(time_t now);
};

extern const Slide DASHBOARD_SLIDE, SLEEP_SLIDE, WEIGHT_SLIDE;

// The ISO week number: "W43". 0 is this week, 1 the week before.
String weekLabel(time_t now, int week);

// "7:24"; "--" for a missing number
String hoursAndMinutes(int minutes);
