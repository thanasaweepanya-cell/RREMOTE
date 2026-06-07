#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <ESPmDNS.h>

#include "config.h"
#include "models.h"
#include "storage.h"
#include "wifi_manager.h"
#include "api_handlers.h"
#include "learn_processor.h"

// ====== Global objects ======
WebServer server(80);
DNSServer dnsServer;

Preferences prefs;
Preferences keyPrefs;

IRrecv irrecv(PIN_IR_RECV, IR_RECV_BUF_SIZE, IR_RECV_TIMEOUT_MS, true);
IRsend irsend(PIN_IR_SEND);

decode_results results;

// ====== Global state ======
KeyMeta keys[MAX_KEYS];
LearnState learn;

bool irrecvEnabled = false;
bool isSendingIR = false;

void setup()
{
  Serial.begin(115200);
  delay(200);
  irsend.begin();
  irrecv.enableIRIn();

  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS mount failed");
  }
  Serial.println("SPIFFS files:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  {
    Serial.printf(" - %s (%u bytes)\n", file.name(), (unsigned)file.size());
    file = root.openNextFile();
  }
  Serial.println("SPIFFS check done.");

  loadKeysMeta();

  // WiFi (AP+STA)
  startAP();
  tryConnectSTA();

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < STA_CONNECT_WAIT_MS)
  {
    delay(200);
  }

  Serial.println("Ready.");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("STA not connected (use AP fallback).");
  }

  if (MDNS.begin(MDNS_NAME))
  {
    MDNS.addService("http", "tcp", 80);
    Serial.print("mDNS: http://");
    Serial.print(MDNS_NAME);
    Serial.println(".local/");
  }
  else
  {
    Serial.println("mDNS start failed");
  }

  setupRoutes();
  server.begin();
  Serial.println("HTTP server started");

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
}

void loop()
{
  dnsServer.processNextRequest();
  server.handleClient();
  learnTick();
  staReconnectTick();
}