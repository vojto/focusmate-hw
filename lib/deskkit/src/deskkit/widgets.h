#pragma once
#include "display.h"

/*
 * Widgets, and a Rotation that shows them one at a time. A widget is one thing
 * on the screen (a clock, a chart, a timer), drawn into the frame, which is
 * one widget's size (display.h). Apps pick widgets from a shared set and list
 * them in a Rotation.
 *
 * Each widget says how much it matters right now. Normal widgets take turns,
 * moving on every so often or on a tap. An urgent widget stops the turns and
 * stays until it isn't urgent any more; a tap lets the user peek at the
 * others for a while. A hidden widget has nothing to show and is skipped.
 */

namespace deskkit {

enum Priority { HIDDEN, NORMAL, URGENT };

// Leave out the trailing functions a widget doesn't need
struct Widget {
  String (*key)(time_t now);       // everything it shows; see isNewFrame in display.h
  void (*draw)(time_t now);        // into the frame, which is already cleared
  void (*start)();                 // once, before anything else, like starting its fetch
  Priority (*priority)(time_t now);  // left out, always NORMAL
  void (*update)(time_t now);      // every loop, shown or not, like playing a chime
};

class Rotation {
 public:
  template <int N>
  Rotation(const Widget *const (&widgets)[N], uint32_t turnMs) : widgets(widgets), count(N), turnMs(turnMs) {}

  // Call on every loop: updates every widget, picks the one to show and draws
  // it if it changed. The notice, when there is one, is an orange bar across
  // the top of the widgets taking turns.
  void show(time_t now, const char *notice = "");

  // Moves on to the next widget, or peeks past an urgent one
  void tap();

  // Whether a widget was urgent at the last show()
  bool isUrgent() const { return urgent >= 0; }

 private:
  void handleTap(time_t now);
  void pickTurn(time_t now);
  void next(time_t now);
  void startTurn(int index);
  bool isLastTurn(time_t now) const;
  void startAll();
  void updateAll(time_t now);
  int firstWith(Priority priority, time_t now, int from = 0) const;
  void draw(int index, time_t now, const char *notice);

  const Widget *const *widgets;
  int count;
  uint32_t turnMs;
  bool hasStarted = false;
  int urgent = -1;            // the urgent widget, or -1
  int turn = -1;              // the normal widget whose turn it is, or -1
  uint32_t turnAt = 0;
  bool isPeeking = false;
  bool wasTapped = false;
  uint32_t tappedAt = 0;
};

}  // namespace deskkit
