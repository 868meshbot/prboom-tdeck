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

// tdeck-doom — tdeck_input.cpp
// Replaces gamepad.c + psxcontroller.c from esp32-doom.
// Maps the T-Deck Plus BBQ10 keyboard (I2C 0x55) and optical trackball to Doom events.
// Menu/back: Esc (0x1B), Backspace (0x08), or U/u
//
// ── Control scheme ────────────────────────────────────────────────────────────
//
//   TRACKBALL
//   ─────────
//   Roll UP       →  Move forward       (key_up)
//   Roll DOWN     →  Move backward      (key_down)
//   Roll LEFT     →  Turn left          (key_left)
//   Roll RIGHT    →  Turn right         (key_right)
//   Press (GPIO0) →  Fire               (key_fire)
//
//   KEYBOARD (BBQ10 sends ASCII; MCU at I2C 0x55 auto-repeats while held)
//   ────────────────────────────────────────────────────────────────────────
//   W / w         →  Move forward       (key_up)
//   S / s         →  Move backward      (key_down)
//   A / a         →  Strafe left        (key_strafeleft)
//   D / d         →  Strafe right       (key_straferight)
//   Space         →  Use / open door    (key_use)
//   P / p         →  Fire               (key_fire)
//   Esc   (0x1B)  →  Menu / back        (key_escape)
//   M / m         →  Automap            (key_map)
//   $             →  Cycle weapon       (key_weapontoggle)
//   1–7           →  Select weapon 1–7
//
// ── Key-up emulation ─────────────────────────────────────────────────────────
// The BBQ10 MCU (0x55) sends only key-down events; Doom needs ev_keyup too.
// Strategy: post ev_keydown on first receipt; extend a hold deadline each time
// the same key repeats (MCU auto-repeats at ~10 Hz while held); post ev_keyup
// when the deadline expires without a repeat.  KEY_HOLD_MS = 120 ms ≈ 4 tics
// of silence before release is declared.

#include <Arduino.h>
#include "../hardware/Board.h"
#include "../utils/Log.h"

extern "C" {
#include "gamepad.h"
#include "psxcontroller.h"
#include "d_event.h"
#include "d_main.h"
#include "g_game.h"
#include "doomdef.h"
}

// ── Required gamepad globals (normally in gamepad.c) ─────────────────────────
int usejoystick = 0;
int joyleft = 0, joyright = 0, joyup = 0, joydown = 0;

// ── Key-hold state ────────────────────────────────────────────────────────────
static int      s_heldDoomKey    = 0;
static uint32_t s_holdDeadlineMs = 0;
#define KEY_HOLD_MS 120

// ── Trackball direction flags ─────────────────────────────────────────────────
static bool s_tbUp    = false;
static bool s_tbDown  = false;
static bool s_tbLeft  = false;
static bool s_tbRight = false;
static bool s_tbFire  = false;

// ── Helpers ───────────────────────────────────────────────────────────────────
static void postKey(int evtype, int code)
{
    event_t ev;
    ev.type  = (evtype_t)evtype;
    ev.data1 = code;
    ev.data2 = 0;
    ev.data3 = 0;
    D_PostEvent(&ev);
}

// Post ev_keydown / ev_keyup only when the flag actually changes.
static void tbEdge(bool* flag, int doomKey, bool active)
{
    if (active == *flag) return;
    *flag = active;
    postKey(active ? ev_keydown : ev_keyup, doomKey);
}

// Map ASCII byte from BBQ10 MCU → Doom key code (0 = ignore).
static int asciiToDoom(uint8_t c)
{
    switch (c) {
    case 'w': case 'W': return key_up;
    case 's': case 'S': return key_down;
    case 'a': case 'A': return key_strafeleft;
    case 'd': case 'D': return key_straferight;
    case ' ':           return key_use;
    case 'p': case 'P': return 'p';         // Fire
    case '\r':          return key_menu_enter; // Menu select / confirm
    case 0x1B:          return key_escape;  // Menu / back
    case 0x08: case 0x7F:                   // Backspace (BBQ10 sends 0x08)
    case 'u': case 'U': return key_escape;  // U / backspace → menu / back
    case 'm': case 'M': return 'm';         // Automap
    case '$':           return key_weapontoggle;
    // Weapon slots: ASCII '1'–'7' == Doom weapon key codes
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': return (int)c;
    default:            return 0;
    }
}

// ── Public interface (extern "C") ─────────────────────────────────────────────

extern "C" void gamepadInit(void)
{
    key_fire = 'p';   // P fires; avoids conflict with Enter/menu-select
    key_map  = 'm';   // M opens automap (BBQ10 has no Tab key)
}

extern "C" void gamepadPoll(void)
{
    // Re-enforce bindings every tic — prboom's M_LoadDefaults runs before
    // gamepadInit() but some subsystems may reset keys after that.
    key_fire = 'p';
    key_map  = 'm';

    auto& board   = ops::Board::instance();
    uint32_t now  = (uint32_t)millis();

    // Drain trackball ISR counters and trackball press debounce
    board.tick();

    // ── Trackball movement ─────────────────────────────────────────────────
    int16_t dx = 0, dy = 0;
    board.consumeTrackballDelta(dx, dy);

    tbEdge(&s_tbUp,    key_up,    dy < 0);
    tbEdge(&s_tbDown,  key_down,  dy > 0);
    tbEdge(&s_tbLeft,  key_left,  dx < 0);
    tbEdge(&s_tbRight, key_right, dx > 0);

    // ── Trackball press → fire ─────────────────────────────────────────────
    bool pressed = board.consumeTrackballPress();
    // Fire key stays down for exactly one poll cycle (one Doom tic) then auto-releases.
    if (pressed && !s_tbFire) {
        s_tbFire = true;
        postKey(ev_keydown, key_fire);
    } else if (!pressed && s_tbFire) {
        s_tbFire = false;
        postKey(ev_keyup, key_fire);
    }

    // ── Keyboard auto-release ──────────────────────────────────────────────
    if (s_heldDoomKey && (int32_t)(now - s_holdDeadlineMs) >= 0) {
        postKey(ev_keyup, s_heldDoomKey);
        s_heldDoomKey = 0;
    }

    // ── Keyboard poll ──────────────────────────────────────────────────────
    char raw = 0;
    if (board.pollKeyboard(raw)) {
        int dk = asciiToDoom((uint8_t)raw);
        if (dk) {
            if (dk == s_heldDoomKey) {
                // Same key repeating: extend hold window, do NOT re-post ev_keydown
                s_holdDeadlineMs = now + KEY_HOLD_MS;
            } else {
                // New key: release previous, press new
                if (s_heldDoomKey) {
                    postKey(ev_keyup, s_heldDoomKey);
                }
                s_heldDoomKey    = dk;
                s_holdDeadlineMs = now + KEY_HOLD_MS;
                postKey(ev_keydown, dk);
            }
        }
    }
}

// ── PSX controller stubs (no PSX hardware on T-Deck) ─────────────────────────
extern "C" void jsInit(void)             {}
extern "C" void psxcontrollerInit(void)  {}
extern "C" int  psxReadInput(void)       { return 0; }
