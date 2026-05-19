#include "ECOSMQTT.h"

void ECOSMQTT::_varInt(std::vector<uint8_t>& b, int v) {
  do { uint8_t byte = v & 0x7F; v >>= 7; if (v) byte |= 0x80; b.push_back(byte); } while (v);
}
void ECOSMQTT::_u16b(std::vector<uint8_t>& b, uint16_t n) { b.push_back(n>>8); b.push_back(n&0xFF); }

std::vector<uint8_t> ECOSMQTT::_buildConnect(const char* cid) {
  bool hasUser = strlen(_cfg.mqtt_user) > 0;
  bool hasPass = strlen(_cfg.mqtt_pass) > 0;
  // Connect flags: clean session + user flag + pass flag
  uint8_t flags = 0x02;
  if (hasUser) flags |= 0x80;
  if (hasPass) flags |= 0x40;

  std::vector<uint8_t> pl;
  const uint8_t hdr[] = {0,4,'M','Q','T','T',0x04,flags,0x00,0x3C};
  pl.insert(pl.end(), hdr, hdr+sizeof(hdr));
  _u16b(pl, strlen(cid));
  for (const char* c=cid;            *c; c++) pl.push_back(*c);
  if (hasUser) { _u16b(pl, strlen(_cfg.mqtt_user)); for (const char* c=_cfg.mqtt_user; *c; c++) pl.push_back(*c); }
  if (hasPass) { _u16b(pl, strlen(_cfg.mqtt_pass)); for (const char* c=_cfg.mqtt_pass; *c; c++) pl.push_back(*c); }

  std::vector<uint8_t> pkt; pkt.push_back(0x10);
  _varInt(pkt, pl.size()); pkt.insert(pkt.end(), pl.begin(), pl.end());
  return pkt;
}

std::vector<uint8_t> ECOSMQTT::_buildPublish(const char* t, const char* p, bool retain) {
  std::vector<uint8_t> body;
  _u16b(body, strlen(t));
  for (const char* c=t; *c; c++) body.push_back(*c);
  for (const char* c=p; *c; c++) body.push_back(*c);
  std::vector<uint8_t> pkt; pkt.push_back(retain ? 0x31 : 0x30);
  _varInt(pkt, body.size()); pkt.insert(pkt.end(), body.begin(), body.end());
  return pkt;
}

std::vector<uint8_t> ECOSMQTT::_buildSubscribe(const char* t) {
  std::vector<uint8_t> body;
  _u16b(body, _pktId++); _u16b(body, strlen(t));
  for (const char* c=t; *c; c++) body.push_back(*c);
  body.push_back(0x00);
  std::vector<uint8_t> pkt; pkt.push_back(0x82);
  _varInt(pkt, body.size()); pkt.insert(pkt.end(), body.begin(), body.end());
  return pkt;
}

void ECOSMQTT::_send(const std::vector<uint8_t>& pkt) {
  if (_connected) _ws.sendBIN(pkt.data(), pkt.size());
}

void ECOSMQTT::_wsEvent(WStype_t ev, uint8_t* data, size_t len) {
  switch (ev) {
    case WStype_DISCONNECTED:
      _connected = false; _ready = false;
      Serial.println("[MQTT] desconectado");
      if (onDisconnect) onDisconnect();
      break;
    case WStype_CONNECTED:
      _connected = true;
      { char cid[48]; snprintf(cid, sizeof(cid), "ecos_%s_%lu", _cfg.device_id, millis()%100000);
        _send(_buildConnect(cid)); }
      break;
    case WStype_BIN:
      if (len < 2) break;
      if ((data[0] & 0xF0) == 0x20) {  // CONNACK
        _ready = (len >= 4 && data[3] == 0);
        if (_ready) {
          Serial.println("[MQTT] pronto");
          if (onConnect) onConnect();
        }
      } else if ((data[0] & 0xF0) == 0x30) {  // PUBLISH recebido
        if (len < 4) break;
        uint16_t tLen = (data[2]<<8)|data[3];
        if (tLen+4 > len) break;
        char topic[128]={}; memcpy(topic, data+4, min((size_t)tLen, sizeof(topic)-1));
        const char* pl = (const char*)(data+4+tLen);
        size_t plLen = len-4-tLen;
        char payload[256]={}; memcpy(payload, pl, min(plLen, sizeof(payload)-1));
        if (onMessage) onMessage(topic, payload);
      }
      break;
    default: break;
  }
}

void ECOSMQTT::begin() {
  if (_cfg.use_ssl) _ws.beginSSL(_cfg.mqtt_host, _cfg.mqtt_port, _cfg.mqtt_path, "", "mqtt");
  else              _ws.begin(_cfg.mqtt_host, _cfg.mqtt_port, _cfg.mqtt_path, "mqtt");
  _ws.onEvent([this](WStype_t e, uint8_t* d, size_t l){ _wsEvent(e, d, l); });
  _ws.setReconnectInterval(5000);
  Serial.printf("[MQTT] %s:%d%s ssl=%d\n", _cfg.mqtt_host, _cfg.mqtt_port, _cfg.mqtt_path, _cfg.use_ssl);
}

void ECOSMQTT::update() {
  _ws.loop();
  if (_ready && millis()-_tPing >= 15000) {
    _tPing = millis();
    _send({0xC0, 0x00});
  }
}

bool ECOSMQTT::publish(const char* topic, const char* payload, bool retain) {
  if (!_ready) return false;
  _send(_buildPublish(topic, payload, retain));
  return true;
}

bool ECOSMQTT::subscribe(const char* topic) {
  if (!_connected) return false;
  _send(_buildSubscribe(topic));
  return true;
}
