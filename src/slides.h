#pragma once
#include <time.h>

/*
 * What the screen shows between sessions: slides that take turns every
 * minute, or sooner on a tap. Coming back from a session starts again from the
 * first, and so does a tap on the countdown before a session. A notice, when there is one, is an orange bar across the top of every
 * slide.
 */

// Can be called on every loop, like the show functions in screens.h
void showSlides(time_t now, const char *notice);

// Moves on to the next slide, which then stays for a whole minute
void nextSlide();
