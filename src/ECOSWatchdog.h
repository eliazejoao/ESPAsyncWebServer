#pragma once
#include "esp_task_wdt.h"

#ifndef ECOS_WDT_TIMEOUT_S
#define ECOS_WDT_TIMEOUT_S 60
#endif

inline void ecosWatchdogInit() {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  esp_task_wdt_config_t c = {
    .timeout_ms     = (uint32_t)(ECOS_WDT_TIMEOUT_S * 1000),
    .idle_core_mask = 0,
    .trigger_panic  = true
  };
  esp_task_wdt_reconfigure(&c);
#else
  esp_task_wdt_init(ECOS_WDT_TIMEOUT_S, true);
#endif
  esp_task_wdt_add(NULL);
}

inline void ecosWatchdogReset() { esp_task_wdt_reset(); }
