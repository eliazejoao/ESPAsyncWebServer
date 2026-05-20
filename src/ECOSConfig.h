#pragma once
#include <Arduino.h>
#include <Preferences.h>

#ifndef ECOS_NVS_NS
#define ECOS_NVS_NS "ecos"
#endif

struct ECOSConfig {
  // WiFi
  char ssid[64]          = "";
  char password[64]      = "";
  char ssid2[64]         = "";
  char password2[64]     = "";
  // MQTT
  char mqtt_host[64]     = "ecossolar.com";
  uint16_t mqtt_port     = 443;
  char mqtt_path[32]     = "/ws";
  bool use_ssl           = true;
  // OTA
  char ota_pass[32]      = "eco@ota";
  // Mesh WiFi
  char mesh_ssid[32]     = "ECOS-MESH";
  char mesh_password[32] = "ecos@mesh";
  // Identidade do dispositivo
  char device_id[32]     = "";
  char device_type[16]   = "node";  // "gateway","inversor","irrigacao","sensor"
  // Segurança — portal local
  char portal_user[32]   = "admin";
  char portal_pass[32]   = "eco@admin";
  // Segurança — MQTT broker
  char mqtt_user[32]     = "";
  char mqtt_pass[32]     = "";
  // Segurança — token gerado pelo servidor ao registrar o dispositivo
  char auth_token[64]    = "";
};

void ecosConfigLoad(ECOSConfig& cfg, const char* ns = ECOS_NVS_NS);
void ecosConfigSave(const ECOSConfig& cfg, const char* ns = ECOS_NVS_NS);

// Preenche device_id com MAC se estiver vazio (ex: "ecos_0C82DCD108F0")
// e envia mensagem REGISTER via MQTT ao conectar
void ecosAutoId(ECOSConfig& cfg);

// Retorna MAC formatado (ex: "F0:08:D1:DC:82:0C")
String ecosMacString();

// Retorna chip ID compacto (ex: "0C82DCD108F0")
String ecosChipId();

// Migra config do formato v15 (chaves NVS individuais no namespace "ecos").
// Só executa se o blob ECOSConfig ainda não existe. Retorna true se migrou.
// Chamar logo após ecosConfigLoad() na primeira inicialização v15→v16.
bool ecosConfigMigrateV15(ECOSConfig& cfg);
