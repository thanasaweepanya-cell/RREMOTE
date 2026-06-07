#pragma once

// API endpoint handlers
void handleApiStatus();
void handleApiListKeys();
void handleApiLearnStart();
void handleApiLearnCancel();
void handleApiSend();
void handleApiDelete();
void handleApiWifiGet();
void handleApiWifiSet();
void handleApiWifiScan();
void handleApiWifiDisconnect();
void handleApiWifiToggleAp();

// Setup all web routes
void setupRoutes();