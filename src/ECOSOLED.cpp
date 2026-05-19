#include "ECOSOLED.h"

bool ECOSOLED::begin(int sda, int scl, int rst) {
  Wire.begin(sda, scl);
  // Recria o objeto com os pinos corretos
  _u8g2 = U8G2_SSD1306_128X64_NONAME_F_SW_I2C(U8G2_R0, (uint8_t)scl, (uint8_t)sda, (uint8_t)rst);
  _ready = _u8g2.begin();
  if (!_ready) Serial.println("[OLED] ERRO init");
  return _ready;
}

void ECOSOLED::clear()                          { if (_ready) _u8g2.clearBuffer(); }
void ECOSOLED::draw(uint8_t x, uint8_t y, const char* t) {
  if (!_ready) return;
  _u8g2.setFont(u8g2_font_6x10_tf);
  _u8g2.drawStr(x, y, t);
}
void ECOSOLED::show()                           { if (_ready) _u8g2.sendBuffer(); }

void ECOSOLED::showStartup(const char* title, const char* subtitle) {
  if (!_ready) return;
  _u8g2.clearBuffer();
  _u8g2.setFont(u8g2_font_7x13B_tf);
  _u8g2.drawStr(0, 20, title);
  if (subtitle) {
    _u8g2.setFont(u8g2_font_6x10_tf);
    _u8g2.drawStr(0, 34, subtitle);
  }
  _u8g2.sendBuffer();
}

void ECOSOLED::showError(const char* msg) {
  if (!_ready) return;
  _u8g2.clearBuffer();
  _u8g2.setFont(u8g2_font_6x10_tf);
  _u8g2.drawStr(0, 20, "ERRO:");
  _u8g2.drawStr(0, 34, msg);
  _u8g2.sendBuffer();
}

void ECOSOLED::showProgress(const char* msg, uint8_t pct) {
  if (!_ready) return;
  _u8g2.clearBuffer();
  _u8g2.setFont(u8g2_font_6x10_tf);
  _u8g2.drawStr(0, 20, msg);
  _u8g2.drawFrame(0, 28, 128, 10);
  _u8g2.drawBox(0, 28, (uint8_t)((pct / 100.0f) * 128), 10);
  char pctStr[8]; snprintf(pctStr, sizeof(pctStr), "%d%%", pct);
  _u8g2.drawStr(56, 50, pctStr);
  _u8g2.sendBuffer();
}
