#include "api_handlers.h"
#include "config.h"
#include "models.h"
#include "storage.h"
#include "ir_utils.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <SPIFFS.h>
#include "wifi_manager.h"
#include "learn_processor.h"
#include <preferences.h>

extern WebServer server;
extern KeyMeta keys[MAX_KEYS];
extern LearnState learn;
extern IRsend irsend;
extern IRrecv irrecv;
extern bool isSendingIR;
extern Preferences prefs;

static void sendJson(int code, const JsonDocument &doc)
{
  String out;
  serializeJson(doc, out);
  server.send(code, "application/json; charset=utf-8", out);
}

static bool loadFile(const char *path, const char *contentType)
{
  if (!SPIFFS.exists(path))
    return false;
  File f = SPIFFS.open(path, "r");
  server.streamFile(f, contentType);
  f.close();
  return true;
}

void handleApiStatus()
{
  StaticJsonDocument<768> doc;
  doc["ok"] = true;

  doc["learn"]["active"] = learn.active;
  doc["learn"]["got"] = learn.got;
  doc["learn"]["need"] = LEARN_TRIES;
  doc["learn"]["slot"] = learn.slot;
  doc["learn"]["status"] = learn.status;
  doc["learn"]["remaining_ms"] = learn.active ? (int32_t)max<int32_t>(0, (int32_t)(LEARN_WINDOW_MS - (millis() - learn.startedAt))) : 0;

  doc["wifi"]["ap_ssid"] = AP_SSID;
  doc["wifi"]["ap_ip"] = WiFi.softAPIP().toString();
  doc["wifi"]["ap_enabled"] = isApModeEnabled();
  doc["wifi"]["sta_connected"] = (WiFi.status() == WL_CONNECTED);
  doc["wifi"]["sta_ssid"] = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "";
  doc["wifi"]["sta_ip"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";

  sendJson(200, doc);
}

void handleApiListKeys()
{
  StaticJsonDocument<2048> doc;
  doc["ok"] = true;
  JsonArray arr = doc["keys"].to<JsonArray>();
  for (uint8_t i = 0; i < MAX_KEYS; i++)
  {
    JsonObject k = arr.add<JsonObject>();
    k["slot"] = i;
    k["id"] = keys[i].id;
    k["name"] = keys[i].name;
    k["used"] = keys[i].used;
  }
  sendJson(200, doc);
}

void handleApiLearnStart()
{
  if (learn.active)
  {
    StaticJsonDocument<256> doc;
    doc["ok"] = false;
    doc["error"] = "learn_already_active";
    sendJson(409, doc);
    return;
  }

  String name = server.hasArg("name") ? server.arg("name") : "";
  name.trim();
  if (name.length() == 0)
    name = "New key";

  int slot = findFreeSlot();
  if (slot < 0)
  {
    StaticJsonDocument<256> doc;
    doc["ok"] = false;
    doc["error"] = "full";
    sendJson(409, doc);
    return;
  }

  learn.active = true;
  learn.startedAt = millis();
  learn.slot = slot;
  learn.got = 0;
  strlcpy(learn.pendingName, name.c_str(), sizeof(learn.pendingName));
  strlcpy(learn.status, "waiting", sizeof(learn.status));

  StaticJsonDocument<256> doc;
  doc["ok"] = true;
  doc["slot"] = slot;
  doc["need"] = LEARN_TRIES;
  sendJson(200, doc);
}

void handleApiLearnCancel()
{
  learn.active = false;
  learn.got = 0;
  learn.slot = -1;
  strlcpy(learn.status, "idle", sizeof(learn.status));

  StaticJsonDocument<128> doc;
  doc["ok"] = true;
  sendJson(200, doc);
}

void handleApiSend()
{
  if (!server.hasArg("slot"))
  {
    StaticJsonDocument<256> doc;
    doc["ok"] = false;
    doc["error"] = "missing_slot";
    sendJson(400, doc);
    return;
  }

  int slot = server.arg("slot").toInt();

  if (slot < 0 || slot >= MAX_KEYS || !keys[slot].used)
  {
    StaticJsonDocument<256> doc;
    doc["ok"] = false;
    doc["error"] = "invalid_slot";
    sendJson(400, doc);
    return;
  }

  RawCode code;

  if (!loadRawFromSlot(slot, code))
  {
    StaticJsonDocument<256> doc;
    doc["ok"] = false;
    doc["error"] = "no_code_data";
    sendJson(500, doc);
    return;
  }

  Serial.println("========== SEND IR ==========");
  Serial.printf("slot: %d\n", slot);
  Serial.printf("freq: %u kHz\n", code.freq_khz);
  Serial.printf("len : %u\n", code.len);

  std::vector<uint16_t> buf(code.len);

  for (uint16_t i = 0; i < code.len; i++)
  {
    buf[i] = (uint16_t)abs(code.data[i]);
  }

  isSendingIR = true;
  irrecv.disableIRIn();
  delay(30);

  irsend.sendRaw(
      buf.data(),
      code.len,
      code.freq_khz);

  delay(120);

  irrecv.enableIRIn();
  isSendingIR = false;

  Serial.println("IR SENT");
  Serial.println("============================");

  StaticJsonDocument<256> doc;
  doc["ok"] = true;
  doc["sent"]["slot"] = slot;
  doc["sent"]["freq_khz"] = code.freq_khz;
  doc["sent"]["len"] = code.len;

  sendJson(200, doc);
}

void handleApiDelete()
{
  if (!server.hasArg("slot"))
  {
    StaticJsonDocument<256> doc;
    doc["ok"] = false;
    doc["error"] = "missing_slot";
    sendJson(400, doc);
    return;
  }
  int slot = server.arg("slot").toInt();
  if (slot < 0 || slot >= MAX_KEYS)
  {
    StaticJsonDocument<256> doc;
    doc["ok"] = false;
    doc["error"] = "invalid_slot";
    sendJson(400, doc);
    return;
  }

  Preferences keyPrefs;
  keyPrefs.begin("keys", false);
  String k = String(slot);
  keyPrefs.remove(k.c_str());
  keyPrefs.end();

  keys[slot].used = false;
  strlcpy(keys[slot].name, (String("Key ") + slot).c_str(), sizeof(keys[slot].name));
  saveKeyMeta(slot);

  StaticJsonDocument<128> doc;
  doc["ok"] = true;
  sendJson(200, doc);
}

void handleApiWifiGet()
{
  String ssid, pass;
  wifiLoadConfig(ssid, pass);

  StaticJsonDocument<384> doc;
  doc["ok"] = true;
  doc["sta_ssid"] = ssid;
  doc["sta_connected"] = (WiFi.status() == WL_CONNECTED);
  doc["connected_ssid"] = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "";
  doc["sta_ip"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
  doc["ap_ssid"] = AP_SSID;
  doc["ap_ip"] = WiFi.softAPIP().toString();
  doc["ap_enabled"] = isApModeEnabled();
  sendJson(200, doc);
}

void handleApiWifiSet()
{
  String ssid = server.hasArg("ssid") ? server.arg("ssid") : "";
  String pass = server.hasArg("pass") ? server.arg("pass") : "";
  ssid.trim();

  if (ssid.length() == 0)
  {
    StaticJsonDocument<256> doc;
    doc["ok"] = false;
    doc["error"] = "missing_ssid";
    sendJson(400, doc);
    return;
  }

  Serial.println("[WiFi] Saving WiFi config...");
  wifiSaveConfig(ssid, pass);

  Serial.println("[WiFi] Disconnecting STA (keeping AP)...");
  WiFi.disconnect(false); // ← FIX: false = เก็บ AP mode ไว้
  delay(500);             // ← FIX: รอให้ disconnect เสร็จ

  Serial.println("[WiFi] Starting AP mode...");
  startAP();
  delay(200);

  Serial.println("[WiFi] Attempting STA connection...");
  tryConnectSTA();

  StaticJsonDocument<256> doc;
  doc["ok"] = true;
  doc["message"] = "saved_reconnecting";
  sendJson(200, doc);
}

void handleApiWifiScan()
{
  const int networkCount = WiFi.scanNetworks(false, true);

  if (networkCount < 0)
  {
    StaticJsonDocument<192> doc;
    doc["ok"] = false;
    doc["error"] = "scan_failed";
    sendJson(500, doc);
    return;
  }

  DynamicJsonDocument doc(512 + (networkCount * 128));
  doc["ok"] = true;
  JsonArray arr = doc["networks"].to<JsonArray>();

  for (int i = 0; i < networkCount; i++)
  {
    JsonObject net = arr.add<JsonObject>();
    net["ssid"] = WiFi.SSID(i);
    net["rssi"] = WiFi.RSSI(i);
    net["channel"] = WiFi.channel(i);
    net["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }

  WiFi.scanDelete();
  sendJson(200, doc);
}

void handleApiWifiDisconnect()
{
  prefs.begin("wifi", false);
  prefs.remove("sta_ssid");
  prefs.remove("sta_pass");
  prefs.end();

  WiFi.disconnect(false, false);

  StaticJsonDocument<256> doc;
  doc["ok"] = true;
  doc["ap_enabled"] = isApModeEnabled();
  sendJson(200, doc);
}

void handleApiWifiToggleAp()
{
  const bool nextApEnabled = !isApModeEnabled();
  setApModeEnabled(nextApEnabled);

  StaticJsonDocument<192> doc;
  doc["ok"] = true;
  doc["ap_enabled"] = isApModeEnabled();
  sendJson(200, doc);
}

void setupRoutes()
{
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/keys", HTTP_GET, handleApiListKeys);
  server.on("/api/learn/start", HTTP_POST, handleApiLearnStart);
  server.on("/api/learn/cancel", HTTP_POST, handleApiLearnCancel);
  server.on("/api/send", HTTP_POST, handleApiSend);
  server.on("/api/delete", HTTP_POST, handleApiDelete);
  server.on("/api/wifi/get", HTTP_GET, handleApiWifiGet);
  server.on("/api/wifi/set", HTTP_POST, handleApiWifiSet);
  server.on("/api/wifi/scan", HTTP_GET, handleApiWifiScan);
  server.on("/api/wifi/disconnect", HTTP_POST, handleApiWifiDisconnect);
  server.on("/api/wifi/toggle-ap", HTTP_POST, handleApiWifiToggleAp);

  server.on("/", HTTP_GET, []()
            {
    if (!loadFile("/index.html", "text/html; charset=utf-8")) server.send(404, "text/plain", "Missing index.html"); });
  server.on("/app.css", HTTP_GET, []()
            {
    if (!loadFile("/app.css", "text/css; charset=utf-8")) server.send(404, "text/plain", "Missing app.css"); });
  server.on("/app.js", HTTP_GET, []()
            {
    if (!loadFile("/app.js", "application/javascript; charset=utf-8")) server.send(404, "text/plain", "Missing app.js"); });

  server.onNotFound([]()
                    {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", ""); });
}