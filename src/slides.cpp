#include "slides.h"
#include "health.h"
#include "network.h"
#include "slide.h"

/*
 * The slides and the turns they take. Each slide lives in its own slide_*.cpp;
 * the notice bar is drawn on top of whichever is showing.
 */

const uint32_t NOTICE_ORANGE = 0xEF9F27;

#define SLIDE_MS 60000          // how long a slide stays
#define NOTICE_HEIGHT 20        // a bar across the top, above the first row's text

const Slide *const SLIDES[] = {&DASHBOARD_SLIDE, &SLEEP_SLIDE, &WEIGHT_SLIDE};
const int SLIDE_COUNT = sizeof(SLIDES) / sizeof(SLIDES[0]);

static void showSlide(int index);
static void drawNotice(const char *notice);

static int slide = 0;
static uint32_t slideShownAt = 0;

// MARK: Taking turns

void showSlides(time_t now, const char *notice) {
  bool isBackFromSession = !shownFrame().startsWith("slide");
  if (isBackFromSession) showSlide(0);
  else if (millis() - slideShownAt >= SLIDE_MS) nextSlide();

  const Slide &shown = *SLIDES[slide];
  if (!isNewFrame("slide " + String(slide) + " " + notice + " " + shown.key(now))) return;
  shown.draw(now);
  drawNotice(notice);
  pushFrame();
}

void nextSlide() { showSlide((slide + 1) % SLIDE_COUNT); }

bool isLastSlide() { return slide == SLIDE_COUNT - 1; }

static void showSlide(int index) {
  slide = index;
  slideShownAt = millis();
}

static void drawNotice(const char *notice) {
  if (!*notice) return;
  frame.fillRect(0, 0, frame.width(), NOTICE_HEIGHT, NOTICE_ORANGE);
  drawText(notice, CENTER_X, NOTICE_HEIGHT / 2, {&fonts::FreeSans9pt7b, BACKGROUND}, middle_center);
}

// MARK: For the slides

String weekLabel(time_t now, int week) {
  if (!isTimeSynced()) return "";
  time_t then = now - week * 7 * 86400;
  struct tm local;
  localtime_r(&then, &local);
  char number[4];
  strftime(number, sizeof(number), "%V", &local);
  return "W" + String(atoi(number));
}

String hoursAndMinutes(int minutes) {
  if (minutes == HEALTH_MISSING) return "--";
  char text[12];
  snprintf(text, sizeof(text), "%d:%02d", minutes / 60, minutes % 60);
  return text;
}
