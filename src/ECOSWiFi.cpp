#include "ECOSWiFi.h"
#include "ECOSWatchdog.h"

// ─── Portal HTML ─────────────────────────────────────────────────────────────
static const char PORTAL_STYLE[] PROGMEM =
  "<!DOCTYPE html><html><head><meta charset='utf-8'>"
  "<meta name='viewport' content='width=device-width,initial-scale=1'>"
  "<title>ECOS Setup</title><style>"
  "*{box-sizing:border-box;margin:0;padding:0}"
  "body{background:#0f172a;color:#e2e8f0;font-family:-apple-system,sans-serif;"
  "min-height:100vh;display:flex;align-items:center;justify-content:center;padding:16px}"
  ".card{background:#1e293b;border-radius:16px;padding:24px;width:100%;max-width:420px}"
  ".logo{display:flex;align-items:center;gap:10px;margin-bottom:20px}"
  ".dot{width:10px;height:10px;background:#22c55e;border-radius:50%}"
  "h1{font-size:1.1rem;font-weight:700}"
  "h2{font-size:.78rem;color:#94a3b8;margin:16px 0 6px;border-bottom:1px solid #334155;"
  "padding-bottom:3px;text-transform:uppercase;letter-spacing:.06em}"
  "label{display:block;font-size:.73rem;color:#94a3b8;margin:8px 0 3px}"
  "input[type=text],input[type=password],input[type=number]{"
  "width:100%;padding:8px 12px;background:#0f172a;border:1px solid #334155;"
  "border-radius:8px;color:#f1f5f9;font-size:.875rem;outline:none}"
  "input:focus{border-color:#22c55e}"
  ".pw{position:relative}.pw input{padding-right:38px}"
  ".eye{position:absolute;right:8px;top:50%;transform:translateY(-50%);"
  "background:none;border:none;color:#64748b;cursor:pointer;font-size:1rem;padding:2px}"
  ".row2{display:grid;grid-template-columns:1fr 80px;gap:8px}"
  ".scan{background:#1e40af;border:none;color:#fff;padding:6px 10px;"
  "border-radius:6px;font-size:.78rem;cursor:pointer;margin-bottom:4px;width:100%}"
  ".nets{display:none;background:#0f172a;border:1px solid #334155;border-radius:8px;"
  "margin-bottom:6px;max-height:140px;overflow-y:auto}"
  ".net{padding:7px 12px;cursor:pointer;font-size:.83rem;"
  "border-bottom:1px solid #1e293b;display:flex;justify-content:space-between}"
  ".net:hover{background:#1e293b}.rssi{font-size:.68rem;color:#64748b}"
  ".opt{font-size:.7rem;color:#64748b;font-weight:400}"
  "button[type=submit]{margin-top:20px;width:100%;padding:12px;"
  "background:#22c55e;border:none;border-radius:10px;color:#0f172a;"
  "font-size:.95rem;font-weight:700;cursor:pointer}"
  "</style></head><body><div class='card'>";

static const char PORTAL_SCRIPT[] PROGMEM =
  "<script>"
  "function tp(id){var e=document.getElementById(id);"
  "e.type=e.type==='password'?'text':'password';}"
  "function scan(fid,nid){"
  "var d=document.getElementById(nid);"
  "d.innerHTML='<div class=\"net\">Buscando...</div>';d.style.display='block';"
  "fetch('/scan').then(r=>r.json()).then(list=>{"
  "d.innerHTML=list.length?"
  "list.map(x=>'<div class=\"net\" onclick=\"document.getElementById(\\''+fid+'\\').value=\\''+x.s+'\\'"
  ";document.getElementById(\\''+nid+'\\').style.display=\\'none\\'\">"
  "'+x.s+'<span class=\"rssi\">'+x.r+'dBm</span></div>').join(''):"
  "'<div class=\"net\">Nenhuma rede</div>';"
  "}).catch(()=>d.innerHTML='<div class=\"net\">Erro</div>');}"
  "</script></body></html>";

// ─── begin ────────────────────────────────────────────────────────────────────
void ECOSWiFi::begin(bool forceAP) {
  if (strlen(_cfg.ssid) == 0 || forceAP) {
    _noSsidMode = true;
    ensureAP();
    return;
  }
  WiFi.mode(WIFI_STA);
  _connectBest();
  uint32_t t = millis();
  Serial.print("[WiFi] aguardando");
  while (WiFi.status() != WL_CONNECTED && millis()-t < ECOS_WIFI_TIMEOUT_MS) {
    delay(500); Serial.print("."); ecosWatchdogReset();
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] %s — %s\n", activeSSID, WiFi.localIP().toString().c_str());
    _failCount = 0; _wasConnected = true;
    if (onConnect) onConnect();
  } else {
    Serial.println("\n[WiFi] timeout — abrindo AP");
    ensureAP();
  }
}

