#pragma once
#include <lvgl.h>
namespace UiTheme {
inline lv_color_t background() { return lv_color_hex(0x000203); }
inline lv_color_t panel() { return background(); }
inline lv_color_t border() { return lv_color_hex(0x45677C); }
inline lv_color_t text() { return lv_color_hex(0xF8FBFF); }
inline lv_color_t muted() { return lv_color_hex(0xA7BED4); }
inline lv_color_t blue() { return lv_color_hex(0x00D5F4); }
inline lv_color_t green() { return lv_color_hex(0x00EB58); }
inline lv_color_t yellow() { return lv_color_hex(0xEEEF00); }
inline lv_color_t orange() { return lv_color_hex(0xFF9200); }
inline lv_color_t red() { return lv_color_hex(0xFF2020); }
}  // namespace UiTheme
