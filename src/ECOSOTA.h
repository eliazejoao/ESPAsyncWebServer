#pragma once
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <functional>

class ECOSOTA {
public:
  using VoidFn = std::function<void()>;

  // hostname: nome mDNS (ex: "ecos-gateway"), password: senha OTA
  void begin(const char* hostname, const char* password);
  void update();  // chame no loop() — processa ArduinoOTA

  // Verifica e aplica atualização HTTP
  // baseUrl: "https://ecossolar.com/api/firmware"
  // deviceId: "ecos-gateway"
  void checkHttp(const char* baseUrl, const char* deviceId, int ledPin = -1);

  VoidFn onStart;
  VoidFn onEnd;
  VoidFn onError;
};
