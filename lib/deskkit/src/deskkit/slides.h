#pragma once
#include "display.h"

/*
 * Screens that take turns: a SlideShow moves to the next slide every so
 * often, or sooner when the app calls next() (on a tap, say). When the app
 * showed another screen in between, it starts again from the first. A notice,
 * when there is one, is an orange bar across the top of every slide.
 */

namespace deskkit {

struct Slide {
  String (*key)(time_t now);   // everything it shows; see isNewFrame in display.h
  void (*draw)(time_t now);    // into the frame, which is already cleared
};

class SlideShow {
 public:
  template <int N>
  SlideShow(const Slide *const (&slides)[N], uint32_t slideMs) : slides(slides), count(N), slideMs(slideMs) {}

  // Can be called on every loop: draws only when the slide or what it shows changed
  void show(time_t now, const char *notice = "");

  // Moves on to the next slide, which then stays a whole turn
  void next();

  bool isLast() const;

 private:
  void showSlide(int index);

  const Slide *const *slides;
  int count;
  uint32_t slideMs;
  int shown = 0;
  uint32_t shownAt = 0;
};

}  // namespace deskkit
