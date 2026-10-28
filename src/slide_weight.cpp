#include "slide.h"
#include "weight.h"

/*
 * The average weight of this week and the seven before it, a row each, as
 * bars of half-kilo segments that start a kilo under the lightest week (from
 * zero they would all look the same), colored from green for the lightest to
 * red for the heaviest. This week's average is followed by a chevron and
 * today's weigh-in. See tools/sketch/weight-bars.html.
 */

// The sleep colors from green to red
const uint32_t WEIGHT_COLORS[] = {0x59DE65, 0xADE74E, 0xFBD530, 0xFF9044, 0xFC5855};
const int WEIGHT_COLOR_COUNT = sizeof(WEIGHT_COLORS) / sizeof(WEIGHT_COLORS[0]);

#define ROW_HEIGHT 30           // 240 / WEIGHT_WEEKS
#define BAR_HEIGHT 26
#define PADDING 12
#define BAR_LEFT 56             // after the week labels
#define VALUE_GAP 8             // between a bar and its weight
#define SEGMENT_GRAMS 500
#define SEGMENT_GAP 2
#define SEGMENT_RADIUS 3
#define CHEVRON_SIZE 4          // half its height
#define CHEVRON_GAP 7           // on each side of it

const TextStyle LABEL_TEXT = {&fonts::FreeSans9pt7b, TEXT_GRAY};
const lgfx::IFont *const VALUE_FONT = &fonts::FreeSans12pt7b;

// The bars' scale: start and end in grams, and the x of the end
struct Scale {
  int start, end, right;
  int x(int grams) const { return BAR_LEFT + (int)lroundf((grams - start) * (right - BAR_LEFT) / (float)(end - start)); }
};

static String weightKey(time_t now);
static void drawWeight(time_t now);
static void drawRow(int row, int grams, int todayGrams, const Scale &scale, int lightest, int heaviest, time_t now);
static int drawBar(int y, int grams, const Scale &scale, uint32_t color);
static int drawValue(int x, int y, int grams, uint32_t color);
static void drawChevron(int x, int y);
static int roundedToTenth(int grams);
static String kilograms(int grams);
static uint32_t weightColor(int grams, int lightest, int heaviest);
static uint32_t blend(uint32_t from, uint32_t to, float amount);

const Slide WEIGHT_SLIDE = {
    weightKey,
    drawWeight,
};

// Changes with the numbers and the week
static String weightKey(time_t now) { return keyOf(latestWeight()) + weekLabel(now, 0); }

static void drawWeight(time_t now) {
  Weight weight = latestWeight();
  int lightest = INT_MAX, heaviest = 0;
  for (int grams : weight.weeklyGrams) {
    if (grams == WEIGHT_MISSING) continue;
    lightest = min(lightest, grams);
    heaviest = max(heaviest, grams);
  }
  frame.setFont(VALUE_FONT);
  Scale scale = {(lightest / 1000 - 1) * 1000, (heaviest + 999) / 1000 * 1000,
                 frame.width() - PADDING - VALUE_GAP - frame.textWidth("109.0")};
  for (int week = 0; week < WEIGHT_WEEKS; week++) {
    drawRow(week, weight.weeklyGrams[week], week == 0 ? weight.todayGrams : WEIGHT_MISSING, scale, lightest, heaviest, now);
  }
}

// The week's label, bar and weight. With today's weigh-in, the week's weight
// turns gray and today's follows it after a chevron.
static void drawRow(int row, int grams, int todayGrams, const Scale &scale, int lightest, int heaviest, time_t now) {
  int y = row * ROW_HEIGHT + ROW_HEIGHT / 2;
  drawText(weekLabel(now, row), PADDING, y, LABEL_TEXT, middle_left);
  int x = grams == WEIGHT_MISSING ? BAR_LEFT : drawBar(y, grams, scale, weightColor(grams, lightest, heaviest));
  if (todayGrams == WEIGHT_MISSING) {
    drawValue(x, y, grams, TEXT_WHITE);
    return;
  }
  x = drawValue(x, y, grams, TEXT_GRAY) + CHEVRON_GAP;
  drawChevron(x, y);
  drawValue(x + CHEVRON_SIZE + CHEVRON_GAP - VALUE_GAP, y, todayGrams, TEXT_WHITE);
}

// Half-kilo segments, the last one cut at the weight rounded to 100 g.
// Returns where the bar ends.
static int drawBar(int y, int grams, const Scale &scale, uint32_t color) {
  int end = roundedToTenth(grams);
  for (int from = scale.start; from < end; from += SEGMENT_GRAMS) {
    int left = scale.x(from) + SEGMENT_GAP / 2;
    int width = max(scale.x(min(from + SEGMENT_GRAMS, end)) - left - SEGMENT_GAP / 2, 1);
    frame.fillSmoothRoundRect(left, y - BAR_HEIGHT / 2, width, BAR_HEIGHT, SEGMENT_RADIUS, color);
  }
  return scale.x(end);
}

// After a gap; returns where the text ends
static int drawValue(int x, int y, int grams, uint32_t color) {
  x += VALUE_GAP;
  return x + drawText(kilograms(grams), x, y, {VALUE_FONT, color}, middle_left);
}

static void drawChevron(int x, int y) {
  frame.drawWideLine(x, y - CHEVRON_SIZE, x + CHEVRON_SIZE, y, 1, TEXT_GRAY);
  frame.drawWideLine(x + CHEVRON_SIZE, y, x, y + CHEVRON_SIZE, 1, TEXT_GRAY);
}

static int roundedToTenth(int grams) { return (grams + 50) / 100 * 100; }

// "107.1", rounded like the bar's end
static String kilograms(int grams) {
  if (grams == WEIGHT_MISSING) return "--";
  int tenths = roundedToTenth(grams) / 100;
  return String(tenths / 10) + "." + String(tenths % 10);
}

static uint32_t weightColor(int grams, int lightest, int heaviest) {
  if (heaviest == lightest) return WEIGHT_COLORS[0];
  float position = (grams - lightest) / (float)(heaviest - lightest) * (WEIGHT_COLOR_COUNT - 1);
  int from = min((int)position, WEIGHT_COLOR_COUNT - 2);
  return blend(WEIGHT_COLORS[from], WEIGHT_COLORS[from + 1], position - from);
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
