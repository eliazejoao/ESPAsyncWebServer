#include "ECOSOTA.h"

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

  NetworkClientSecure secChk; secChk.setInsecure();
  HTTPClient http; http.begin(secChk, checkUrl);
  int code = http.GET();
  if (code != 200) { http.end(); return; }
  String body = http.getString(); http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body)) return;
  if (!doc["atualizar"].as<bool>()) return;

  Serial.println("[OTA-HTTP] nova versão disponível, baixando...");
  if (onStart) onStart();

  NetworkClientSecure secDl; secDl.setInsecure();
  if (ledPin >= 0) httpUpdate.setLedPin(ledPin, HIGH);
  httpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret = httpUpdate.update(secDl, downUrl);
  if (ret == HTTP_UPDATE_FAILED) {
    Serial.printf("[OTA-HTTP] erro: %s\n", httpUpdate.getLastErrorString().c_str());
    if (onError) onError();
  }
}
