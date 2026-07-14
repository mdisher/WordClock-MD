// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#ifndef SERIAL_HELPER_H
#define SERIAL_HELPER_H

#include <Arduino.h>
#include "Config.h"

#if USE_SERIAL
#define SERIAL_PRINT(x) Serial.print(x)
#define SERIAL_PRINTLN(x) Serial.println(x)
#define SERIAL_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define SERIAL_PRINT(x)
#define SERIAL_PRINTLN(x)
#define SERIAL_PRINTF(...)
#endif

void initSerial();

#endif // SERIAL_HELPER_H
