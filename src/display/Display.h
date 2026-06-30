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

// tdeck-doom — Display.h
// Lightweight ST7789 320×240 driver via TFT_eSPI.
// No LVGL — exposes a raw RGB565 framebuffer blit for Doom's renderer.

#pragma once
#include <cstdint>

namespace tdoom {
namespace display {

static constexpr int WIDTH  = 320;
static constexpr int HEIGHT = 240;

// Initialise TFT_eSPI, attach backlight PWM, clear to black.
// Call after Board::init().
void init();

// Push a region of RGB565 pixels to the screen.
// pixels: row-major array of w*h uint16_t values.
void blit(int x, int y, int w, int h, const uint16_t* pixels);

// Push a full-screen framebuffer (must be WIDTH*HEIGHT pixels).
void blitFull(const uint16_t* pixels);

// Fill the entire screen with a solid RGB565 colour.
void fill(uint16_t color);

// Set backlight brightness: 0 = off, 255 = full brightness.
void setBacklight(uint8_t val);

}  // namespace display
}  // namespace tdoom
