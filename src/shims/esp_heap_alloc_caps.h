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

// tdeck-doom — shim: esp_heap_alloc_caps.h
// This header was removed in ESP-IDF 4+.  Provides the old API via the new one.
// Placed in src/shims/ which appears first in -I search order.
#pragma once
#include "esp_heap_caps.h"

#ifndef pvPortMallocCaps
#  define pvPortMallocCaps(size, caps) heap_caps_malloc((size), (caps))
#endif
