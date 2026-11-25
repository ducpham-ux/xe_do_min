#include "arduino_hal.h"
#include <SoftwareSerial.h>
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

bool ArduinoHal::readColor(core::RawColor& out) {
  // TCS3200 ở sườn TRÁI (TCS34725 đã bỏ sau test lần 1)
  out.r = tcs3200Channel(LOW, LOW);
  out.g = tcs3200Channel(HIGH, HIGH);
  out.b = tcs3200Channel(LOW, HIGH);
  return out.r || out.g || out.b;  // cả 3 kênh hết thời gian chờ = không có xung (dây OUT lỏng?)
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
