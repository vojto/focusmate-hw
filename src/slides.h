#pragma once
#include <time.h>

/*
 * What the screen shows between sessions, or around one after a tap: slides
 * that take turns every minute, or sooner on a tap (see main.cpp). Coming back
 * from a session screen starts again from the first. A notice, when there is
 * one, is an orange bar across the top of every slide.
 */

// Can be called on every loop, like the show functions in screens.h
void showSlides(time_t now, const char *notice);

// Moves on to the next slide, which then stays for a whole minute
void nextSlide();

bool isLastSlide();
