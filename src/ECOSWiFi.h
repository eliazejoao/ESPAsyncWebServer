#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <functional>
#include "ECOSConfig.h"

#ifndef ECOS_WIFI_TIMEOUT_MS
#define ECOS_WIFI_TIMEOUT_MS 30000
#endif
#ifndef ECOS_WIFI_CHECK_MS
#define ECOS_WIFI_CHECK_MS   30000
#endif
#ifndef ECOS_WIFI_MAX_FAILS
#define ECOS_WIFI_MAX_FAILS  3
#endif
#ifndef ECOS_AP_SSID
#define ECOS_AP_SSID "ECOS-Setup"
#endif
#ifndef ECOS_AP_PASS
#define ECOS_AP_PASS "eco@setup"
#endif

class ECOSWiFi {
public:
  ECOSWiFi(ECOSConfig& cfg, WebServer& server) : _cfg(cfg), _server(server) {}

  // Inicializa WiFi. forceAP=true força portal de configuração independente de SSID.
  void begin(bool forceAP = false);

  // Chame no loop() — gerencia reconexão e AP
  void update(uint32_t now);

  // Registra rotas do portal de configuração no WebServer
  // onSaved: chamado após salvar (cfg já atualizado) — use para reiniciar, etc.
  void setupPortalRoutes(std::function<void(ECOSConfig&)> onSaved = nullptr);

  bool isConnected()  const { return WiFi.status() == WL_CONNECTED; }
  bool apRunning()    const { return _apRunning; }
  bool noSsidMode()   const { return _noSsidMode; }
  uint8_t channel()   const { return (uint8_t)WiFi.channel(); }
  uint8_t failCount() const { return _failCount; }

  // Eventos — atribua lambdas antes de begin()
  std::function<void()> onConnect;
  std::function<void()> onDisconnect;

  void ensureAP();   // abre portal de configuração (AP)
  void closeAP();    // fecha portal de configuração

  char activeSSID[64] = "";

  // Retorna false e envia 401 se autenticação falhar
  // Em noSsidMode (primeiro boot) não exige auth
  bool checkAuth();

private:
  ECOSConfig& _cfg;
  WebServer&  _server;
  bool        _apRunning  = false;
  bool        _noSsidMode = false;
  uint8_t     _failCount  = 0;
  uint32_t    _tCheck     = 0;
  bool        _wasConnected = false;

  bool _connectBest();
  std::function<void(ECOSConfig&)> _onSaved;
};
