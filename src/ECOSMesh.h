#pragma once
#include <Arduino.h>
#include <painlessMesh.h>
#include <map>
#include <functional>
#include "ECOSConfig.h"

#ifndef ECOS_MESH_PORT
#define ECOS_MESH_PORT 5555
#endif
#ifndef ECOS_MESH_RSSI_MIN
#define ECOS_MESH_RSSI_MIN (-72)
#endif

// Formato JSON da mensagem mesh:
// {"id":"dev-01","type":"DATA","p":"{...}"}
// {"id":"dev-01","type":"REGISTER"}

class ECOSMesh {
public:
  using PacketFn  = std::function<void(const char* devId, const char* type, const char* payload)>;
  using NodeFn    = std::function<void(uint32_t nodeId)>;

  explicit ECOSMesh(ECOSConfig& cfg) : _cfg(cfg) {}

  void beginRoot(uint8_t channel);  // Gateway: root da mesh (AP para filhos, STA para roteador)
  void beginNode(uint8_t channel);  // Nó: conecta à mesh como filho
  void stop();
  void update();   // chame no loop()

  // Envia para nó específico pelo device_id string
  bool sendToId(const String& devId, const char* type, const char* payload);
  // Broadcast para todos os nós
  bool broadcast(const char* type, const char* payload);

  bool     running()   const { return _running; }
  uint32_t nodeCount() const { return _count; }
  uint32_t myNodeId()        { return _running ? _mesh.getNodeId() : 0; }

  // Callbacks
  PacketFn onPacket;
  NodeFn   onNodeJoin;
  NodeFn   onNodeLeave;

private:
  ECOSConfig&   _cfg;
  Scheduler     _sched;
  painlessMesh  _mesh;
  bool          _running = false;
  uint32_t      _count   = 0;
  std::map<uint32_t, String> _devMap;  // nodeId → device_id

  void _onReceive(uint32_t from, String& raw);
  void _onNew(uint32_t id);
  void _onDrop(uint32_t id);
  void _initCallbacks();
  String _buildMsg(const char* devId, const char* type, const char* payload);
};
