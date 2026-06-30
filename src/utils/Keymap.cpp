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

// tdeck-doom — Keymap.cpp
// T9-style accent cycling for the BBQ10 keyboard on the LilyGo T-Deck.
// Imported from openmeshos-868.
//
// Layout remap is applied first so a French user pressing physical-Q gets
// logical-A, then cycling à, â… as expected.

#include "Keymap.h"
#include <cstring>

namespace ops {
namespace keymap {

const char* kLayoutNames = "English (Default)\nFrench AZERTY\nGerman QWERTZ";

static constexpr uint32_t CYCLE_MS = 500;

// Physical QWERTY → logical letter maps
static const char kMap[LAYOUT_COUNT][26] = {
    // EN — identity
    { 'a','b','c','d','e','f','g','h','i','j','k','l','m',
      'n','o','p','q','r','s','t','u','v','w','x','y','z' },
    // FR AZERTY: q→a, a→q, w→z, z→w
    { 'q','b','c','d','e','f','g','h','i','j','k','l','m',
      'n','o','p','a','r','s','t','u','v','z','x','y','w' },
    // DE QWERTZ: y→z, z→y
    { 'a','b','c','d','e','f','g','h','i','j','k','l','m',
      'n','o','p','q','r','s','t','u','v','w','x','z','y' },
};

struct CycleRow {
    char        base;
    const char* lo;  // lowercase cycle: base + accented variants
    const char* hi;  // uppercase cycle
};

static const CycleRow kCycleFR[] = {
    { 'e', "e\xE9\xE8\xEA\xEB", "E\xC9\xC8\xCA\xCB" },
    { 'a', "a\xE0\xE2",         "A\xC0\xC2"          },
    { 'u', "u\xF9\xFB\xFC",     "U\xD9\xDB\xDC"      },
    { 'i', "i\xEE\xEF",         "I\xCE\xCF"          },
    { 'o', "o\xF4",             "O\xD4"              },
    { 'c', "c\xE7",             "C\xC7"              },
};
static constexpr int kCycleFRLen = (int)(sizeof(kCycleFR)/sizeof(kCycleFR[0]));

static const CycleRow kCycleDE[] = {
    { 'a', "a\xE4", "A\xC4" },
    { 'o', "o\xF6", "O\xD6" },
    { 'u', "u\xFC", "U\xDC" },
    { 's', "s\xDF", "S\xDF" },
};
static constexpr int kCycleDELen = (int)(sizeof(kCycleDE)/sizeof(kCycleDE[0]));

static char     s_pendingBase  = '\0';
static bool     s_pendingUpper = false;
static int      s_cycleIdx     = 0;
static uint32_t s_cycleMs      = 0;
static uint8_t  s_cycleLayout  = 0xFF;

static void _clearCycle()
{
    s_pendingBase  = '\0';
    s_pendingUpper = false;
    s_cycleIdx     = 0;
    s_cycleMs      = 0;
    s_cycleLayout  = 0xFF;
}

static char _remap(char raw, uint8_t layout)
{
    if (raw >= 'a' && raw <= 'z') return kMap[layout][(uint8_t)(raw - 'a')];
    if (raw >= 'A' && raw <= 'Z')
        return (char)(kMap[layout][(uint8_t)(raw - 'A')] - 'a' + 'A');
    return raw;
}

KeyOut translate(char raw, uint8_t layout, uint32_t nowMs)
{
    // Extended bytes from BBQ10 hardware (e.g. Shift+0 → 0xE0): discard
    if ((uint8_t)raw > 127) { _clearCycle(); return {'\0', false}; }

    if (layout == LAYOUT_EN || layout >= LAYOUT_COUNT) {
        _clearCycle();
        return {raw, false};
    }

    char mapped  = _remap(raw, layout);
    bool isUpper = (mapped >= 'A' && mapped <= 'Z');
    bool isLower = (mapped >= 'a' && mapped <= 'z');

    if (!isLower && !isUpper) { _clearCycle(); return {mapped, false}; }

    char baseLo = isUpper ? (char)(mapped - 'A' + 'a') : mapped;

    const CycleRow* table    = nullptr;
    int             tableLen = 0;
    if (layout == LAYOUT_FR) { table = kCycleFR; tableLen = kCycleFRLen; }
    else if (layout == LAYOUT_DE) { table = kCycleDE; tableLen = kCycleDELen; }

    const CycleRow* row = nullptr;
    for (int i = 0; i < tableLen; i++) {
        if (table[i].base == baseLo) { row = &table[i]; break; }
    }

    if (!row) { _clearCycle(); return {mapped, false}; }

    const char* cycleStr = isUpper ? row->hi : row->lo;
    int         cycleLen = (int)strlen(cycleStr);

    bool continuing = (s_pendingBase  == baseLo  &&
                       s_cycleLayout  == layout   &&
                       s_pendingUpper == isUpper  &&
                       s_cycleMs      > 0         &&
                       (nowMs - s_cycleMs) < CYCLE_MS);

    if (continuing) {
        s_cycleIdx = (s_cycleIdx + 1) % cycleLen;
        s_cycleMs  = nowMs;
        return {cycleStr[s_cycleIdx], true};
    } else {
        s_pendingBase  = baseLo;
        s_pendingUpper = isUpper;
        s_cycleIdx     = 0;
        s_cycleMs      = nowMs;
        s_cycleLayout  = layout;
        return {cycleStr[0], false};
    }
}

}  // namespace keymap
}  // namespace ops
