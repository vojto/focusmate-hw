#include "slide.h"
#include "weight.h"

/*
 * A column per week, this week on the right: a faded line from the week's
 * lightest weigh-in to its heaviest, a faded bar from the week before's average
 * to this week's (at least CHANGE_MIN_HEIGHT tall) with a line in full color at
 * this week's end, and the average above. Colored by the change from the week before, green for
 * a loss of CHANGE_RANGE_GRAMS or more to red for a gain of as much. This
 * week's change ("-0.3") is large in the first corner that has room. See
 * tools/sketch/weight-columns.html and weight-edge.html.
 */

// The sleep colors from green to red in OKLCH (lightness, chroma, hue), blended
// in OKLCH. See tools/sketch/weight-oklch.html.
const float RANGE_COLORS[][3] = {
    {0.8005f, 0.2000f, 145.0f}, {0.8594f, 0.1891f, 127.8f}, {0.8802f, 0.1699f, 94.9f}, {0.7602f, 0.1605f, 52.0f}, {0.6807f, 0.2004f, 25.1f},
};
const int RANGE_COLOR_COUNT = sizeof(RANGE_COLORS) / sizeof(RANGE_COLORS[0]);

#define PADDING 12
#define COLUMN_GAP 4
#define AXIS_HEIGHT 22          // the week labels under the columns
#define VALUE_GAP 5             // between the columns and the text
#define CHANGE_MIN_HEIGHT 4     // so a week without change still shows
#define CHANGE_INSET 3          // on each side of the change bar
#define ARROW_WIDTH 10          // the chevron inside a bar, pointing the way the weight went
#define ARROW_HEIGHT 5
#define ARROW_ROOM 15           // the shortest bar that gets one
#define ARROW_RADIUS 0.75f
#define RANGE_RADIUS 0.75f      // half the range line's width
#define RANGE_LIGHTNESS 0.5f    // the range line's OKLCH lightness, the same for every color
#define BAR_LIGHTNESS 0.36f     // and the change bar's
#define RANGE_FADE 0.5f         // how much of gray is left in the range line, when there is no change
#define BAR_FADE 0.4f           // and in the bar
#define EDGE_HEIGHT 2           // the line at this week's end of the bar
#define CHANGE_RANGE_GRAMS 500  // a week's change that is all green or all red
#define SCALE_STEP_GRAMS 500    // the scale's ends are rounded out to this
#define TEXT_HEIGHT 16          // Font2's

#define CHANGE_TEXT_HEIGHT 25  // FreeSans18pt7b's digits

// Font2: "107.4" is 37px, which fits a 33.5px column and its gap; FreeSans9pt7b is 45px
const TextStyle WEEK_TEXT = {&fonts::Font2, TEXT_GRAY};
const lgfx::IFont *const AVERAGE_FONT = &fonts::Font2;
const lgfx::IFont *const CHANGE_FONT = &fonts::FreeSans18pt7b;

// What a column covers, from the top of its text to the bottom of its lines
struct Span {
  int top, bottom;
};
const Span NO_SPAN = {INT_MAX, INT_MIN};

// Grams to y: lowest at the bottom of the columns, highest under the text
struct Scale {
  int lowest, highest;
  float y(int grams) const {
    float bottom = frame.height() - AXIS_HEIGHT - VALUE_GAP, top = PADDING + TEXT_HEIGHT + VALUE_GAP;
    return bottom - (grams - lowest) * (bottom - top) / (highest - lowest);
  }
};

static String rangeKey(time_t now);
static void drawRanges(time_t now);
static Span drawColumn(float x, int week, const Weight &weight, const Scale &scale, time_t now);
static void drawChange(const Weight &weight, const Span spans[]);
static bool hasRoom(int left, int right, int top, int bottom, const Span spans[]);
static float columnX(int week);
static float columnWidth();
static Scale scaleOf(const Weight &weight);
static uint32_t changeColor(const Weight &weight, int week, float lightness = 0, float grayFade = 1);
static int gramsBefore(const Weight &weight, int week);
static void drawArrow(float x, float top, float bottom, bool isUp, uint32_t color);
static String kilograms(int grams);
static uint32_t blend(uint32_t from, uint32_t to, float amount);

