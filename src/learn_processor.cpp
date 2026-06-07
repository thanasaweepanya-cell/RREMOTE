#include "learn_processor.h"
#include "config.h"
#include "models.h"
#include "ir_utils.h"
#include "storage.h"

extern LearnState learn;
extern KeyMeta keys[MAX_KEYS];

void learnTick()
{
  // ไม่ได้อยู่ในโหมดเรียนรู้
  if (!learn.active)
    return;

  // หมดเวลา Learning
  if (millis() - learn.startedAt >= LEARN_WINDOW_MS)
  {
    learn.active = false;
    learn.got = 0;
    learn.slot = -1;
    learn.startedAt = 0;

    strlcpy(
        learn.status,
        "timeout",
        sizeof(learn.status));

    return;
  }

  RawCode code;

  // ยังไม่มีสัญญาณ IR
  if (!captureOnce(code))
    return;

  // เก็บตัวอย่าง
  learn.samples[learn.got] = code;
  learn.got++;

  // ยังเก็บไม่ครบ
  if (learn.got < LEARN_TRIES)
  {
    strlcpy(
        learn.status,
        "waiting",
        sizeof(learn.status));

    return;
  }

  // ตรวจสอบว่า RAW ทั้ง 3 ครั้งเหมือนกัน
  bool ok =
      rawEquals(learn.samples[0], learn.samples[1]) &&
      rawEquals(learn.samples[0], learn.samples[2]);

  if (!ok)
  {
    learn.got = 0;

    strlcpy(
        learn.status,
        "mismatch_try_again",
        sizeof(learn.status));

    return;
  }

  // ตรวจสอบ Slot
  int slot = learn.slot;

  if (slot < 0 || slot >= MAX_KEYS)
  {
    learn.active = false;
    learn.got = 0;
    learn.slot = -1;
    learn.startedAt = 0;

    strlcpy(
        learn.status,
        "error",
        sizeof(learn.status));

    return;
  }

  // บันทึก RAW ลง Flash
  bool saved = saveRawToSlot(
      slot,
      learn.samples[0]);

  if (!saved)
  {
    learn.active = false;
    learn.got = 0;
    learn.slot = -1;
    learn.startedAt = 0;

    strlcpy(
        learn.status,
        "save_failed",
        sizeof(learn.status));

    return;
  }

  // บันทึกข้อมูลปุ่ม
  keys[slot].used = true;

  strlcpy(
      keys[slot].name,
      learn.pendingName,
      sizeof(keys[slot].name));

  saveKeyMeta(slot);

  // เรียนรู้เสร็จ
  learn.active = false;
  learn.got = 0;
  learn.slot = -1;
  learn.startedAt = 0;

  strlcpy(
      learn.status,
      "saved",
      sizeof(learn.status));
}