#include "wifi_manager.h"
#include "config.h"
#include "storage.h"
#include <WiFi.h>

static uint32_t lastStaAttemptMs = 0;
static bool printedStaIp = false;

void startAP()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[AP] Started: SSID=%s IP=%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void tryConnectSTA()
{
  String ssid, pass;
  wifiLoadConfig(ssid, pass);

  if (ssid.length() == 0)
  {
    Serial.println("[STA] No WiFi config saved, skipping");
    return;
  }

  Serial.printf("[STA] Connecting to SSID: %s\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());
}

void staReconnectTick()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!printedStaIp)
    {
      printedStaIp = true;
      Serial.printf("[STA] ✓ Connected, IP: %s\n", WiFi.localIP().toString().c_str());
    }
    return;
  }

  printedStaIp = false;

  if (millis() - lastStaAttemptMs < 10000)
  {
    return;
  }

  lastStaAttemptMs = millis();

  uint8_t status = WiFi.status();
  Serial.printf("[STA] Not connected (status=%d), retrying in 10s...\n", status);
  tryConnectSTA();
}

bool isApModeEnabled()
{
  const wifi_mode_t mode = WiFi.getMode();
  return (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA);
}

void setApModeEnabled(bool enabled)
{
  if (enabled)
  {
    startAP();
    return;
  }

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_MODE_STA);
  Serial.println("[AP] Disabled");
}