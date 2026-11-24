#include "arduino_hal.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include <utility/stm32_eeprom.h>  // API Flash của thư viện EEPROM (ghi đệm 1 trang)
#include "backup.h"
#include "pins.h"
#if ENABLE_GRIPPER
#include <Servo.h>
static Servo gServo[2];
#endif

using core::Side;

#if DEBUG_LOG
static SoftwareSerial dbg(PIN_DBG_RX, PIN_DBG_TX);
#endif

void ArduinoHal::begin() {
  // Lý do reset: bật nguồn (POR) hay nhấn RESET (PIN). Xóa cờ để lần sau đọc đúng.
  powerOn_ = (RCC->CSR & RCC_CSR_PORRSTF) != 0;
  RCC->CSR |= RCC_CSR_RMVF;

  pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT); pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENA, OUTPUT); pinMode(PIN_ENB, OUTPUT);
  analogWriteResolution(8);
  analogWriteFrequency(1000);   // L298N chuyển mạch chậm: 1 kHz là an toàn
  setMotors(0, 0);

  pinMode(PIN_L2, INPUT); pinMode(PIN_L1, INPUT); pinMode(PIN_M, INPUT);
  pinMode(PIN_R1, INPUT); pinMode(PIN_R2, INPUT);
  pinMode(PIN_OBS_L, INPUT_PULLUP); pinMode(PIN_OBS_R, INPUT_PULLUP);

  pinMode(PIN_TCS_S2, OUTPUT); pinMode(PIN_TCS_S3, OUTPUT);
  pinMode(PIN_TCS_OUT, INPUT);

  pinMode(PIN_BUZZER, OUTPUT); buzzer(false);
  pinMode(PIN_LED, OUTPUT); led(false);

#if DEBUG_LOG
  dbg.begin(DEBUG_BAUD);
#endif
  Wire.setSDA(PIN_SDA);
  Wire.setSCL(PIN_SCL);
  Wire.begin();
  Wire.setClock(100000);
  tcsOk_ = tcs34725Init();
  if (!tcsOk_) log("CANH BAO: khong thay TCS34725 (kiem tra PB6/PB7)");

  enableBackupDomain();
#if ENABLE_GRIPPER
  gServo[0].attach(PIN_SERVO_ARM);
  gServo[1].attach(PIN_SERVO_GRIP);
#endif
}

// ---------------------------------------------------------------- line + vật cản
uint8_t ArduinoHal::readLine() {
  // PB11..PB15 nằm liền nhau -> đọc cả cổng 1 lần, dịch 11 bit: bit0 = L2 ... bit4 = R2
  uint8_t v = (uint8_t)((GPIOB->IDR >> 11) & 0x1F);
#if !LINE_BLACK_IS_HIGH
  v = (uint8_t)(~v & 0x1F);
#endif
  return v;
}

bool ArduinoHal::obstacle(Side s) {
  int v = digitalRead(s == Side::Left ? PIN_OBS_L : PIN_OBS_R);
  return OBS_ACTIVE_HIGH ? v == HIGH : v == LOW;
}

// ---------------------------------------------------------------- màu
uint16_t ArduinoHal::tcs3200Channel(uint8_t s2, uint8_t s3) {
  digitalWrite(PIN_TCS_S2, s2);
  digitalWrite(PIN_TCS_S3, s3);
  delayMicroseconds(200);  // (đã đo thật) chờ bộ lọc ổn định
  unsigned long us = pulseIn(PIN_TCS_OUT, LOW, TCS3200_TIMEOUT_US);
  return us > 65535 ? 65535 : (uint16_t)us;
}

bool ArduinoHal::readColor(Side s, core::RawColor& out) {
  if (s == Side::Left) {
    out.r = tcs3200Channel(LOW, LOW);
    out.g = tcs3200Channel(HIGH, HIGH);
    out.b = tcs3200Channel(LOW, HIGH);
    out.c = 0;
    return true;  // 0 = hết thời gian chờ -> lõi tự coi là "không có vật"
  }
  if (!tcsOk_) {
    if (++i2cFails_ % 20 == 0) { i2cRecover(); tcsOk_ = tcs34725Init(); }
    return false;
  }
  Wire.beginTransmission(TCS34725_ADDR);
  Wire.write(0x80 | 0x20 | 0x14);  // lệnh + tự tăng địa chỉ, bắt đầu từ CDATAL
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(TCS34725_ADDR, 8) != 8) {
    tcsOk_ = false;
    return false;
  }
  uint16_t v[4];
  for (int i = 0; i < 4; ++i) { uint8_t lo = Wire.read(); uint8_t hi = Wire.read(); v[i] = (uint16_t)(lo | (hi << 8)); }
  out.c = v[0]; out.r = v[1]; out.g = v[2]; out.b = v[3];
  i2cFails_ = 0;
  return true;
}

