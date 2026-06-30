// tdeck-doom — Keymap.h
// BBQ10 keyboard layout translation with T9-style accent cycling.
// Imported from openmeshos-868.

#pragma once
#include <cstdint>

namespace ops {
namespace keymap {

static constexpr uint8_t LAYOUT_EN    = 0;  // English QWERTY (default)
static constexpr uint8_t LAYOUT_FR    = 1;  // French AZERTY
static constexpr uint8_t LAYOUT_DE    = 2;  // German QWERTZ
static constexpr uint8_t LAYOUT_COUNT = 3;

extern const char* kLayoutNames;

struct KeyOut {
    char ch;      // character to emit (0 = nothing to send)
    bool replace; // true: erase previous char first (accent cycling)
};

// Translate one raw BBQ10 keypress.
// layout: LAYOUT_EN/FR/DE. nowMs: millis() for cycle timeout.
KeyOut translate(char raw, uint8_t layout, uint32_t nowMs);

}  // namespace keymap
}  // namespace ops
