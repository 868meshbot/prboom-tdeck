// tdeck-doom — Log.h
// Imported from openmeshos-868.

#pragma once
#include <Arduino.h>

#define OPS_LOG(tag, fmt, ...) Serial.printf("[DOOM] %s: " fmt "\n", tag, ##__VA_ARGS__)
