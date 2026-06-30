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

// tdeck-doom — Board.cpp
// Hardware abstraction for the LilyGo T-Deck Plus (ESP32-S3).
// Imported from openmeshos-868; all pin numbers verified against the official LilyGo schematic.
// DO NOT MODIFY — hardware workarounds go in callers.

#include "Board.h"
#include "../utils/Log.h"
#include <Wire.h>
#include <sys/time.h>

// ── T-Deck keyboard MCU (ESP32-C3, I2C 0x55) ─────────────────────────────────
// Protocol: bare Wire.requestFrom(0x55, 1). Returns 0x00 = no key, non-zero = ASCII.
static constexpr uint8_t KB_ADDR = 0x55;

namespace ops {

// ── Pin definitions (T-Deck Plus) ─────────────────────────────────────────────
//
// CRITICAL: PIN_POWERON (GPIO10) must be HIGH before any peripheral
// (display, radio, SD) receives power.

static constexpr gpio_num_t PIN_POWERON   = GPIO_NUM_10;  // peripheral rail enable

// SPI bus — shared by TFT + LoRa + SD
static constexpr gpio_num_t PIN_SPI_SCK   = GPIO_NUM_40;
static constexpr gpio_num_t PIN_SPI_MOSI  = GPIO_NUM_41;
static constexpr gpio_num_t PIN_SPI_MISO  = GPIO_NUM_38;

// SX1262 LoRa radio
static constexpr gpio_num_t PIN_LORA_CS   = GPIO_NUM_9;
static constexpr gpio_num_t PIN_LORA_RST  = GPIO_NUM_17;
static constexpr gpio_num_t PIN_LORA_DIO1 = GPIO_NUM_45;
static constexpr gpio_num_t PIN_LORA_BUSY = GPIO_NUM_13;

// ST7789 TFT display
static constexpr gpio_num_t PIN_TFT_CS    = GPIO_NUM_12;
static constexpr gpio_num_t PIN_TFT_DC    = GPIO_NUM_11;
static constexpr gpio_num_t PIN_BACKLIGHT = GPIO_NUM_42;

// Trackball (optical encoder, active-low FALLING pulses)
static constexpr gpio_num_t PIN_TBALL_UP    = GPIO_NUM_3;
static constexpr gpio_num_t PIN_TBALL_DOWN  = GPIO_NUM_15;
static constexpr gpio_num_t PIN_TBALL_LEFT  = GPIO_NUM_1;
static constexpr gpio_num_t PIN_TBALL_RIGHT = GPIO_NUM_2;
// Press = GPIO0 (BOOT button), active-low
static constexpr gpio_num_t PIN_TBALL_PRESS = GPIO_NUM_0;

// I2C bus — keyboard + touchscreen
static constexpr gpio_num_t I2C_SDA = GPIO_NUM_18;
static constexpr gpio_num_t I2C_SCL = GPIO_NUM_8;

// Touchscreen interrupt
static constexpr gpio_num_t PIN_TOUCH_INT = GPIO_NUM_16;

// Keyboard interrupt
static constexpr gpio_num_t KB_INT = GPIO_NUM_46;

// GPS serial (T-Deck Plus built-in)
static constexpr gpio_num_t PIN_GPS_TX = GPIO_NUM_43;
static constexpr gpio_num_t PIN_GPS_RX = GPIO_NUM_44;

// SD card
static constexpr gpio_num_t PIN_SD_CS = GPIO_NUM_39;

// Battery ADC — 100k/100k voltage divider on GPIO4 (sees VBAT/2)
static constexpr int PIN_BATT_ADC = 4;

// ── Trackball ISR counters (IRAM — must survive cache eviction) ────────────────
static volatile uint32_t s_isrU = 0, s_isrD = 0, s_isrL = 0, s_isrR = 0;

static void IRAM_ATTR isr_tball_up()    { s_isrU++; }
static void IRAM_ATTR isr_tball_down()  { s_isrD++; }
static void IRAM_ATTR isr_tball_left()  { s_isrL++; }
static void IRAM_ATTR isr_tball_right() { s_isrR++; }

// ── Static instance ────────────────────────────────────────────────────────────
static Board s_board;
bool Board::trackballDebug = false;

Board& Board::instance() { return s_board; }

// ── init() ────────────────────────────────────────────────────────────────────
void Board::init()
{
    OPS_LOG("Board", "Initialising T-Deck Plus hardware");

    // 1) Power on peripheral rail FIRST — nothing works without this
    pinMode(PIN_POWERON, OUTPUT);
    digitalWrite(PIN_POWERON, HIGH);
    delay(100);

    // 2) Display backlight (TFT_eSPI handles display init; ledc takes over later)
    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);