const Slide WEIGHT_RANGE_SLIDE = {
    rangeKey,
    drawRanges,
};

// Changes with the numbers and the week
static String rangeKey(time_t now) { return keyOf(latestWeight()) + weekLabel(now, 0); }

static void drawRanges(time_t now) {
  Weight weight = latestWeight();
  Scale scale = scaleOf(weight);
  Span spans[WEIGHT_WEEKS];
  for (int week = 0; week < WEIGHT_WEEKS; week++) spans[week] = drawColumn(columnX(week), week, weight, scale, now);
  drawChange(weight, spans);
}

// x is the column's middle
static Span drawColumn(float x, int week, const Weight &weight, const Scale &scale, time_t now) {
  drawText(weekLabel(now, week), lroundf(x), frame.height() - AXIS_HEIGHT / 2, WEEK_TEXT, middle_center);
  int grams = weight.weeklyGrams[week];
  if (grams == WEIGHT_MISSING || weight.lightestGrams[week] == WEIGHT_MISSING) return NO_SPAN;

  uint32_t color = changeColor(weight, week);
  float top = scale.y(weight.heaviestGrams[week]);
  frame.drawWideLine(x, top, x, scale.y(weight.lightestGrams[week]), RANGE_RADIUS, changeColor(weight, week, RANGE_LIGHTNESS, RANGE_FADE));
  int before = gramsBefore(weight, week) == WEIGHT_MISSING ? grams : gramsBefore(weight, week);
  float barTop = min(scale.y(grams), scale.y(before)), barBottom = max(scale.y(grams), scale.y(before));
  float growth = max(0.0f, CHANGE_MIN_HEIGHT - (barBottom - barTop)) / 2;
  int left = lroundf(x - columnWidth() / 2 + CHANGE_INSET), right = lroundf(x + columnWidth() / 2 - CHANGE_INSET);
  int y = lroundf(barTop - growth);
  int height = lroundf(barBottom + growth) - y;
  frame.fillRect(left, y, right - left, height, changeColor(weight, week, BAR_LIGHTNESS, BAR_FADE));
  frame.fillRect(left, grams >= before ? y : y + height - EDGE_HEIGHT, right - left, EDGE_HEIGHT, color);
  if (grams != before && barBottom - barTop >= ARROW_ROOM) drawArrow(x, barTop, barBottom, grams > before, color);
  int textBottom = min((int)lroundf(top), y) - VALUE_GAP;
  drawText(kilograms(grams), lroundf(x), textBottom, {AVERAGE_FONT, color}, bottom_center);
  int linesBottom = max(lroundf(scale.y(weight.lightestGrams[week])), lroundf(barBottom + growth));
  return {textBottom - TEXT_HEIGHT, linesBottom};
}

// This week's change in the first free corner, right ones first, as this
// week is on the right; nowhere when none is free
static void drawChange(const Weight &weight, const Span spans[]) {
  if (weight.weeklyGrams[0] == WEIGHT_MISSING || gramsBefore(weight, 0) == WEIGHT_MISSING) return;
  int tenths = (weight.weeklyGrams[0] + 50) / 100 - (gramsBefore(weight, 0) + 50) / 100;
  String text = (tenths > 0 ? "+" : tenths < 0 ? "-" : "") + kilograms(abs(tenths) * 100) + " kg";
  frame.setFont(CHANGE_FONT);
  int width = frame.textWidth(text);
  int top = PADDING, bottom = frame.height() - AXIS_HEIGHT - VALUE_GAP;
  for (bool isRight : {true, false}) {
    int left = isRight ? frame.width() - PADDING - width : PADDING;
    for (int baseline : {top + CHANGE_TEXT_HEIGHT, bottom}) {
      if (!hasRoom(left, left + width, baseline - CHANGE_TEXT_HEIGHT, baseline, spans)) continue;
      drawText(text, left, baseline, {CHANGE_FONT, changeColor(weight, 0)}, baseline_left);
      return;
    }
  }
}

// Whether the box keeps VALUE_GAP away from every column it is over
static bool hasRoom(int left, int right, int top, int bottom, const Span spans[]) {
  for (int week = 0; week < WEIGHT_WEEKS; week++) {
    float x = columnX(week);
    if (x + columnWidth() / 2 < left || x - columnWidth() / 2 > right) continue;
    if (spans[week].top < bottom + VALUE_GAP && spans[week].bottom > top - VALUE_GAP) return false;
  }
  return true;
}

