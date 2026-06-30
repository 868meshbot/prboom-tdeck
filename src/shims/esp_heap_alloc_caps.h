// tdeck-doom — shim: esp_heap_alloc_caps.h
// This header was removed in ESP-IDF 4+.  Provides the old API via the new one.
// Placed in src/shims/ which appears first in -I search order.
#pragma once
#include "esp_heap_caps.h"

#ifndef pvPortMallocCaps
#  define pvPortMallocCaps(size, caps) heap_caps_malloc((size), (caps))
#endif
