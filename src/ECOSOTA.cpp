#include "ECOSOTA.h"
#include <WiFiClientSecure.h>

void ECOSOTA::begin(const char* hostname, const char* password) {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(password);
  ArduinoOTA.onStart([this]() {
    Serial.println("[OTA] iniciando...");
    if (onStart) onStart();
  });
  ArduinoOTA.onEnd([this]() {
    Serial.println("\n[OTA] concluído");
    if (onEnd) onEnd();
  });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
    Serial.printf("[OTA] %u%%\r", p*100/t);
  });
  ArduinoOTA.onError([this](ota_error_t e) {
    Serial.printf("[OTA] erro [%u]\n", e);
    if (onError) onError();
  });
  ArduinoOTA.begin();
  Serial.printf("[OTA] push pronto — hostname: %s\n", hostname);
}

void ECOSOTA::update() {
  ArduinoOTA.handle();
}

void ECOSOTA::checkHttp(const char* baseUrl, const char* deviceId, int ledPin) {
  char checkUrl[200], downUrl[200];
  snprintf(checkUrl, sizeof(checkUrl), "%s/check?device_id=%s",    baseUrl, deviceId);
  snprintf(downUrl,  sizeof(downUrl),  "%s/download?device_id=%s", baseUrl, deviceId);

  WiFiClientSecure secChk; secChk.setInsecure();
  HTTPClient http; http.begin(secChk, checkUrl);
  int code = http.GET();
  if (code != 200) { http.end(); return; }
  String body = http.getString(); http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body)) return;
  if (!doc["atualizar"].as<bool>()) return;

  // Não reflacha se for a mesma versão que já foi baixada
  String mod = doc["modificado"] | "";
  if (mod.length() > 0 && mod == _lastMod) {
    Serial.println("[OTA-HTTP] já na versão atual");
    return;
  }

  Serial.println("[OTA-HTTP] nova versão disponível, baixando...");
  if (onStart) onStart();

  _lastMod = mod; // guarda antes de baixar — evita loop se rebootOnUpdate=false
  WiFiClientSecure secDl; secDl.setInsecure();
  if (ledPin >= 0) httpUpdate.setLedPin(ledPin, HIGH);
  httpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret = httpUpdate.update(secDl, downUrl);
  if (ret == HTTP_UPDATE_FAILED) {
    _lastMod = ""; // reset para tentar de novo na próxima chamada
    Serial.printf("[OTA-HTTP] erro: %s\n", httpUpdate.getLastErrorString().c_str());
    if (onError) onError();
  }
}