// ─── update ───────────────────────────────────────────────────────────────────
void ECOSWiFi::update(uint32_t now) {
  if (_dnsRunning) _dns.processNextRequest();
  if (_noSsidMode) return;
  if (now - _tCheck < ECOS_WIFI_CHECK_MS) return;
  _tCheck = now;

  if (WiFi.status() != WL_CONNECTED) {
    _failCount++;
    Serial.printf("[WiFi] desconectado (falha %d)\n", _failCount);
    if (_wasConnected && onDisconnect) { onDisconnect(); _wasConnected = false; }
    ensureAP();
    if (_failCount >= ECOS_WIFI_MAX_FAILS) {
      WiFi.disconnect(); delay(500); _connectBest(); _failCount = 0;
    } else {
      WiFi.reconnect();
    }
  } else {
    if (!_wasConnected) {
      _wasConnected = true; _failCount = 0;
      Serial.printf("[WiFi] reconectado — %s\n", WiFi.localIP().toString().c_str());
      if (_apRunning) closeAP();
      if (onConnect) onConnect();
    }
  }
}

// ─── AP helpers ───────────────────────────────────────────────────────────────
void ECOSWiFi::ensureAP() {
  if (_apRunning) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ECOS_AP_SSID, ECOS_AP_PASS);
  _apRunning = true;
  _server.begin();  // lwIP já existe após WiFi.mode() — seguro inicializar aqui
  if (!_dnsRunning) {
    _dns.start(53, "*", WiFi.softAPIP());
    _dnsRunning = true;
  }
  Serial.printf("[AP] %s aberto (%s) — DNS captivo ativo\n", ECOS_AP_SSID, WiFi.softAPIP().toString().c_str());
}

void ECOSWiFi::closeAP() {
  if (!_apRunning) return;
  WiFi.softAPdisconnect(false);
  _apRunning = false;
  if (_dnsRunning) { _dns.stop(); _dnsRunning = false; }
  Serial.println("[AP] fechado");
}

// ─── connectBest ─────────────────────────────────────────────────────────────
bool ECOSWiFi::_connectBest() {
  if (strlen(_cfg.ssid) == 0) return false;
  bool has2 = strlen(_cfg.ssid2) > 0;
  if (!has2) {
    WiFi.begin(_cfg.ssid, _cfg.password);
    strncpy(activeSSID, _cfg.ssid, sizeof(activeSSID));
    return true;
  }
  Serial.println("[WiFi] scan para melhor rede...");
  int n = WiFi.scanNetworks(false, true);
  int r1 = -999, r2 = -999;
  for (int i = 0; i < n; i++) {
    String s = WiFi.SSID(i);
    if (s == String(_cfg.ssid))  r1 = max(r1, (int)WiFi.RSSI(i));
    if (s == String(_cfg.ssid2)) r2 = max(r2, (int)WiFi.RSSI(i));
    ecosWatchdogReset();
  }
  WiFi.scanDelete();
  Serial.printf("[WiFi] %s=%ddBm | %s=%ddBm\n", _cfg.ssid, r1, _cfg.ssid2, r2);
  if (r2 > r1 && r2 > -999) {
    WiFi.begin(_cfg.ssid2, _cfg.password2);
    strncpy(activeSSID, _cfg.ssid2, sizeof(activeSSID));
  } else {
    WiFi.begin(_cfg.ssid, _cfg.password);
    strncpy(activeSSID, _cfg.ssid, sizeof(activeSSID));
  }
  return true;
}

// ─── Auth helper ─────────────────────────────────────────────────────────────
bool ECOSWiFi::checkAuth() {
  if (_noSsidMode) return true;          // primeiro boot: sem bloqueio
  if (strlen(_cfg.portal_user) == 0) return true;  // sem usuário: aberto
  if (!_server.authenticate(_cfg.portal_user, _cfg.portal_pass)) {
    _server.requestAuthentication(BASIC_AUTH, "ECOS");
    return false;
  }
  return true;
}

