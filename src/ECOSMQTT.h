#pragma once
#include <Arduino.h>
#include <WebSocketsClient.h>
#include <functional>
#include <vector>
#include "ECOSConfig.h"

#ifndef ECOS_MQTT_PAYLOAD_MAX
#define ECOS_MQTT_PAYLOAD_MAX 512
#endif

class ECOSMQTT {
public:
  using MsgFn = std::function<void(const char* topic, const char* payload)>;
  using VoidFn = std::function<void()>;

  explicit ECOSMQTT(ECOSConfig& cfg) : _cfg(cfg) {}

  void begin();      // inicia conexão WebSocket/MQTT
  void update();     // chame no loop()
  bool publish(const char* topic, const char* payload, bool retain = false);
  bool subscribe(const char* topic);

  bool ready()     const { return _ready; }
  bool connected() const { return _connected; }

  // Callbacks — atribua antes de begin()
  MsgFn  onMessage;
  VoidFn onConnect;
  VoidFn onDisconnect;

private:
  ECOSConfig&      _cfg;
  WebSocketsClient _ws;
  bool             _connected = false;
  bool             _ready     = false;
  uint16_t         _pktId     = 1;
  uint32_t         _tPing     = 0;

  void _wsEvent(WStype_t ev, uint8_t* data, size_t len);
  void _send(const std::vector<uint8_t>& pkt);
  std::vector<uint8_t> _buildConnect(const char* cid);
  std::vector<uint8_t> _buildPublish(const char* t, const char* p, bool retain);
  std::vector<uint8_t> _buildSubscribe(const char* t);
  static void _varInt(std::vector<uint8_t>& b, int v);
  static void _u16b(std::vector<uint8_t>& b, uint16_t n);
};
