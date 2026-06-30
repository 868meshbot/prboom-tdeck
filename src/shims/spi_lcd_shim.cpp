// tdeck-doom — spi_lcd_shim.cpp
// Replaces lib/esp32-doom/components/prboom-esp32-compat/spi_lcd.c.
// Implements the three-function spi_lcd.h interface using our TFT_eSPI Display wrapper.
//
// Pipeline:
//   Doom renders 8-bit indexed pixels → screens[0].data (320×240 bytes)
//   I_SetPalette() fills lcdpal[256] with byte-swapped RGB565 entries
//   spi_lcd_send() casts screens[0].data to uint8_t*, applies lcdpal lookup,
//   writes 320×240 RGB565 pixels into s_rgb565, then calls display::blitFull().
//
// lcdpal[] byte order: I_SetPalette() stores (v>>8)+(v<<8) — byte-swapped RGB565.
// TFT_eSPI is initialised with setSwapBytes(true) which reverses that swap on output,
// producing correct big-endian RGB565 for the ST7789.

#include "../display/Display.h"
#include "../utils/Log.h"
#include <esp_heap_caps.h>

extern "C" {
#include "spi_lcd.h"
}

// Palette declared in i_video.c; byte-swapped RGB565, 256 entries.
extern "C" int16_t lcdpal[256];

// PSRAM buffer for the converted 320×240 RGB565 frame (153,600 bytes).
static uint16_t* s_rgb565 = nullptr;

extern "C" void spi_lcd_init()
{
    tdoom::display::init();
    s_rgb565 = (uint16_t*)ps_malloc(tdoom::display::WIDTH * tdoom::display::HEIGHT * sizeof(uint16_t));
    if (!s_rgb565) OPS_LOG("LCD", "FATAL: cannot allocate RGB565 conversion buffer");
}

extern "C" void spi_lcd_wait_finish()
{
    // TFT_eSPI flushes synchronously — no DMA pipeline to drain.
}

extern "C" void spi_lcd_send(uint16_t* scr)
{
    if (!s_rgb565) return;
    // scr is actually an 8-bit indexed buffer cast to uint16_t* by i_video.c.
    const uint8_t* src = reinterpret_cast<const uint8_t*>(scr);
    const int N = tdoom::display::WIDTH * tdoom::display::HEIGHT;
    for (int i = 0; i < N; i++) {
        s_rgb565[i] = static_cast<uint16_t>(lcdpal[src[i]]);
    }
    tdoom::display::blitFull(s_rgb565);
}
