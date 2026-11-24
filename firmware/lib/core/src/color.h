// color.h - nhận diện màu đỏ từ 2 cảm biến.
// Mặc định dùng ngưỡng đã thử thật trên xe. Sau khi chạy chế độ CALIB (học mẫu thật trên sân),
// dùng phân loại "tâm màu gần nhất" trong không gian chromaticity (r,g chuẩn hóa) để loại
// được cả màu cam/vàng gần đỏ.
#pragma once
#include <stdint.h>
#include "hal.h"
#include "params.h"

namespace core {

enum class Sensor : uint8_t { Tcs3200 = 0, Tcs34725 = 1 };  // trái / phải
enum ColorClass : uint8_t { kRed = 0, kYellow, kGreen, kPurple, kNumClasses };

struct Calib {
  static constexpr uint32_t kMagic = 0xCA1B0002u;
  uint32_t magic = kMagic;
  uint16_t count[2][kNumClasses] = {};      // số mẫu đã học (0 = chưa có)
  float cent[2][kNumClasses][2] = {};       // tâm chromaticity (r, g)
  bool has(Sensor s, uint8_t cls) const { return count[(uint8_t)s][cls] > 0; }
};

void chroma(Sensor s, const RawColor& c, float out[3]);
void calibAdd(Calib& cal, Sensor s, uint8_t cls, const RawColor& c);
bool isRedSample(Sensor s, const RawColor& c, const Params& p, const Calib& cal);

class ColorVote {
 public:
  void reset() { reds_ = total_ = 0; }
  void add(bool red) { ++total_; if (red) ++reds_; }
  uint8_t reds() const { return reds_; }
  uint8_t total() const { return total_; }
  bool decide(uint8_t need) const { return reds_ >= need; }
 private:
  uint8_t reds_ = 0, total_ = 0;
};

}  // namespace core
