#pragma once
#include <stdint.h>
#include "config.h"

// ====== Models ======
struct RawCode
{
  uint16_t freq_khz = IR_CARRIER_KHZ;
  uint16_t len = 0;
  int16_t data[MAX_RAW_LEN];
};

struct KeyMeta
{
  char id[16];   // "k00".."k19"
  char name[32]; // display
  bool used = false;
};

// Learning state
struct LearnState
{
  bool active = false;
  uint32_t startedAt = 0;
  char pendingName[32] = {0};
  int slot = -1;
  bool isAdvanced = false;  // Track if this is advanced learn

  RawCode samples[LEARN_TRIES];
  uint8_t got = 0;

  char status[64] = "idle";
};
