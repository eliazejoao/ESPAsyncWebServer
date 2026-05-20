#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "esp_system.h"
#include "ECOSWatchdog.h"

#ifndef ECOS_MIN_HEAP_KB
#define ECOS_MIN_HEAP_KB 20      // reinicia se heap livre < 20KB
#endif
#ifndef ECOS_INTEGRITY_CHECK_MS
#define ECOS_INTEGRITY_CHECK_MS 10000  // verifica a cada 10s
#endif

#define ECOS_CRASH_FILE "/last_crash.txt"

class ECOSIntegrity {
public:
  void begin() {
    _resetReason = esp_reset_reason();
    _bootTime    = millis();
    Serial.printf("[Integrity] Motivo do reset: %s\n", resetReasonStr());
    if (wasCrash()) _saveCrashLog();
  }

  // Chame no loop() — verifica heap e watchdog
  void update() {
    uint32_t now = millis();
    if (now - _tCheck < ECOS_INTEGRITY_CHECK_MS) return;
    _tCheck = now;
    uint32_t heap = ESP.getFreeHeap();
    if (heap < (uint32_t)(ECOS_MIN_HEAP_KB * 1024)) {
      Serial.printf("[Integrity] HEAP CRÍTICO: %u bytes — reiniciando!\n", heap);
      delay(200); ESP.restart();
    }
  }

  const char* resetReasonStr() const {
    switch (_resetReason) {
      case ESP_RST_POWERON:   return "power-on";
      case ESP_RST_EXT:       return "reset externo";
      case ESP_RST_SW:        return "software";
      case ESP_RST_PANIC:     return "PANIC/crash";
      case ESP_RST_INT_WDT:   return "watchdog (interrupcao)";
      case ESP_RST_TASK_WDT:  return "watchdog (tarefa)";
      case ESP_RST_WDT:       return "watchdog";
      case ESP_RST_DEEPSLEEP: return "deep sleep";
      case ESP_RST_BROWNOUT:  return "BROWNOUT (tensao baixa)";
      default:                return "desconhecido";
    }
  }

  bool     wasCrash()    const { return _resetReason == ESP_RST_PANIC    ||
                                         _resetReason == ESP_RST_INT_WDT  ||
                                         _resetReason == ESP_RST_TASK_WDT ||
                                         _resetReason == ESP_RST_BROWNOUT; }
  uint32_t freeHeap()    const { return ESP.getFreeHeap(); }
  uint32_t uptimeSec()   const { return (millis() - _bootTime) / 1000; }
  esp_reset_reason_t resetReason() const { return _resetReason; }

  // Lê o log do último crash (salvo na flash)
  String lastCrashLog() {
    if (!LittleFS.exists(ECOS_CRASH_FILE)) return "";
    File f = LittleFS.open(ECOS_CRASH_FILE, "r");
    String s = f.readString(); f.close();
    return s;
  }

private:
  esp_reset_reason_t _resetReason;
  uint32_t           _bootTime = 0;
  uint32_t           _tCheck   = 0;

  void _saveCrashLog() {
    // Monta só se necessário — ECOSFlash::begin() pode já ter montado
    if (!LittleFS.begin(true)) return;
    File f = LittleFS.open(ECOS_CRASH_FILE, "w");
    if (!f) return;
    f.printf("reset=%s uptime_prev=%lus heap=%u\n",
      resetReasonStr(), uptimeSec(), ESP.getFreeHeap());
    f.close();
    Serial.println("[Integrity] crash log salvo");
  }
};
