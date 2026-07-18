#pragma once

#include <Arduino.h>
#include "config.h"

struct PinEntry {
  uint8_t id;
  bool used;
  char label[PIN_LABEL_MAX + 1];
  uint32_t createdAt;  // millis at create (informational)
};

void pinStorageBegin();
int pinStorageCount();
bool pinStorageList(PinEntry* out, int maxOut, int* countOut);
// Returns new id (>=0) or -1 on error. errMsg must be writable.
int pinStorageCreate(const char* pin6, const char* label, char* errMsg, size_t errLen);
bool pinStorageDelete(uint8_t id);
bool pinStorageVerify(const char* pin6);
