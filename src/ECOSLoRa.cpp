#include "ECOSLoRa.h"

bool ECOSLoRa::begin(int sck, int miso, int mosi, int cs, int rst, int dio0,
                     long freq, int sf, long bw, int cr, int sync, int preamble) {
  SPI.begin(sck, miso, mosi, cs);
  LoRa.setPins(cs, rst, dio0);
  if (!LoRa.begin(freq)) {
    Serial.println("[LoRa] ERRO init");
    return false;
  }
  LoRa.setSpreadingFactor(sf);
  LoRa.setSignalBandwidth(bw);
  LoRa.setCodingRate4(cr);
  LoRa.setPreambleLength(preamble);
  LoRa.setSyncWord(sync);
  LoRa.enableCrc();
  Serial.printf("[LoRa] OK %.0fMHz SF%d BW%.0fkHz sync=0x%02X\n",
    freq/1e6, sf, bw/1e3, sync);
  return true;
}

void ECOSLoRa::update() {
  int sz = LoRa.parsePacket();
  if (sz <= 0) return;
  _lastRssi = LoRa.packetRssi();
  _lastSnr  = LoRa.packetSnr();
  String raw = "";
  while (LoRa.available()) raw += (char)LoRa.read();
  raw.trim();
  if (raw.length() == 0) return;
  _rxCount++;

  // Parse "devId|type|payload"
  int s1 = raw.indexOf('|'); if (s1 < 0) return;
  int s2 = raw.indexOf('|', s1+1); if (s2 < 0) return;
  String devId   = raw.substring(0, s1);   devId.trim();
  String type    = raw.substring(s1+1, s2); type.trim();
  String payload = raw.substring(s2+1);

  Serial.printf("[LoRa RX] %s|%s rssi=%d snr=%.1f\n",
    devId.c_str(), type.c_str(), _lastRssi, _lastSnr);

  if (type == "ACK") return;
  if (onPacket) onPacket(devId.c_str(), type.c_str(), payload.c_str(), _lastRssi, _lastSnr);
}

bool ECOSLoRa::send(const char* devId, const char* type, const char* payload) {
  char pkt[256];
  snprintf(pkt, sizeof(pkt), "%s|%s|%s", devId, type, payload);
  LoRa.beginPacket(); LoRa.print(pkt); LoRa.endPacket(true);
  _txCount++;
  Serial.printf("[LoRa TX] %s\n", pkt);
  if (_ledPin >= 0) { digitalWrite(_ledPin, HIGH); delay(50); digitalWrite(_ledPin, LOW); }
  return true;
}
