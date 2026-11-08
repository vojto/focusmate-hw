#include "slides.h"

#define KEY_PREFIX "slide"      // a frame key starting with this is one of a slide show's
#define NOTICE_HEIGHT 20        // a bar across the top, above the first row's text

namespace deskkit {

static void drawNotice(const char *notice);

void SlideShow::show(time_t now, const char *notice) {
  bool isBackFromOtherScreen = !shownFrame().startsWith(KEY_PREFIX);
  if (isBackFromOtherScreen) showSlide(0);
  else if (millis() - shownAt >= slideMs) next();

  const Slide &slide = *slides[shown];
  if (!isNewFrame(KEY_PREFIX " " + String(shown) + " " + notice + " " + slide.key(now))) return;
  slide.draw(now);
  drawNotice(notice);
  pushFrame();
}

void SlideShow::next() { showSlide((shown + 1) % count); }

bool SlideShow::isLast() const { return shown == count - 1; }

void SlideShow::showSlide(int index) {
  shown = index;
  shownAt = millis();
}

static void drawNotice(const char *notice) {
  if (!*notice) return;
  frame.fillRect(0, 0, frame.width(), NOTICE_HEIGHT, NOTICE_ORANGE);
  drawText(notice, frame.width() / 2, NOTICE_HEIGHT / 2, {&fonts::FreeSans9pt7b, BACKGROUND}, middle_center);
}

}  // namespace deskkit
