#include "../../src/lv_conf.h"
// Desktop-only allocator capacity; other rendering options match firmware.
#undef LV_MEM_SIZE
#define LV_MEM_SIZE (2U * 1024U * 1024U)
