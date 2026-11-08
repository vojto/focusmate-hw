#pragma once
#include <stdint.h>

/*
 * The colors deskkit's own widgets draw with, on a black screen. Apps use
 * them too, so their screens match. Colors are 0xRRGGBB: M5GFX reads a
 * uint32_t that way, but an int or uint16_t as RGB565, so keep the type.
 */

namespace deskkit {

const uint32_t BACKGROUND = 0x000000;
const uint32_t TEXT_WHITE = 0xF1EFE8;
const uint32_t TEXT_GRAY = 0xB4B2A9;
const uint32_t DIVIDER_GRAY = 0x444441;  // the grid's lines
const uint32_t NOTICE_ORANGE = 0xEF9F27; // the slide show's notice bar

}  // namespace deskkit
