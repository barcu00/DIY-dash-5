#pragma once
#include <sdkconfig.h>

// Check the selected, precompiled SDK configuration. Compiler -D flags cannot
// enable XIP inside already compiled ESP-IDF libraries.
#if !defined(CONFIG_SPIRAM_XIP_FROM_PSRAM) || !CONFIG_SPIRAM_XIP_FROM_PSRAM
#error "RGB/NVS safety requires the 3.1.1-h SDK with PSRAM XIP; see platformio.ini"
#endif
#if !defined(CONFIG_SPIRAM_FETCH_INSTRUCTIONS) || !CONFIG_SPIRAM_FETCH_INSTRUCTIONS || \
    !defined(CONFIG_SPIRAM_RODATA) || !CONFIG_SPIRAM_RODATA
#error "RGB/NVS safety requires both instructions and read-only data in PSRAM"
#endif
#if !defined(CONFIG_ESP32S3_DATA_CACHE_LINE_64B) || !CONFIG_ESP32S3_DATA_CACHE_LINE_64B
#error "RGB bounce-buffer configuration requires the SDK's 64-byte data-cache lines"
#endif
