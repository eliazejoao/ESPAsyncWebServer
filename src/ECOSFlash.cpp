#include "ECOSFlash.h"
#include "ECOSWatchdog.h"

bool ECOSFlash::begin() {
  _ok = LittleFS.begin(true);
  if (!_ok) { Serial.println("[Flash] ERRO: LittleFS não montou"); return false; }
  if (LittleFS.exists(ECOS_QUEUE_FILE)) {
    File f = LittleFS.open(ECOS_QUEUE_FILE, "r");
    _count = 0;
    while (f.available()) { if (f.readStringUntil('\n').length() > 2) _count++; }
    f.close();
    if (_count > 0) Serial.printf("[Flash] %d pacotes pendentes\n", _count);
  }
  Serial.printf("[Flash] %uKB total, %uKB livres\n",
    (unsigned)(LittleFS.totalBytes()/1024),
    (unsigned)((LittleFS.totalBytes()-LittleFS.usedBytes())/1024));
  return true;
}

void ECOSFlash::push(const char* topic, const char* payload) {
  if (!_ok) return;
  // Se limite atingido, descarta o pacote mais antigo
  if (LittleFS.usedBytes() > (size_t)ECOS_QUEUE_MAX_KB * 1024) {
    File fin  = LittleFS.open(ECOS_QUEUE_FILE, "r");
    File fout = LittleFS.open("/q_tmp.jsonl",  "w");
    fin.readStringUntil('\n');
    uint8_t buf[64];
    while (fin.available()) { size_t n = fin.readBytes((char*)buf, sizeof(buf)); fout.write(buf, n); }
    fin.close(); fout.close();
    LittleFS.remove(ECOS_QUEUE_FILE);
    LittleFS.rename("/q_tmp.jsonl", ECOS_QUEUE_FILE);
    if (_count > 0) _count--;
  }
  File f = LittleFS.open(ECOS_QUEUE_FILE, "a");
  if (!f) return;
  JsonDocument doc;
  doc["t"] = topic; doc["p"] = payload;
  String line; serializeJson(doc, line);
  f.println(line); f.close();
  _count++;
  Serial.printf("[Flash] gravado (%d): %s\n", _count, topic);
}

void ECOSFlash::flush(SendFn sendFn) {
  if (!_ok || !sendFn || _count == 0) return;
  if (!LittleFS.exists(ECOS_QUEUE_FILE)) { _count = 0; return; }
  Serial.printf("[Flash] enviando %d pacotes...\n", _count);

  File fin  = LittleFS.open(ECOS_QUEUE_FILE, "r");
  File ftmp = LittleFS.open("/q_tmp.jsonl", "w");
  int sent = 0, kept = 0;
  bool failed = false;

  while (fin.available()) {
    String line = fin.readStringUntil('\n'); line.trim();
    if (line.length() < 5) continue;
    if (!failed) {
      JsonDocument doc;
      if (deserializeJson(doc, line) != DeserializationError::Ok) continue;
      const char* t = doc["t"]; const char* p = doc["p"];
      if (!t || !p) continue;
      if (sendFn(t, p)) { sent++; delay(20); ecosWatchdogReset(); continue; }
      failed = true;
    }
    ftmp.println(line); kept++;
  }
  fin.close(); ftmp.close();

  LittleFS.remove(ECOS_QUEUE_FILE);
  if (kept > 0) LittleFS.rename("/q_tmp.jsonl", ECOS_QUEUE_FILE);
  else           LittleFS.remove("/q_tmp.jsonl");
  _count = kept;

  Serial.printf("[Flash] flush: %d enviados, %d restantes\n", sent, kept);
}
