#pragma once

/*
 * Debug commands over serial: USB on the device, the terminal in the
 * simulator. On only in builds with DEBUG_TOOLS. Each command is one letter,
 * answered with "OK <letter>", or "WAIT" until the clock is synced. "s" is
 * built in: it sends "SHOT <width> <height>" and then the screen as rows of R,
 * G, B bytes. tools/screenshot.py speaks this.
 */

namespace deskkit {

// Apps add theirs in setup(), like a fake state to take a screenshot of.
// Without DEBUG_TOOLS they are never run.
void addCommand(char letter, void (*run)());

// MARK: For begin() and update() in app.h

void startConsole();
void updateConsole();

}  // namespace deskkit
