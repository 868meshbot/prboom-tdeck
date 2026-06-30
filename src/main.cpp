// tdeck-doom — main.cpp
// Entry point for Doom on the LilyGo T-Deck Plus (ESP32-S3).
// Mirrors the role of app_main.c in the upstream esp32-doom IDF project.

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "hardware/Board.h"
#include "utils/Log.h"

// Doom engine entry point (i_main.c in prboom-esp32-compat).
extern "C" int doom_main(int argc, const char* const* argv);

// spi_lcd_init() and jsInit() are provided by our shims.
extern "C" void spi_lcd_init();
extern "C" void jsInit();

static void doomTask(void* /*arg*/)
{
    const char* argv[] = { "doom", "-cout", "ICWEFDA", nullptr };
    doom_main(3, argv);
    vTaskDelete(nullptr);
}

void setup()
{
    Serial.begin(115200);
    delay(200);
    OPS_LOG("Main", "T-Deck Doom starting");

    // Peripheral rail, I2C, trackball ISRs, RTC seed
    ops::Board::instance().init();

    // TFT_eSPI display init via our spi_lcd shim (called before doom task)
    spi_lcd_init();

    // Input init (no-op: PSX controller stubs)
    jsInit();

    // Doom engine runs in its own FreeRTOS task on core 0.
    // Stack 22 KB matches the upstream esp32-doom configuration.
    xTaskCreatePinnedToCore(
        doomTask,       // function
        "doom",         // name
        22 * 1024,      // stack bytes
        nullptr,        // arg
        5,              // priority
        nullptr,        // handle out
        0               // core
    );
}

void loop()
{
    // Board::tick() is called from gamepadPoll() inside the doom task.
    // Nothing to do here; yield so the idle task runs.
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
