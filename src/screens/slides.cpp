#include "slides.h"

#define SLIDE_MS 60000  // how long a slide stays

// &WEIGHT_SLIDE, the weekly averages as rows of bars, is off for now (2026-10-30)
static const deskkit::Slide *const SLIDES[] = {&DASHBOARD_SLIDE, &SLEEP_SLIDE, &WEIGHT_RANGE_SLIDE};

deskkit::SlideShow slideShow(SLIDES, SLIDE_MS);