    // 3) I2C bus — probe for keyboard MCU at 0x55
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(50);
    Wire.beginTransmission(KB_ADDR);
    _keyboardPresent = (Wire.endTransmission(true) == 0);
    OPS_LOG("Board", "Keyboard MCU at 0x%02X: %s",
            KB_ADDR, _keyboardPresent ? "FOUND" : "NOT FOUND");

    // 4) Trackball GPIOs — FALLING-edge ISRs count encoder pulses
    pinMode(PIN_TBALL_UP,    INPUT_PULLUP);
    pinMode(PIN_TBALL_DOWN,  INPUT_PULLUP);
    pinMode(PIN_TBALL_LEFT,  INPUT_PULLUP);
    pinMode(PIN_TBALL_RIGHT, INPUT_PULLUP);
    pinMode(PIN_TBALL_PRESS, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_TBALL_UP),    isr_tball_up,    FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_TBALL_DOWN),  isr_tball_down,  FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_TBALL_LEFT),  isr_tball_left,  FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_TBALL_RIGHT), isr_tball_right, FALLING);
    OPS_LOG("Board", "Trackball ISRs attached (UP=%d DN=%d LT=%d RT=%d)",
            PIN_TBALL_UP, PIN_TBALL_DOWN, PIN_TBALL_LEFT, PIN_TBALL_RIGHT);

    // 5) Touch controller interrupt
    pinMode(PIN_TOUCH_INT, INPUT_PULLUP);

    // 6) GPS serial
