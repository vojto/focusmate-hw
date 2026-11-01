#include "slide.h"
#include "weight.h"

/*
 * A column per week, this week on the right: a faded line from the week's
 * lightest weigh-in to its heaviest, a bar from the week before's average to
 * this week's (at least CHANGE_MIN_HEIGHT tall), and the average above. Colored by the change from the week before, green for
 * a loss of CHANGE_RANGE_GRAMS or more to red for a gain of as much. See
 * tools/sketch/weight-columns.html.
 */

// The sleep colors from green to red
const uint32_t RANGE_COLORS[] = {0x59DE65, 0xADE74E, 0xFBD530, 0xFF9044, 0xFC5855};
const int RANGE_COLOR_COUNT = sizeof(RANGE_COLORS) / sizeof(RANGE_COLORS[0]);

#define PADDING 12
#define COLUMN_GAP 4
#define AXIS_HEIGHT 22          // the week labels under the columns
#define VALUE_GAP 5             // between the columns and the text
#define CHANGE_MIN_HEIGHT 4     // so a week without change still shows
#define CHANGE_INSET 3          // on each side of the change bar
#define RANGE_RADIUS 0.75f      // half the range line's width
#define RANGE_FADE 0.5f         // how much of the range line's color is left
#define CHANGE_RANGE_GRAMS 500  // a week's change that is all green or all red
#define SCALE_STEP_GRAMS 500    // the scale's ends are rounded out to this
#define TEXT_HEIGHT 16          // Font2's

// Font2: "107.4" is 37px, which fits a 33.5px column and its gap; FreeSans9pt7b is 45px
const TextStyle WEEK_TEXT = {&fonts::Font2, TEXT_GRAY};
const lgfx::IFont *const AVERAGE_FONT = &fonts::Font2;

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
static void drawColumn(float x, int week, const Weight &weight, const Scale &scale, time_t now);
static float columnWidth();
static Scale scaleOf(const Weight &weight);
static uint32_t changeColor(const Weight &weight, int week);
static int gramsBefore(const Weight &weight, int week);
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
  for (int week = 0; week < WEIGHT_WEEKS; week++) {
    int column = WEIGHT_WEEKS - 1 - week;
    drawColumn(PADDING + column * (columnWidth() + COLUMN_GAP) + columnWidth() / 2, week, weight, scale, now);
  }
}

// x is the column's middle
static void drawColumn(float x, int week, const Weight &weight, const Scale &scale, time_t now) {
  drawText(weekLabel(now, week), lroundf(x), frame.height() - AXIS_HEIGHT / 2, WEEK_TEXT, middle_center);
  int grams = weight.weeklyGrams[week];
  if (grams == WEIGHT_MISSING || weight.lightestGrams[week] == WEIGHT_MISSING) return;

  uint32_t color = changeColor(weight, week);
  float top = scale.y(weight.heaviestGrams[week]);
  frame.drawWideLine(x, top, x, scale.y(weight.lightestGrams[week]), RANGE_RADIUS, blend(BACKGROUND, color, RANGE_FADE));
  int before = gramsBefore(weight, week) == WEIGHT_MISSING ? grams : gramsBefore(weight, week);
  float barTop = min(scale.y(grams), scale.y(before)), barBottom = max(scale.y(grams), scale.y(before));
  float growth = max(0.0f, CHANGE_MIN_HEIGHT - (barBottom - barTop)) / 2;
  int left = lroundf(x - columnWidth() / 2 + CHANGE_INSET), right = lroundf(x + columnWidth() / 2 - CHANGE_INSET);
  int y = lroundf(barTop - growth);
  frame.fillRect(left, y, right - left, lroundf(barBottom + growth) - y, color);
  drawText(kilograms(grams), lroundf(x), min((int)lroundf(top), y) - VALUE_GAP, {AVERAGE_FONT, color}, bottom_center);
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

// Gray when there is no week before to compare with
static uint32_t changeColor(const Weight &weight, int week) {
  int before = gramsBefore(weight, week);
  if (before == WEIGHT_MISSING) return TEXT_GRAY;
  float amount = constrain((weight.weeklyGrams[week] - before) / (2.0f * CHANGE_RANGE_GRAMS) + 0.5f, 0.0f, 1.0f);
  float position = amount * (RANGE_COLOR_COUNT - 1);
  int from = min((int)position, RANGE_COLOR_COUNT - 2);
  return blend(RANGE_COLORS[from], RANGE_COLORS[from + 1], position - from);
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
