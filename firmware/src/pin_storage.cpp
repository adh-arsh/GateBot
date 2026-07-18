#include <Preferences.h>
#include <mbedtls/sha256.h>
#include <string.h>
#include "pin_storage.h"
#include "config.h"

static Preferences pinPrefs;
static const char* PIN_NS = "gbpins";

static bool isDigits6(const char* pin) {
  if (!pin || strlen(pin) != PIN_LENGTH) return false;
  for (int i = 0; i < PIN_LENGTH; i++) {
    if (pin[i] < '0' || pin[i] > '9') return false;
  }
  return true;
}

static void sha256Hex(const char* input, char outHex[65]) {
  unsigned char hash[32];
  mbedtls_sha256_ret((const unsigned char*)input, strlen(input), hash, 0);
  static const char* hex = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    outHex[i * 2] = hex[(hash[i] >> 4) & 0xF];
    outHex[i * 2 + 1] = hex[hash[i] & 0xF];
  }
  outHex[64] = '\0';
}

static String keyUsed(uint8_t id) { return "u" + String(id); }
static String keyHash(uint8_t id) { return "h" + String(id); }
static String keyLabel(uint8_t id) { return "l" + String(id); }
static String keyCreated(uint8_t id) { return "c" + String(id); }

void pinStorageBegin() {
  pinPrefs.begin(PIN_NS, false);
}

int pinStorageCount() {
  int n = 0;
  for (uint8_t i = 0; i < PIN_MAX_COUNT; i++) {
    if (pinPrefs.getBool(keyUsed(i).c_str(), false)) n++;
  }
  return n;
}

bool pinStorageList(PinEntry* out, int maxOut, int* countOut) {
  if (!out || !countOut || maxOut <= 0) return false;
  int n = 0;
  for (uint8_t i = 0; i < PIN_MAX_COUNT && n < maxOut; i++) {
    if (!pinPrefs.getBool(keyUsed(i).c_str(), false)) continue;
    out[n].id = i;
    out[n].used = true;
    out[n].createdAt = pinPrefs.getUInt(keyCreated(i).c_str(), 0);
    String lab = pinPrefs.getString(keyLabel(i).c_str(), "");
    strncpy(out[n].label, lab.c_str(), PIN_LABEL_MAX);
    out[n].label[PIN_LABEL_MAX] = '\0';
    n++;
  }
  *countOut = n;
  return true;
}

int pinStorageCreate(const char* pin6, const char* label, char* errMsg, size_t errLen) {
  if (!isDigits6(pin6)) {
    snprintf(errMsg, errLen, "pin must be %d digits", PIN_LENGTH);
    return -1;
  }
  if (pinStorageCount() >= PIN_MAX_COUNT) {
    snprintf(errMsg, errLen, "pin limit reached (%d)", PIN_MAX_COUNT);
    return -1;
  }

  char hashHex[65];
  sha256Hex(pin6, hashHex);

  // Reject duplicates
  for (uint8_t i = 0; i < PIN_MAX_COUNT; i++) {
    if (!pinPrefs.getBool(keyUsed(i).c_str(), false)) continue;
    String existing = pinPrefs.getString(keyHash(i).c_str(), "");
    if (existing.equals(hashHex)) {
      snprintf(errMsg, errLen, "pin already exists");
      return -1;
    }
  }

  int slot = -1;
  for (uint8_t i = 0; i < PIN_MAX_COUNT; i++) {
    if (!pinPrefs.getBool(keyUsed(i).c_str(), false)) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    snprintf(errMsg, errLen, "no free slot");
    return -1;
  }

  String lab = label ? String(label) : "";
  lab.trim();
  if (lab.length() > PIN_LABEL_MAX) lab = lab.substring(0, PIN_LABEL_MAX);
  if (lab.length() == 0) lab = "PIN " + String(slot);

  pinPrefs.putString(keyHash(slot).c_str(), hashHex);
  pinPrefs.putString(keyLabel(slot).c_str(), lab);
  pinPrefs.putUInt(keyCreated(slot).c_str(), millis());
  pinPrefs.putBool(keyUsed(slot).c_str(), true);

  Serial.printf("[pins] created id=%d label=%s\n", slot, lab.c_str());
  return slot;
}

bool pinStorageDelete(uint8_t id) {
  if (id >= PIN_MAX_COUNT) return false;
  if (!pinPrefs.getBool(keyUsed(id).c_str(), false)) return false;
  pinPrefs.putBool(keyUsed(id).c_str(), false);
  pinPrefs.remove(keyHash(id).c_str());
  pinPrefs.remove(keyLabel(id).c_str());
  pinPrefs.remove(keyCreated(id).c_str());
  Serial.printf("[pins] deleted id=%d\n", id);
  return true;
}

bool pinStorageVerify(const char* pin6) {
  if (!isDigits6(pin6)) return false;
  char hashHex[65];
  sha256Hex(pin6, hashHex);
  for (uint8_t i = 0; i < PIN_MAX_COUNT; i++) {
    if (!pinPrefs.getBool(keyUsed(i).c_str(), false)) continue;
    String existing = pinPrefs.getString(keyHash(i).c_str(), "");
    if (existing.equals(hashHex)) return true;
  }
  return false;
}
