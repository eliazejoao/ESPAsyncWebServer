#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

class ECOSOLED {
public:
  bool begin(int sda, int scl, int rst);
  void clear();
  void draw(uint8_t x, uint8_t y, const char* text);
  void show();

  // Telas prontas
  void showStartup(const char* title, const char* subtitle = nullptr);
  void showError(const char* msg);
  void showProgress(const char* msg, uint8_t pct);  // pct 0-100

  // Acesso direto ao U8g2 para desenho customizado
  U8G2_SSD1306_128X64_NONAME_F_SW_I2C& raw() { return _u8g2; }

private:
  U8G2_SSD1306_128X64_NONAME_F_SW_I2C _u8g2{U8G2_R0, 0, 0, U8X8_PIN_NONE};
  bool _ready = false;
};
