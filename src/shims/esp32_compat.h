// SPDX-License-Identifier: GPL-2.0-or-later
// prboom-tdeck — Doom for the LilyGo T-Deck Plus
// Copyright (C) 2024 prboom-tdeck contributors
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details: https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

// tdeck-doom — esp32_compat.h
// Force-included before every translation unit (via -include in build_flags).
// Patches the one API actually removed between the esp32-doom era and the
// current Arduino ESP32 SDK: pvPortMallocCaps → heap_caps_malloc.
// esp_spi_flash.h / spi_flash_mmap_handle_t / SPI_FLASH_MMAP_DATA are all
// still present in this SDK version — no shim needed for those.

#pragma once

// pvPortMallocCaps was removed; heap_caps_malloc is the replacement.
// Forward-declare only — including esp_heap_caps.h drags in <stdbool.h> which
// defines false/true as macros, breaking doomtype.h's:
//   typedef enum {false, true} boolean;
// <stddef.h> and <stdint.h> define size_t / uint32_t without touching stdbool.
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
extern void *heap_caps_malloc(size_t size, uint32_t caps);
#ifdef __cplusplus
}
#endif
#ifndef pvPortMallocCaps
#  define pvPortMallocCaps(size, caps) heap_caps_malloc((size), (caps))
#endif
