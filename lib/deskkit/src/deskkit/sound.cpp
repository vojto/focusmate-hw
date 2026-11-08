#include "sound.h"
#include <Arduino.h>
#include <M5Unified.h>

#define NOTE_MS 140
#define NOTE_STEP_MS 160  // from one note's start to the next one's

namespace deskkit {

void playNotes(std::initializer_list<int> frequencies) {
  for (int frequency : frequencies) {
    M5.Speaker.tone(frequency, NOTE_MS);
    delay(NOTE_STEP_MS);
  }
}

}  // namespace deskkit
