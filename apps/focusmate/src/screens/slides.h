#pragma once
#include <deskkit/slides.h>

/*
 * What the screen shows between sessions, or around one after a tap: the app's
 * slides, which take turns every minute (see deskkit/slides.h). Each slide is
 * its own slide_*.cpp; to add one, write the file, declare it here and list it
 * in slides.cpp.
 */

extern const deskkit::Slide DASHBOARD_SLIDE, SLEEP_SLIDE, WEIGHT_SLIDE, WEIGHT_RANGE_SLIDE;

extern deskkit::SlideShow slideShow;
