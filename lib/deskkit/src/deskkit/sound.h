#pragma once
#include <initializer_list>

/*
 * Short tunes on the device's speaker, or the computer's in the simulator.
 */

namespace deskkit {

// Notes in Hz, one after another; returns when they are done
void playNotes(std::initializer_list<int> frequencies);

}  // namespace deskkit
