#pragma once
#include <stdint.h>

// =========================
// Board / IR Pins
// =========================
static constexpr uint16_t PIN_IR_RECV = 15;
static constexpr uint16_t PIN_IR_SEND = 4;

// IR carrier frequency (kHz)
static constexpr uint16_t IR_CARRIER_KHZ = 38;

// =========================
// IR Receiver tuning
// =========================
static constexpr uint16_t IR_RECV_BUF_SIZE = 1024;
static constexpr uint8_t IR_RECV_TIMEOUT_MS = 50;

// =========================
// RAW compare tolerance
// =========================
// 3 ครั้งต้อง "เหมือนกัน" แต่อนุญาตให้คลาดเคลื่อนเล็กน้อย (ตามจริง IR timing จะสวิงได้)
static constexpr uint16_t RAW_TOL_USEC = 200; // +/- 200us
static constexpr float RAW_TOL_RATIO = 0.20f; // หรือ 20% (เลือก max ระหว่าง 2 เงื่อนไข)

// =========================
// Learn constraints
// =========================
static constexpr uint8_t LEARN_TRIES = 3;
static constexpr uint8_t MAX_KEYS = 20;
static constexpr uint32_t LEARN_WINDOW_MS = 30000;

// =========================
// RAW storage limits
// =========================
static constexpr uint16_t MAX_RAW_LEN = 350;

// =========================
// WiFi AP defaults
// =========================
static constexpr const char *AP_SSID = "ESP32-IR";
static constexpr const char *AP_PASS = "12345678"; // >= 8 chars
static constexpr uint8_t DNS_PORT = 53;
// mDNS hostname (เปิดเว็บผ่าน http://remote.local/)
static constexpr const char *MDNS_NAME = "remote";

// ถ้าต้องการ "ใช้ Wi‑Fi บ้านเป็นหลัก" ให้พยายามต่อ STA นานขึ้นหน่อย
static constexpr uint32_t STA_CONNECT_WAIT_MS = 10000;