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

  void begin(const char* hostname, const char* password);
  void update();

  // Verifica e aplica atualização HTTP pull.
  // Compara "modificado" do servidor com o último download — evita reflachar mesma versão.
  void checkHttp(const char* baseUrl, const char* deviceId, int ledPin = -1);

  // Limpa o timestamp guardado (força re-download na próxima chamada)
  void resetLastMod() { _lastMod = ""; }

  VoidFn onStart;
  VoidFn onEnd;
  VoidFn onError;

private:
  String _lastMod = "";
};