#ifdef OPS_HAS_BUILTIN_GPS
    _gpsSerial.begin(38400, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    OPS_LOG("Board", "GPS serial started (RX=%d TX=%d)", PIN_GPS_RX, PIN_GPS_TX);
#endif

    // 7) Seed ESP32 RTC from compile timestamp (GPS/NTP will correct it later)
    {
        static const char* months[] = {
            "Jan","Feb","Mar","Apr","May","Jun",
            "Jul","Aug","Sep","Oct","Nov","Dec"
        };
        struct tm tm = {};
        char mon[4] = {};
        sscanf(__DATE__, "%3s %d %d", mon, &tm.tm_mday, &tm.tm_year);
        sscanf(__TIME__, "%d:%d:%d",  &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
        tm.tm_year -= 1900;
        for (int i = 0; i < 12; i++) {
            if (strcmp(mon, months[i]) == 0) { tm.tm_mon = i; break; }
        }
        time_t ct = mktime(&tm);
        struct timeval tv = { ct, 0 };
        settimeofday(&tv, nullptr);
        OPS_LOG("Board", "RTC seeded: %s %s", __DATE__, __TIME__);
    }

    _initialized = true;
    OPS_LOG("Board", "Hardware ready (POWERON=GPIO%d BL=GPIO%d)",
            PIN_POWERON, PIN_BACKLIGHT);
}

// ── tick() ────────────────────────────────────────────────────────────────────
void Board::tick()
{
    if (!_initialized) return;

    // Drain ISR counters atomically
    uint32_t u, d, l, r;
    noInterrupts();
    u = s_isrU; s_isrU = 0;
    d = s_isrD; s_isrD = 0;
    l = s_isrL; s_isrL = 0;
    r = s_isrR; s_isrR = 0;
    interrupts();

    if (u || d || l || r) {
        _trackballX += (int16_t)(r - l);
        _trackballY += (int16_t)(d - u);

        if (trackballDebug) {
            uint32_t maxVal = u;
            const char* dir = "UP";
            if (d > maxVal) { maxVal = d; dir = "DOWN";  }
            if (l > maxVal) { maxVal = l; dir = "LEFT";  }
            if (r > maxVal) {             dir = "RIGHT"; }
            OPS_LOG("TRACKBALL", "dir=%s U=%lu D=%lu L=%lu R=%lu",
                    dir, (unsigned long)u, (unsigned long)d,
                    (unsigned long)l, (unsigned long)r);
        }
    }

    // Trackball press: falling-edge debounce on GPIO0 (active-low, 150 ms)
    {
        uint32_t now_ms = millis();
        bool curDown = !digitalRead(PIN_TBALL_PRESS);
        if (curDown && !_trackballPrevDown && (now_ms - _trackballPressMs >= 150UL)) {
            _trackballPressed = true;
            _trackballPressMs = now_ms;
        }
        _trackballPrevDown = curDown;
    }

    // GPS feed + RTC sync (once on first fix, then at most every 10 minutes)
#ifdef OPS_HAS_BUILTIN_GPS
    while (_gpsSerial.available()) {
        _gps.encode(_gpsSerial.read());
    }
    if (_gps.time.isUpdated() && _gps.date.isValid() && _gps.time.isValid()) {
        uint32_t now_ms = millis();
        if (_gpsLastSync == 0 || now_ms - _gpsLastSync >= 600000UL) {
            struct tm t = {};
            t.tm_year = _gps.date.year() - 1900;
            t.tm_mon  = _gps.date.month() - 1;
            t.tm_mday = _gps.date.day();
            t.tm_hour = _gps.time.hour();
            t.tm_min  = _gps.time.minute();
            t.tm_sec  = _gps.time.second();
            struct timeval tv = { mktime(&t), 0 };
            settimeofday(&tv, nullptr);
            _gpsLastSync = now_ms;
        }
    }
#endif
}

// ── Trackball ─────────────────────────────────────────────────────────────────
bool Board::consumeTrackballPress()
{
    if (_trackballPressed) { _trackballPressed = false; return true; }
    return false;
}

void Board::consumeTrackballDelta(int16_t& dx, int16_t& dy)
{
    dx = _trackballX;  dy = _trackballY;
    _trackballX = 0;   _trackballY = 0;
}

// ── Battery ───────────────────────────────────────────────────────────────────
// GPIO4 reads VBAT/2 via 100k/100k divider.
// 4.20 V full → raw ≈ 2607 | 3.50 V empty → raw ≈ 2172
static int _battRaw()
{
    int32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogRead(4);
    return (int)(sum / 16);
}

bool Board::batteryCharging() const
{
    if (!_initialized) return false;
    static bool s_charging = false;
    const int raw = _battRaw();
    if (!s_charging && raw > 2575) s_charging = true;
    if ( s_charging && raw < 2519) s_charging = false;
    return s_charging;
}

int Board::batteryPercent() const
{
    if (!_initialized) return 0;
    const int raw = _battRaw();
    const int pct = (int)map((long)raw, 2172, 2607, 0, 100);
    return constrain(pct, 0, 100);
}

// ── GPS ───────────────────────────────────────────────────────────────────────
bool Board::hasGPSFix() const {
#ifdef OPS_HAS_BUILTIN_GPS
    return _gps.location.isValid();
#else
    return false;
#endif
}
float Board::gpsLat() const {
#ifdef OPS_HAS_BUILTIN_GPS
    return static_cast<float>(_gps.location.lat());
#else
    return 0.0f;
#endif
}
float Board::gpsLng() const {
#ifdef OPS_HAS_BUILTIN_GPS
    return static_cast<float>(_gps.location.lng());
#else
    return 0.0f;
#endif
}
float Board::gpsAltM() const {
#ifdef OPS_HAS_BUILTIN_GPS
    return _gps.altitude.isValid() ? static_cast<float>(_gps.altitude.meters()) : 0.0f;
#else
    return 0.0f;
#endif
}
float Board::gpsHdop() const {
#ifdef OPS_HAS_BUILTIN_GPS
    return _gps.hdop.isValid() ? static_cast<float>(_gps.hdop.hdop()) : 99.9f;
#else
    return 99.9f;
#endif
}
uint8_t Board::gpsSatellites() const {
#ifdef OPS_HAS_BUILTIN_GPS
    return _gps.satellites.isValid() ? (uint8_t)_gps.satellites.value() : 0;
#else
    return 0;
#endif
}
uint32_t Board::gpsNmeaCount() const {
#ifdef OPS_HAS_BUILTIN_GPS
    return _gps.passedChecksum();
#else
    return 0;
#endif
}
bool Board::gpsDateTime(uint16_t& year, uint8_t& month, uint8_t& day,
                         uint8_t& hour, uint8_t& minute, uint8_t& sec) const {
#ifdef OPS_HAS_BUILTIN_GPS
    if (!_gps.date.isValid() || !_gps.time.isValid()) return false;
    year   = _gps.date.year();
    month  = _gps.date.month();
    day    = _gps.date.day();
    hour   = _gps.time.hour();
    minute = _gps.time.minute();
    sec    = _gps.time.second();
    return true;
#else
    (void)year; (void)month; (void)day;
    (void)hour; (void)minute; (void)sec;
    return false;
#endif
}

// ── Keyboard ──────────────────────────────────────────────────────────────────
// T-Deck keyboard MCU (0x55): single-byte requestFrom — 0x00 = no key, else ASCII.
bool Board::pollKeyboard(char& outKey)
{
    if (!_keyboardPresent) return false;
    if (Wire.requestFrom((uint8_t)KB_ADDR, (uint8_t)1) < 1) return false;
    if (!Wire.available()) return false;
    uint8_t raw = Wire.read();
    while (Wire.available()) Wire.read();
    if (raw == 0) return false;
    OPS_LOG("KB", "0x%02X ('%c')", raw, (raw >= 32 && raw < 127) ? (char)raw : '?');
    outKey = (char)raw;
    return true;
}

}  // namespace ops
