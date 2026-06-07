#include "storage.h"
#include "config.h"
#include "Preferences.h"
#include "memory"
extern KeyMeta keys[MAX_KEYS];
extern Preferences prefs;
extern Preferences keyPrefs;

static String keyDataKey(uint8_t i)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "k%02u", i);
  return String(buf);
}

void loadKeysMeta()
{
  prefs.begin("meta", true);
  for (uint8_t i = 0; i < MAX_KEYS; i++)
  {
    String usedKey = "used_" + String(i);
    String nameKey = "name_" + String(i);
    keys[i].used = prefs.getBool(usedKey.c_str(), false);

    snprintf(keys[i].id, sizeof(keys[i].id), "k%02u", i);

    String defName = String("Key ") + i;
    String nm = prefs.getString(nameKey.c_str(), defName);
    strlcpy(keys[i].name, nm.c_str(), sizeof(keys[i].name));
  }
  prefs.end();
}

void saveKeyMeta(uint8_t i)
{
  prefs.begin("meta", false);
  String usedKey = "used_" + String(i);
  String nameKey = "name_" + String(i);
  prefs.putBool(usedKey.c_str(), keys[i].used);
  prefs.putString(nameKey.c_str(), String(keys[i].name));
  prefs.end();
}

bool saveRawToSlot(uint8_t i, const RawCode &code)
{
  uint16_t len = code.len;
  if (len == 0 || len > MAX_RAW_LEN)
    return false;

  size_t blobSize = sizeof(uint16_t) * 2 + sizeof(int16_t) * len;
  std::unique_ptr<uint8_t[]> blob(new uint8_t[blobSize]);
  uint8_t *p = blob.get();

  memcpy(p, &code.freq_khz, sizeof(uint16_t));
  p += sizeof(uint16_t);
  memcpy(p, &len, sizeof(uint16_t));
  p += sizeof(uint16_t);
  memcpy(p, code.data, sizeof(int16_t) * len);

  keyPrefs.begin("keys", false);
  String k = keyDataKey(i);
  size_t written = keyPrefs.putBytes(k.c_str(), blob.get(), blobSize);
  keyPrefs.end();

  return written == blobSize;
}

bool loadRawFromSlot(uint8_t i, RawCode &out)
{
  keyPrefs.begin("keys", true);
  String k = keyDataKey(i);
  size_t sz = keyPrefs.getBytesLength(k.c_str());
  if (sz < sizeof(uint16_t) * 2)
  {
    keyPrefs.end();
    return false;
  }
  std::unique_ptr<uint8_t[]> blob(new uint8_t[sz]);
  size_t got = keyPrefs.getBytes(k.c_str(), blob.get(), sz);
  keyPrefs.end();
  if (got != sz)
    return false;

  const uint8_t *p = blob.get();
  uint16_t freq = 0, len = 0;
  memcpy(&freq, p, sizeof(uint16_t));
  p += sizeof(uint16_t);
  memcpy(&len, p, sizeof(uint16_t));
  p += sizeof(uint16_t);

  if (len == 0 || len > MAX_RAW_LEN)
    return false;
  if (sizeof(uint16_t) * 2 + sizeof(int16_t) * len != sz)
    return false;

  out.freq_khz = freq;
  out.len = len;
  memcpy(out.data, p, sizeof(int16_t) * len);
  return true;
}

int findFreeSlot()
{
  for (int i = 0; i < MAX_KEYS; i++)
    if (!keys[i].used)
      return i;
  return -1;
}

void wifiLoadConfig(String &staSsid, String &staPass)
{
  prefs.begin("wifi", true);
  staSsid = prefs.getString("sta_ssid", "");
  staPass = prefs.getString("sta_pass", "");
  prefs.end();
}

void wifiSaveConfig(const String &staSsid, const String &staPass)
{
  prefs.begin("wifi", false);
  prefs.putString("sta_ssid", staSsid);
  prefs.putString("sta_pass", staPass);
  prefs.end();
}