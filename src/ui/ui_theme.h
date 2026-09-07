#pragma once
#include <lvgl.h>
namespace UiTheme {
inline lv_color_t background() { return lv_color_hex(0x07090D); }
inline lv_color_t panel() { return lv_color_hex(0x11161D); }
inline lv_color_t border() { return lv_color_hex(0x27313C); }
inline lv_color_t text() { return lv_color_hex(0xF2F5F7); }
inline lv_color_t muted() { return lv_color_hex(0x7D8996); }
inline lv_color_t blue() { return lv_color_hex(0x20A4F3); }
inline lv_color_t green() { return lv_color_hex(0x39D353); }
inline lv_color_t yellow() { return lv_color_hex(0xFFCF33); }
inline lv_color_t red() { return lv_color_hex(0xFF3B30); }
}  // namespace UiTheme