bool ArduinoHal::tcsWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(TCS34725_ADDR);
  Wire.write(0x80 | reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool ArduinoHal::tcs34725Init() {
  Wire.beginTransmission(TCS34725_ADDR);
  Wire.write(0x80 | 0x12);  // thanh ghi ID
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(TCS34725_ADDR, 1) != 1) return false;
  uint8_t id = Wire.read();
  if (id != 0x44 && id != 0x4D) return false;
  bool ok = tcsWrite(0x01, 0xF6);   // (đã đo thật) ATIME: tích phân ~24 ms
  ok &= tcsWrite(0x0F, 0x03);       // (đã đo thật) gain 60x
  ok &= tcsWrite(0x00, 0x01);       // PON
  delay(3);
  ok &= tcsWrite(0x00, 0x03);       // PON + AEN
  return ok;
}

void ArduinoHal::i2cRecover() {
  // Giải phóng bus I2C bị treo: phát 9 xung SCL rồi điều kiện STOP
  Wire.end();
  pinMode(PIN_SDA, INPUT_PULLUP);
  pinMode(PIN_SCL, OUTPUT_OPEN_DRAIN);
  for (int i = 0; i < 9; ++i) {
    digitalWrite(PIN_SCL, LOW); delayMicroseconds(5);
    digitalWrite(PIN_SCL, HIGH); delayMicroseconds(5);
  }
  Wire.setSDA(PIN_SDA);
  Wire.setSCL(PIN_SCL);
  Wire.begin();
  Wire.setClock(100000);
  log("Khoi phuc bus I2C");
}

// ---------------------------------------------------------------- động cơ, còi, LED
static void motor(uint8_t pinEn, uint8_t pinA, uint8_t pinB, int16_t v, bool invert) {
  if (invert) v = (int16_t)-v;
  if (v > 255) v = 255;
  if (v < -255) v = -255;
  if (v >= 0) { digitalWrite(pinA, HIGH); digitalWrite(pinB, LOW); analogWrite(pinEn, v); }
  else { digitalWrite(pinA, LOW); digitalWrite(pinB, HIGH); analogWrite(pinEn, -v); }
}

void ArduinoHal::setMotors(int16_t left, int16_t right) {
  motor(PIN_ENA, PIN_IN1, PIN_IN2, left, MOTOR_LEFT_INVERT);
  motor(PIN_ENB, PIN_IN3, PIN_IN4, right, MOTOR_RIGHT_INVERT);
}

void ArduinoHal::buzzer(bool on) { digitalWrite(PIN_BUZZER, on ? HIGH : LOW); }
void ArduinoHal::led(bool on) { digitalWrite(PIN_LED, on ? LOW : HIGH); }

void ArduinoHal::log(const char* msg) {
#if DEBUG_LOG
  dbg.print(::millis());
  dbg.print(' ');
  dbg.println(msg);
#else
  (void)msg;
#endif
}

// ---------------------------------------------------------------- bộ nhớ
bool ArduinoHal::loadBackup(uint16_t* regs, uint8_t n) {
  if (powerOn_) return false;  // vừa bật nguồn: thanh ghi Backup không còn ý nghĩa
  for (uint8_t i = 0; i < n; ++i) regs[i] = (uint16_t)getBackupRegister(LL_RTC_BKP_DR1 + i);
  return true;
}

void ArduinoHal::saveBackup(const uint16_t* regs, uint8_t n) {
  for (uint8_t i = 0; i < n; ++i) setBackupRegister(LL_RTC_BKP_DR1 + i, regs[i]);
}

bool ArduinoHal::loadCalib(core::Calib& c) {
  eeprom_buffer_fill();
  uint8_t* p = reinterpret_cast<uint8_t*>(&c);
  for (size_t i = 0; i < sizeof(c); ++i) p[i] = eeprom_buffered_read_byte(i);
  return c.magic == core::Calib::kMagic;
}

void ArduinoHal::saveCalib(const core::Calib& c) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(&c);
  eeprom_buffer_fill();
  for (size_t i = 0; i < sizeof(c); ++i) eeprom_buffered_write_byte(i, p[i]);
  eeprom_buffer_flush();  // ghi 1 trang Flash duy nhất
}

void ArduinoHal::servo(uint8_t ch, uint8_t deg) {
#if ENABLE_GRIPPER
  if (ch < 2) gServo[ch].write(deg);
#else
  (void)ch; (void)deg;
#endif
}
