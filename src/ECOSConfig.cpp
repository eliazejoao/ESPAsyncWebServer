#include "ECOSConfig.h"
#include <WiFi.h>

void ecosConfigLoad(ECOSConfig& cfg, const char* ns) {
  Preferences p;
  p.begin(ns, true);
  if (p.getBytesLength("cfg") == sizeof(ECOSConfig))
    p.getBytes("cfg", &cfg, sizeof(ECOSConfig));
  p.end();
}

void ecosConfigSave(const ECOSConfig& cfg, const char* ns) {
  Preferences p;
  p.begin(ns, false);
  p.putBytes("cfg", &cfg, sizeof(ECOSConfig));
  p.end();
}

String ecosChipId() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[14];
  snprintf(buf, sizeof(buf), "%04X%08X",
    (uint32_t)(mac >> 32), (uint32_t)(mac & 0xFFFFFFFF));
  return String(buf);
}

String ecosMacString() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
    (uint8_t)(mac),       (uint8_t)(mac >> 8),  (uint8_t)(mac >> 16),
    (uint8_t)(mac >> 24), (uint8_t)(mac >> 32),  (uint8_t)(mac >> 40));
  return String(buf);
}

void ecosAutoId(ECOSConfig& cfg) {
  if (strlen(cfg.device_id) == 0) {
    String id = "ecos_" + ecosChipId();
    id.toCharArray(cfg.device_id, sizeof(cfg.device_id));
    Serial.printf("[Config] device_id auto: %s\n", cfg.device_id);
  }
}
