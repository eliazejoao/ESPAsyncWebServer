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
    (uint8_t)(mac >> 40), (uint8_t)(mac >> 32), (uint8_t)(mac >> 24),
    (uint8_t)(mac >> 16), (uint8_t)(mac >> 8),  (uint8_t)(mac));
  return String(buf);
}

void ecosAutoId(ECOSConfig& cfg) {
  if (strlen(cfg.device_id) == 0) {
    String id = "ecos_" + ecosChipId();
    id.toCharArray(cfg.device_id, sizeof(cfg.device_id));
    Serial.printf("[Config] device_id auto: %s\n", cfg.device_id);
  }
}

bool ecosConfigMigrateV15(ECOSConfig& cfg) {
  Preferences p;
  p.begin("ecos", true);
  bool hasBlob = (p.getBytesLength("cfg") == sizeof(ECOSConfig));
  bool hasV15  = (p.getString("ssid", "").length() > 0 ||
                  p.getString("device_id", "").length() > 0);
  // Blob existe mas ssid está vazio = blob inválido (v16 nunca configurado)
  bool blobEmpty = hasBlob && strlen(cfg.ssid) == 0;
  if ((hasBlob && !blobEmpty) || !hasV15) { p.end(); return false; }

  p.getString("ssid",      "").toCharArray(cfg.ssid,      sizeof(cfg.ssid));
  p.getString("pass",      "").toCharArray(cfg.password,  sizeof(cfg.password));
  p.getString("device_id", "").toCharArray(cfg.device_id, sizeof(cfg.device_id));
  p.getString("mqtt_host", "ecossolar.com").toCharArray(cfg.mqtt_host, sizeof(cfg.mqtt_host));
  cfg.mqtt_port = p.getUShort("mqtt_port", 443);
  p.getString("mqtt_path", "/ws").toCharArray(cfg.mqtt_path, sizeof(cfg.mqtt_path));
  cfg.use_ssl   = p.getBool("use_ssl", true);
  p.getString("ota_pass",  "eco@ota").toCharArray(cfg.ota_pass, sizeof(cfg.ota_pass));
  p.end();

  ecosConfigSave(cfg);
  Serial.println("[Config] migrado do formato v15");
  return true;
}