// The column's middle, this week on the right
static float columnX(int week) {
  int column = WEIGHT_WEEKS - 1 - week;
  return PADDING + column * (columnWidth() + COLUMN_GAP) + columnWidth() / 2;
}

// A chevron in the middle of the bar
static void drawArrow(float x, float top, float bottom, bool isUp, uint32_t color) {
  float middle = (top + bottom) / 2, tip = middle + (isUp ? -ARROW_HEIGHT : ARROW_HEIGHT) / 2.0f;
  float ends = middle + (isUp ? ARROW_HEIGHT : -ARROW_HEIGHT) / 2.0f;
  frame.drawWideLine(x - ARROW_WIDTH / 2.0f, ends, x, tip, ARROW_RADIUS, color);
  frame.drawWideLine(x, tip, x + ARROW_WIDTH / 2.0f, ends, ARROW_RADIUS, color);
}

// The gap between columns is set; their width is what is left of the screen
static float columnWidth() { return (frame.width() - 2 * PADDING - (WEIGHT_WEEKS - 1) * COLUMN_GAP) / (float)WEIGHT_WEEKS; }

// From the lightest weigh-in to the heaviest, rounded out to half kilos
static Scale scaleOf(const Weight &weight) {
  int lightest = INT_MAX, heaviest = 0;
  for (int week = 0; week < WEIGHT_WEEKS; week++) {
    if (weight.lightestGrams[week] == WEIGHT_MISSING) continue;
    lightest = min(lightest, weight.lightestGrams[week]);
    heaviest = max(heaviest, weight.heaviestGrams[week]);
  }
  if (heaviest == 0) return {0, SCALE_STEP_GRAMS};
  // The oldest week's bar starts at the week before it
  if (weight.earlierGrams != WEIGHT_MISSING) {
    lightest = min(lightest, weight.earlierGrams);
    heaviest = max(heaviest, weight.earlierGrams);
  }
  return {lightest / SCALE_STEP_GRAMS * SCALE_STEP_GRAMS, (heaviest / SCALE_STEP_GRAMS + 1) * SCALE_STEP_GRAMS};
}

// At its own lightness, or at the given one with chroma lowered to match, so
// every faded color is as bright as the others. Gray, faded by grayFade, when
// there is no week before to compare with.
static uint32_t changeColor(const Weight &weight, int week, float lightness, float grayFade) {
  int before = gramsBefore(weight, week);
  if (before == WEIGHT_MISSING) return blend(BACKGROUND, TEXT_GRAY, grayFade);
  float amount = constrain((weight.weeklyGrams[week] - before) / (2.0f * CHANGE_RANGE_GRAMS) + 0.5f, 0.0f, 1.0f);
  float position = amount * (RANGE_COLOR_COUNT - 1);
  int from = min((int)position, RANGE_COLOR_COUNT - 2);
  float lch[3];
  for (int i = 0; i < 3; i++) lch[i] = RANGE_COLORS[from][i] + (RANGE_COLORS[from + 1][i] - RANGE_COLORS[from][i]) * (position - from);
  if (lightness == 0) return oklchToRgb(lch[0], lch[1], lch[2]);
  return oklchToRgb(lightness, lch[1] * lightness / lch[0], lch[2]);
}

// The average of the week before, or WEIGHT_MISSING
static int gramsBefore(const Weight &weight, int week) {
  return week + 1 < WEIGHT_WEEKS ? weight.weeklyGrams[week + 1] : weight.earlierGrams;
}

// "107.4"
static String kilograms(int grams) {
  int tenths = (grams + 50) / 100;
  return String(tenths / 10) + "." + String(tenths % 10);
}

// amount 0 is from, 1 is to
static uint32_t blend(uint32_t from, uint32_t to, float amount) {
  uint32_t color = 0;
  for (int shift : {16, 8, 0}) {
    int a = from >> shift & 0xFF, b = to >> shift & 0xFF;
    color |= (uint32_t)lroundf(a + (b - a) * amount) << shift;
  }
  return color;
}
