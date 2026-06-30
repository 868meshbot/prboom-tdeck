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

// Pass through to the real SDK header — Arduino ESP32 still ships esp_spi_flash.h
// with the old IDF4 API (spi_flash_mmap_handle_t, SPI_FLASH_MMAP_DATA, etc.).
// This file exists only so -I src/shims doesn't hide the real header from
// esp_partition.h's internal #include "esp_spi_flash.h".
#pragma once
#include_next "esp_spi_flash.h"
