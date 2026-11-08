#include "widgets.h"

/*
 * The Rotation's rules. The first urgent widget in the list is shown while it
 * is urgent. Otherwise the normal widgets take turns in the list's order,
 * starting from the first whenever an urgent one was shown in between. Around
 * an urgent widget, a tap shows the first normal one, more taps go through
 * the rest, and a tap on the last brings the urgent one back, as does PEEK_MS
 * without a tap.
 */

#define PEEK_MS 10000       // an urgent widget comes back this long after the last tap
#define NOTICE_HEIGHT 20    // a bar across the top, above the first row's text

namespace deskkit {

static Priority priorityOf(const Widget &widget, time_t now);
static void drawNotice(const char *notice);

// MARK: Public

void Rotation::show(time_t now, const char *notice) {
  if (!hasStarted) startAll();
  updateAll(now);
  urgent = firstWith(URGENT, now);
  if (wasTapped) handleTap(now);
  if (urgent < 0 || millis() - tappedAt >= PEEK_MS) isPeeking = false;

  if (urgent >= 0 && !isPeeking) {
    turn = -1;
    draw(urgent, now, "");
    return;
  }
  pickTurn(now);
  if (turn >= 0) draw(turn, now, notice);
}

void Rotation::tap() { wasTapped = true; }

// MARK: Turns

void Rotation::handleTap(time_t now) {
  wasTapped = false;
  tappedAt = millis();
  if (urgent >= 0 && !isPeeking) isPeeking = true;
  else if (urgent >= 0 && isLastTurn(now)) isPeeking = false;
  else next(now);
}

// Starts from the first when nothing had a turn or the one that had it is no
// longer normal; moves on when its turn is over
void Rotation::pickTurn(time_t now) {
  bool hasTurn = turn >= 0 && priorityOf(*widgets[turn], now) == NORMAL;
  if (!hasTurn) startTurn(firstWith(NORMAL, now));
  else if (millis() - turnAt >= turnMs) next(now);
}

void Rotation::next(time_t now) {
  int following = firstWith(NORMAL, now, turn + 1);
  startTurn(following >= 0 ? following : firstWith(NORMAL, now));
}

void Rotation::startTurn(int index) {
  turn = index;
  turnAt = millis();
}

bool Rotation::isLastTurn(time_t now) const { return firstWith(NORMAL, now, turn + 1) < 0; }

// MARK: Widgets

void Rotation::startAll() {
  hasStarted = true;
  for (int i = 0; i < count; i++)
    if (widgets[i]->start) widgets[i]->start();
}

void Rotation::updateAll(time_t now) {
  for (int i = 0; i < count; i++)
    if (widgets[i]->update) widgets[i]->update(now);
}

int Rotation::firstWith(Priority priority, time_t now, int from) const {
  for (int i = from; i < count; i++)
    if (priorityOf(*widgets[i], now) == priority) return i;
  return -1;
}

// Draws only when the widget or what it shows changed
void Rotation::draw(int index, time_t now, const char *notice) {
  const Widget &widget = *widgets[index];
  if (!isNewFrame("widget " + String(index) + " " + notice + " " + widget.key(now))) return;
  widget.draw(now);
  drawNotice(notice);
  pushFrame();
}

static Priority priorityOf(const Widget &widget, time_t now) { return widget.priority ? widget.priority(now) : NORMAL; }

static void drawNotice(const char *notice) {
  if (!*notice) return;
  frame.fillRect(0, 0, frame.width(), NOTICE_HEIGHT, NOTICE_ORANGE);
  drawText(notice, frame.width() / 2, NOTICE_HEIGHT / 2, {&fonts::FreeSans9pt7b, BACKGROUND}, middle_center);
}

}  // namespace deskkit
