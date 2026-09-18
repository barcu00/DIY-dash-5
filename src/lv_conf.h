#pragma once

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (512U * 1024U)
#if defined(ARDUINO)
#include "board/lvgl_memory.h"
#define LV_MEM_POOL_ALLOC(size) ((void*)diy_lvgl_memory)
#endif
#define LV_TICK_CUSTOM 0
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_ASSERT_HANDLER_INCLUDE "stdlib.h"
#define LV_ASSERT_HANDLER abort();
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_48 1

#endif
