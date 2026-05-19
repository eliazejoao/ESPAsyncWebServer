#pragma once
#include <Arduino.h>
#include <LoRa.h>
#include <SPI.h>
#include <functional>

// Formato de pacote: "deviceId|type|payload"
// Tipos padrão: DATA, VALVE, HB, ACK, CMD, REGISTER

class ECOSLoRa {
public:
  using PacketFn = std::function<void(const char* devId, const char* type,
                                       const char* payload, int rssi, float snr)>;

  // Inicializa SPI e radio LoRa
  bool begin(int sck, int miso, int mosi, int cs, int rst, int dio0,
             long freq   = 915E6, int sf = 9,  long bw  = 125E3,
             int  cr     = 5,     int sync = 0xEC, int preamble = 8);

  void update();   // chame no loop() — verifica pacotes recebidos

  // Envia pacote no formato "devId|type|payload"
  bool send(const char* devId, const char* type, const char* payload);

  int      lastRssi()  const { return _lastRssi; }
  float    lastSnr()   const { return _lastSnr; }
  uint32_t rxCount()   const { return _rxCount; }
  uint32_t txCount()   const { return _txCount; }
  int      ledPin()          { return _ledPin; }
  void     setLedPin(int p)  { _ledPin = p; }

  PacketFn onPacket;

private:
  int      _ledPin   = -1;
  int      _lastRssi = 0;
  float    _lastSnr  = 0;
  uint32_t _rxCount  = 0;
  uint32_t _txCount  = 0;
};
