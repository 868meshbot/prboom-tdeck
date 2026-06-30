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
