// hal.h - lớp trừu tượng phần cứng. Lõi thuật toán CHỈ gọi qua giao diện này,
// nên cùng một code chạy được trên xe thật (ArduinoHal) và trên PC (SimHal).
#pragma once
#include <stdint.h>

namespace core {

enum class Side : uint8_t { Left = 0, Right = 1 };

// Bit cảm biến line: bit0=L2, bit1=L1, bit2=M, bit3=R1, bit4=R2 ; 1 = thấy vạch đen
enum LineBit : uint8_t { kL2 = 1, kL1 = 2, kM = 4, kR1 = 8, kR2 = 16 };

// Giá trị màu thô.
//  - TCS3200 (trái): r,g,b là CHU KỲ xung (us) -> càng NHỎ càng nhiều ánh sáng; c không dùng.
//  - TCS34725 (phải): r,g,b,c là số đếm 16 bit -> càng LỚN càng nhiều ánh sáng.
struct RawColor { uint16_t r = 0, g = 0, b = 0, c = 0; };

struct Calib;  // color.h

class Hal {
 public:
  virtual ~Hal() {}
  virtual uint32_t millis() = 0;
  virtual uint8_t readLine() = 0;
  virtual bool obstacle(Side s) = 0;
  // false = lỗi cảm biến (ví dụ I2C không phản hồi)
  virtual bool readColor(Side s, RawColor& out) = 0;
  virtual void setMotors(int16_t left, int16_t right) = 0;  // -255..255, dương = tiến
  virtual void buzzer(bool on) = 0;
  virtual void led(bool on) = 0;
  virtual void log(const char* msg) = 0;
  // Bộ nhớ giữ qua RESET (thanh ghi Backup). Mặc định: không có.
  virtual bool loadBackup(uint16_t* regs, uint8_t n) { (void)regs; (void)n; return false; }
  virtual void saveBackup(const uint16_t* regs, uint8_t n) { (void)regs; (void)n; }
  // Bộ nhớ hiệu chỉnh màu (Flash). Mặc định: không có.
  virtual bool loadCalib(Calib& c) { (void)c; return false; }
  virtual void saveCalib(const Calib& c) { (void)c; }
  // Servo cho cơ cấu gắp (Vòng 2): ch 0 = nâng, 1 = kẹp.
  virtual void servo(uint8_t ch, uint8_t deg) { (void)ch; (void)deg; }
};

}  // namespace core
