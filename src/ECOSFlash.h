#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <functional>

#ifndef ECOS_QUEUE_FILE
#define ECOS_QUEUE_FILE "/queue.jsonl"
#endif
#ifndef ECOS_QUEUE_MAX_KB
#define ECOS_QUEUE_MAX_KB 4096
#endif

class ECOSFlash {
public:
  using SendFn = std::function<bool(const char* topic, const char* payload)>;

  bool begin();
  void push(const char* topic, const char* payload);
  void flush(SendFn sendFn);

  int    count()   const { return _count; }
  bool   ok()      const { return _ok; }
  size_t usedKB()  const { return _ok ? LittleFS.usedBytes()  / 1024 : 0; }
  size_t totalKB() const { return _ok ? LittleFS.totalBytes() / 1024 : 0; }

private:
  bool _ok    = false;
  int  _count = 0;
};
