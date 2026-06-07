#pragma once
#include <Arduino.h>

// Start AP mode
void startAP();

// Try connect to saved STA
void tryConnectSTA();

// Handle STA reconnect tick
void staReconnectTick();

// Check whether AP mode is currently enabled
bool isApModeEnabled();

// Enable/disable AP mode
void setApModeEnabled(bool enabled);