// tdeck-doom — esp32_compat.h
// Force-included before every translation unit (via -include in build_flags).
// Patches the one API actually removed between the esp32-doom era and the
// current Arduino ESP32 SDK: pvPortMallocCaps → heap_caps_malloc.
// esp_spi_flash.h / spi_flash_mmap_handle_t / SPI_FLASH_MMAP_DATA are all
// still present in this SDK version — no shim needed for those.

#pragma once

// pvPortMallocCaps was removed; heap_caps_malloc is the replacement.
#include "esp_heap_caps.h"
#ifndef pvPortMallocCaps
#  define pvPortMallocCaps(size, caps) heap_caps_malloc((size), (caps))
#endif
