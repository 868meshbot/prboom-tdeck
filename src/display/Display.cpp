// tdeck-doom — Display.cpp
// TFT_eSPI wrapper for direct framebuffer blitting (no LVGL).
// Extracted and simplified from openmeshos-868/src/ui/UIScreen.cpp.

#include "Display.h"
#include "../hardware/Board.h"
#include "../utils/Log.h"
#include <TFT_eSPI.h>

// Single global TFT instance (TFT_eSPI is configured entirely via build flags).
static TFT_eSPI s_tft;

namespace tdoom {
namespace display {

void init()
{
    s_tft.begin();
    s_tft.setRotation(1);         // landscape: 320 wide, 240 tall
    s_tft.setSwapBytes(true);     // send high byte first — ST7789 expects big-endian RGB565
    s_tft.fillScreen(TFT_BLACK);

    // Switch GPIO42 from digital HIGH to ledc PWM so brightness is dimmable.
    // Board::init() already drove the pin HIGH; initBacklightPWM() re-routes the
    // pin mux to ledc, overriding that.
    ops::Board::instance().initBacklightPWM();
    ops::Board::instance().setDisplayBrightness(255);

    OPS_LOG("Display", "ST7789 %dx%d ready", WIDTH, HEIGHT);
}

void blit(int x, int y, int w, int h, const uint16_t* pixels)
{
    s_tft.startWrite();
    s_tft.setAddrWindow(x, y, w, h);
    s_tft.pushPixels((uint16_t*)pixels, w * h);
    s_tft.endWrite();
}

void blitFull(const uint16_t* pixels)
{
    blit(0, 0, WIDTH, HEIGHT, pixels);
}

void fill(uint16_t color)
{
    s_tft.fillScreen(color);
}

void setBacklight(uint8_t val)
{
    ops::Board::instance().setDisplayBrightness(val);
}

}  // namespace display
}  // namespace tdoom
