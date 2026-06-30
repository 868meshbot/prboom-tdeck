// tdeck-doom — Board.h
// Hardware abstraction for the LilyGo T-Deck Plus.
// Imported from openmeshos-868; pin map verified against the official LilyGo schematic.
// DO NOT MODIFY this file — hardware workarounds go in callers.

#pragma once

#include <Arduino.h>
#include <Wire.h>
#ifdef OPS_HAS_BUILTIN_GPS
#include <TinyGPSPlus.h>
#endif

namespace ops {

class Board {
public:
    static Board& instance();

    void init();
    void tick();

    // Trackball direction accumulates between calls.
    // Press is polled from GPIO0 (BOOT button = trackball click, active-low).
    void consumeTrackballDelta(int16_t &dx, int16_t &dy);
    bool consumeTrackballPress();
    void signalTrackballPress() { _trackballPressed = true; }

    // Battery level 0-100 % via voltage divider on BOARD_BAT_ADC (GPIO4)
    int  batteryPercent() const;
    bool batteryCharging() const;  // true when VBAT > 4.1 V (USB attached)

    // BBQ10 keyboard (ESP32-C3 MCU at 0x55): returns true + fills key when pressed.
    bool pollKeyboard(char& outKey);

    // GPS (T-Deck Plus built-in; enable with -DOPS_HAS_BUILTIN_GPS)
    bool     hasGPSFix()      const;
    float    gpsLat()         const;
    float    gpsLng()         const;
    float    gpsAltM()        const;
    float    gpsHdop()        const;
    uint8_t  gpsSatellites()  const;
    uint32_t gpsNmeaCount()   const;
    bool gpsDateTime(uint16_t& year, uint8_t& month,  uint8_t& day,
                     uint8_t&  hour, uint8_t& minute, uint8_t& sec) const;

    uint32_t gpsLastSyncMs() const {
#ifdef OPS_HAS_BUILTIN_GPS
        return _gpsLastSync;
#else
        return 0;
#endif
    }

    void gpsStandby() {
#ifdef OPS_HAS_BUILTIN_GPS
        _gpsSerial.print("$PMTK161,0*28\r\n");
#endif
    }

    void gpsWake() {
#ifdef OPS_HAS_BUILTIN_GPS
        _gpsSerial.write((uint8_t)0xFF);
#endif
    }

    // Screen backlight PWM on GPIO42 via ledc (channel 0, 5 kHz, 8-bit).
    // Call initBacklightPWM() once after Board::init(); then setDisplayBrightness().
    // Range: 0 = off, 128 = 50%, 255 = full.
    static constexpr uint8_t  BL_LEDC_CH   = 0;
    static constexpr uint32_t BL_LEDC_FREQ = 5000;
    static constexpr uint8_t  BL_LEDC_BITS = 8;

    void initBacklightPWM() {
        ledcSetup(BL_LEDC_CH, BL_LEDC_FREQ, BL_LEDC_BITS);
        ledcAttachPin(42, BL_LEDC_CH);
    }
    void setDisplayBrightness(uint8_t val) { ledcWrite(BL_LEDC_CH, val); }

    // Keyboard backlight via keyboard MCU (I2C 0x55).
    // Sends all three known protocols — whichever the MCU firmware acts on.
    void setKeyboardBacklight(uint8_t brightness) {
        Wire.beginTransmission((uint8_t)0x55);
        Wire.write(brightness);
        Wire.endTransmission(true);
        Wire.beginTransmission((uint8_t)0x55);
        Wire.write((uint8_t)0x05);
        Wire.write(brightness);
        Wire.endTransmission(true);
        Wire.beginTransmission((uint8_t)0x55);
        Wire.write((uint8_t)0x01);
        Wire.write(brightness);
        Wire.endTransmission(true);
    }

    bool initialized() const { return _initialized; }

    // Enable/disable per-tick ISR-count logging to serial.
    static bool trackballDebug;

private:
    bool _initialized      = false;
    bool _keyboardPresent  = false;

    int16_t  _trackballX        = 0;
    int16_t  _trackballY        = 0;
    bool     _trackballPressed  = false;
    bool     _trackballPrevDown = false;
    uint32_t _trackballPressMs  = 0;

#ifdef OPS_HAS_BUILTIN_GPS
    HardwareSerial      _gpsSerial{1};
    mutable TinyGPSPlus _gps;
    uint32_t            _gpsLastSync = 0;
#endif
};

}  // namespace ops
