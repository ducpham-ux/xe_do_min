// arduino_hal.h - cài đặt core::Hal trên STM32F103 (STM32duino)
#pragma once
#include <Arduino.h>
#include "hal.h"
#include "color.h"

class ArduinoHal : public core::Hal {
 public:
  void begin();
  uint32_t millis() override { return ::millis(); }
  uint8_t readLine() override;
  bool obstacle(core::Side s) override;
  bool readColor(core::Side s, core::RawColor& out) override;
  void setMotors(int16_t left, int16_t right) override;
  void buzzer(bool on) override;
  void led(bool on) override;
  void log(const char* msg) override;
  bool loadBackup(uint16_t* regs, uint8_t n) override;
  void saveBackup(const uint16_t* regs, uint8_t n) override;
  bool loadCalib(core::Calib& c) override;
  void saveCalib(const core::Calib& c) override;
  void servo(uint8_t ch, uint8_t deg) override;

  bool tcs34725Ok() const { return tcsOk_; }
  bool powerOnReset() const { return powerOn_; }

 private:
  bool tcs34725Init();
  bool tcsWrite(uint8_t reg, uint8_t val);
  void i2cRecover();
  uint16_t tcs3200Channel(uint8_t s2, uint8_t s3);
  bool tcsOk_ = false;
  bool powerOn_ = false;
  uint8_t i2cFails_ = 0;
};