// ─── Portal routes ────────────────────────────────────────────────────────────
void ECOSWiFi::setupPortalRoutes(std::function<void(ECOSConfig&)> onSaved) {
  _onSaved = onSaved;

  _server.on("/", [this]() {
    if (!checkAuth()) return;
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/html", "");
    _server.sendContent(FPSTR(PORTAL_STYLE));
    _server.sendContent(F("<div class='logo'><div class='dot'></div><h1>ECOS Setup</h1></div>"
                          "<form method='POST' action='/save'>"));
    // Device
    _server.sendContent(F("<h2>Dispositivo</h2>"
                          "<label>Device ID</label>"
                          "<input type='text' name='device_id' placeholder='ex: inv-01' value='"));
    _server.sendContent(_cfg.device_id);
    _server.sendContent(F("'><label>Tipo</label>"
                          "<input type='text' name='device_type' placeholder='gateway/inversor/irrigacao' value='"));
    _server.sendContent(_cfg.device_type);
    _server.sendContent(F("'>"));
    // WiFi principal
    _server.sendContent(F("<h2>WiFi Principal</h2>"
                          "<button type='button' class='scan' onclick='scan(\"s1\",\"n1\")'>&#x1F4F6; Buscar redes</button>"
                          "<div id='n1' class='nets'></div>"
                          "<label>SSID</label><input type='text' id='s1' name='ssid' value='"));
    _server.sendContent(_cfg.ssid);
    _server.sendContent(F("'><label>Senha</label><div class='pw'>"
                          "<input type='password' id='p1' name='pass' placeholder='(manter vazio = sem alteração)'>"
                          "<button type='button' class='eye' onclick='tp(\"p1\")'>&#x1F441;</button></div>"));
    // WiFi secundário
    _server.sendContent(F("<h2>WiFi Secundária <span class='opt'>(failover)</span></h2>"
                          "<button type='button' class='scan' onclick='scan(\"s2\",\"n2\")'>&#x1F4F6; Buscar redes</button>"
                          "<div id='n2' class='nets'></div>"
                          "<label>SSID</label><input type='text' id='s2' name='ssid2' value='"));
    _server.sendContent(_cfg.ssid2);
    _server.sendContent(F("'><label>Senha</label><div class='pw'>"
                          "<input type='password' id='p2' name='pass2' placeholder='(manter vazio = sem alteração)'>"
                          "<button type='button' class='eye' onclick='tp(\"p2\")'>&#x1F441;</button></div>"));
    // MQTT
    _server.sendContent(F("<h2>Servidor MQTT</h2><div class='row2'>"
                          "<input type='text' name='host' placeholder='host' value='"));
    _server.sendContent(_cfg.mqtt_host);
    _server.sendContent(F("'><input type='number' name='port' value='"));
    char portBuf[8]; snprintf(portBuf, sizeof(portBuf), "%u", _cfg.mqtt_port);
    _server.sendContent(portBuf);
    _server.sendContent(F("'></div>"));
    // Mesh WiFi
    _server.sendContent(F("<h2>Rede Mesh WiFi</h2>"
                          "<label>SSID da Mesh</label>"
                          "<input type='text' name='mesh_ssid' value='"));
    _server.sendContent(_cfg.mesh_ssid);
    _server.sendContent(F("'><label>Senha da Mesh</label><div class='pw'>"
                          "<input type='password' id='pm' name='mesh_pass' placeholder='(manter vazio = sem alteração)'>"
                          "<button type='button' class='eye' onclick='tp(\"pm\")'>&#x1F441;</button></div>"));
    // Segurança portal
    _server.sendContent(F("<h2>Acesso ao Portal</h2>"
                          "<label>Usuário</label>"
                          "<input type='text' name='p_user' placeholder='admin' value='"));
    _server.sendContent(_cfg.portal_user);
    _server.sendContent(F("'><label>Senha do portal</label><div class='pw'>"
                          "<input type='password' id='ppu' name='p_pass' placeholder='(manter vazio = sem alteração)'>"
                          "<button type='button' class='eye' onclick='tp(\"ppu\")'>&#x1F441;</button></div>"
                          "<h2>MQTT Broker</h2>"
                          "<label>Usuário MQTT</label>"
                          "<input type='text' name='m_user' value='"));
    _server.sendContent(_cfg.mqtt_user);
    _server.sendContent(F("'><label>Senha MQTT</label><div class='pw'>"
                          "<input type='password' id='pmu' name='m_pass' placeholder='(manter vazio = sem alteração)'>"
                          "<button type='button' class='eye' onclick='tp(\"pmu\")'>&#x1F441;</button></div>"));
    // OTA
    _server.sendContent(F("<h2>OTA</h2><label>Senha OTA</label><div class='pw'>"
                          "<input type='password' id='po' name='ota_pass' value='"));
    _server.sendContent(_cfg.ota_pass);
    _server.sendContent(F("'><button type='button' class='eye' onclick='tp(\"po\")'>&#x1F441;</button></div>"));
    _server.sendContent(F("<button type='submit'>Salvar e Reiniciar</button></form></div>"));
    _server.sendContent(FPSTR(PORTAL_SCRIPT));
  });

  _server.on("/scan", [this]() {
    if (!checkAuth()) return;
    int n = WiFi.scanNetworks(false, true);
    String j = "[";
    for (int i = 0; i < n; i++) {
      if (i) j += ",";
      String s = WiFi.SSID(i); s.replace("\"","\\\"");
      j += "{\"s\":\"" + s + "\",\"r\":" + WiFi.RSSI(i) + "}";
      ecosWatchdogReset();
    }
    j += "]"; WiFi.scanDelete();
    _server.send(200, "application/json", j);
  });

  _server.on("/save", HTTP_POST, [this]() {
    if (!checkAuth()) return;
    auto arg = [this](const char* n) { return _server.arg(n); };
    auto has = [this](const char* n) { return _server.hasArg(n); };

    if (has("device_id"))   arg("device_id").toCharArray(_cfg.device_id,   sizeof(_cfg.device_id));
    if (has("device_type")) arg("device_type").toCharArray(_cfg.device_type, sizeof(_cfg.device_type));
    if (has("ssid"))        arg("ssid").toCharArray(_cfg.ssid, sizeof(_cfg.ssid));
    if (has("pass")  && arg("pass").length()  > 0) arg("pass").toCharArray(_cfg.password,  sizeof(_cfg.password));
    if (has("ssid2"))       arg("ssid2").toCharArray(_cfg.ssid2, sizeof(_cfg.ssid2));
    if (has("pass2") && arg("pass2").length() > 0) arg("pass2").toCharArray(_cfg.password2, sizeof(_cfg.password2));
    if (has("host"))        arg("host").toCharArray(_cfg.mqtt_host, sizeof(_cfg.mqtt_host));
    if (has("port"))        _cfg.mqtt_port = arg("port").toInt();
    if (has("mesh_ssid") && arg("mesh_ssid").length() > 0) arg("mesh_ssid").toCharArray(_cfg.mesh_ssid,     sizeof(_cfg.mesh_ssid));
    if (has("mesh_pass") && arg("mesh_pass").length() > 0) arg("mesh_pass").toCharArray(_cfg.mesh_password,  sizeof(_cfg.mesh_password));
    if (has("ota_pass")  && arg("ota_pass").length()  > 0) arg("ota_pass").toCharArray(_cfg.ota_pass,        sizeof(_cfg.ota_pass));
    if (has("p_user")    && arg("p_user").length()    > 0) arg("p_user").toCharArray(_cfg.portal_user,       sizeof(_cfg.portal_user));
    if (has("p_pass")    && arg("p_pass").length()    > 0) arg("p_pass").toCharArray(_cfg.portal_pass,       sizeof(_cfg.portal_pass));
    if (has("m_user"))                                      arg("m_user").toCharArray(_cfg.mqtt_user,         sizeof(_cfg.mqtt_user));
    if (has("m_pass")    && arg("m_pass").length()    > 0) arg("m_pass").toCharArray(_cfg.mqtt_pass,         sizeof(_cfg.mqtt_pass));

    ecosConfigSave(_cfg);
    if (_onSaved) _onSaved(_cfg);

    _server.send(200, "text/html",
      "<html><head><meta charset='utf-8'><style>"
      "body{background:#0f172a;color:#e2e8f0;font-family:-apple-system,sans-serif;"
      "min-height:100vh;display:flex;align-items:center;justify-content:center;text-align:center}"
      ".ok{font-size:2.5rem}.msg{color:#22c55e;font-weight:700}"
      ".sub{color:#64748b;font-size:.8rem;margin-top:8px}"
      "</style></head><body><div>"
      "<div class='ok'>&#10003;</div><div class='msg'>Salvo!</div>"
      "<div class='sub'>Reiniciando...</div></div></body></html>");
    delay(1500); ESP.restart();
  });
}
