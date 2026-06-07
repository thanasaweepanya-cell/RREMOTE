#pragma once
#include "models.h"
#include <Arduino.h>

// Pulse comparison
bool approxEqualPulse(int16_t a, int16_t b);

// Raw code comparison
bool rawEquals(const RawCode &A, const RawCode &B);

// Capture single IR signal
bool captureOnce(RawCode &out);