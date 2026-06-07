#include "ir_utils.h"
#include "config.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

extern IRrecv irrecv;
extern decode_results results;
extern bool isSendingIR;

bool approxEqualPulse(int16_t a, int16_t b)
{
  int32_t da = abs((int32_t)a - (int32_t)b);

  int32_t maxv =
      max(abs((int32_t)a), abs((int32_t)b));

  // tolerance กว้างขึ้น
  int32_t tol =
      max((int32_t)250,
          (int32_t)(maxv * 0.35f));

  return da <= tol;
}

bool rawEquals(const RawCode &A, const RawCode &B)
{
  // len ต่างกันได้เล็กน้อย
  if (abs((int)A.len - (int)B.len) > 4)
    return false;

  uint16_t minLen = min(A.len, B.len);

  uint16_t matched = 0;

  for (uint16_t i = 0; i < minLen; i++)
  {
    if (approxEqualPulse(A.data[i], B.data[i]))
      matched++;
  }

  float score =
      (float)matched / (float)minLen;

  Serial.printf(
      "RAW match score: %.2f (%u/%u)\n",
      score,
      matched,
      minLen);

  // ผ่านถ้าคล้าย >= 85%
  return score >= 0.85f;
}

bool captureOnce(RawCode &out)
{
  if (isSendingIR)
    return false;
  if (!irrecv.decode(&results))
    return false;

  Serial.println("========== IR RECEIVED ==========");
  Serial.printf("decode_type: %d\n", results.decode_type);
  Serial.printf("bits: %u\n", results.bits);
  Serial.printf("rawlen: %u\n", results.rawlen);
  Serial.printf("freq: %u Hz\n", IR_CARRIER_KHZ * 1000U);

  uint16_t *raw = resultToRawArray(&results);
  uint16_t rawlen = getCorrectedRawLength(&results);

  if (rawlen == 0 || rawlen > MAX_RAW_LEN)
  {
    delete[] raw;
    irrecv.resume();
    return false;
  }

  out.freq_khz = IR_CARRIER_KHZ;
  out.len = rawlen;

  for (uint16_t i = 0; i < rawlen; i++)
    out.data[i] = (int16_t)raw[i];

  Serial.printf("corrected rawlen: %u\n", rawlen);

  for (uint16_t i = 0; i < min<uint16_t>(rawlen, 20); i++)
  {
    Serial.printf("%u ", raw[i]);
  }

  Serial.println();
  Serial.println("================================");

  delete[] raw;
  irrecv.resume();
  return true;
}