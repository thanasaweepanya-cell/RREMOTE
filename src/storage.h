#pragma once
#include "models.h"
#include <Arduino.h>

// Load all keys metadata from preferences
void loadKeysMeta();

// Save single key metadata
void saveKeyMeta(uint8_t i);

// Save raw code to slot
bool saveRawToSlot(uint8_t i, const RawCode &code);

// Load raw code from slot
bool loadRawFromSlot(uint8_t i, RawCode &out);

// Find first available slot
int findFreeSlot();

// WiFi config load/save
void wifiLoadConfig(String &staSsid, String &staPass);
void wifiSaveConfig(const String &staSsid, const String &staPass);