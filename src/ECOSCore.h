#pragma once
/*
 * ECOSCore — biblioteca central para dispositivos ECOS (ESP32)
 *
 * Antes de incluir, defina os módulos desejados:
 *   #define ECOS_USE_WIFI    — gerenciador WiFi + portal AP
 *   #define ECOS_USE_FLASH   — fila persistente LittleFS
 *   #define ECOS_USE_MQTT    — cliente MQTT sobre WebSocket
 *   #define ECOS_USE_LORA    — radio LoRa (SX1276/SX1278)
 *   #define ECOS_USE_MESH    — rede mesh WiFi (painlessMesh)
 *   #define ECOS_USE_OTA     — atualização OTA push + pull HTTP
 *   #define ECOS_USE_OLED      — display OLED SSD1306
 *   #define ECOS_USE_INTEGRITY — monitor heap + crash log + motivo do reset
 */

#include "ECOSConfig.h"
#include "ECOSWatchdog.h"

#ifdef ECOS_USE_FLASH
  #include "ECOSFlash.h"
#endif

#ifdef ECOS_USE_WIFI
  #include "ECOSWiFi.h"
#endif

#ifdef ECOS_USE_MQTT
  #include "ECOSMQTT.h"
#endif

#ifdef ECOS_USE_LORA
  #include "ECOSLoRa.h"
#endif

#ifdef ECOS_USE_MESH
  #include "ECOSMesh.h"
#endif

#ifdef ECOS_USE_OTA
  #include "ECOSOTA.h"
#endif

#ifdef ECOS_USE_OLED
  #include "ECOSOLED.h"
#endif

#ifdef ECOS_USE_INTEGRITY
  #include "ECOSIntegrity.h"
#endif
