#include "ECOSMesh.h"

String ECOSMesh::_buildMsg(const char* devId, const char* type, const char* payload) {
  JsonDocument doc;
  doc["id"]   = devId;
  doc["type"] = type;
  if (payload && payload[0]) doc["p"] = payload;
  String out; serializeJson(doc, out);
  return out;
}

void ECOSMesh::_onReceive(uint32_t from, String& raw) {
  JsonDocument doc;
  if (deserializeJson(doc, raw) != DeserializationError::Ok) {
    Serial.printf("[Mesh] JSON inválido de %u\n", from); return;
  }
  const char* devId   = doc["id"]   | "";
  const char* type    = doc["type"] | "";
  const char* payload = doc["p"].isNull() ? (doc["payload"] | "{}") : (doc["p"] | "{}");
  if (!devId[0] || !type[0]) return;

  _devMap[from] = String(devId);
  Serial.printf("[Mesh RX] %s|%s from=%u\n", devId, type, from);
  if (strcmp(type, "REGISTER") == 0) return;
  if (onPacket) onPacket(devId, type, payload);
}

void ECOSMesh::_onNew(uint32_t id) {
  _count = _mesh.getNodeList(false).size();
  Serial.printf("[Mesh] conectou: %u  total=%u\n", id, _count);
  if (onNodeJoin) onNodeJoin(id);
}

void ECOSMesh::_onDrop(uint32_t id) {
  _count = _mesh.getNodeList(false).size();
  _devMap.erase(id);
  Serial.printf("[Mesh] desconectou: %u  total=%u\n", id, _count);
  if (onNodeLeave) onNodeLeave(id);
}

void ECOSMesh::_initCallbacks() {
  _mesh.onReceive([this](uint32_t f, String& r){ _onReceive(f, r); });
  _mesh.onNewConnection([this](uint32_t id){ _onNew(id); });
  _mesh.onDroppedConnection([this](uint32_t id){ _onDrop(id); });
}

void ECOSMesh::beginRoot(uint8_t channel) {
  if (_running) return;
  _mesh.setDebugMsgTypes(ERROR | STARTUP);
  _mesh.init(_cfg.mesh_ssid, _cfg.mesh_password, &_sched, ECOS_MESH_PORT, WIFI_AP_STA, channel);
  _mesh.setRoot(true);
  _mesh.setContainsRoot(true);
  _initCallbacks();
  _running = true;
  Serial.printf("[Mesh] root: %s canal=%d\n", _cfg.mesh_ssid, channel);
}

void ECOSMesh::beginNode(uint8_t channel) {
  if (_running) return;
  _mesh.setDebugMsgTypes(ERROR | STARTUP);
  _mesh.init(_cfg.mesh_ssid, _cfg.mesh_password, &_sched, ECOS_MESH_PORT, WIFI_AP_STA, channel);
  _mesh.setContainsRoot(true);
  _initCallbacks();
  _running = true;
  Serial.printf("[Mesh] nó: %s canal=%d\n", _cfg.mesh_ssid, channel);
}

void ECOSMesh::stop() {
  if (!_running) return;
  _mesh.stop();
  _running = false; _count = 0; _devMap.clear();
  Serial.println("[Mesh] parado");
}

void ECOSMesh::update() {
  if (!_running) return;
  _sched.execute();
  _mesh.update();
}

bool ECOSMesh::sendToId(const String& devId, const char* type, const char* payload) {
  if (!_running) return false;
  for (auto& kv : _devMap) {
    if (kv.second == devId) {
      String msg = _buildMsg(devId.c_str(), type, payload);
      _mesh.sendSingle(kv.first, msg);
      Serial.printf("[Mesh TX] %s|%s → node %u\n", devId.c_str(), type, kv.first);
      return true;
    }
  }
  return false;
}

bool ECOSMesh::broadcast(const char* type, const char* payload) {
  if (!_running) return false;
  String msg = _buildMsg(_cfg.device_id, type, payload);
  _mesh.sendBroadcast(msg);
  return true;
}
