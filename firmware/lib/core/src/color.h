// color.h - nhận diện màu đỏ bằng TCS3200 (sườn trái). TCS34725 đã bỏ (test lần 1: không
// phát hiện được màu trong môi trường test).
// Mặc định dùng ngưỡng đã thử thật trên xe. Sau khi chạy chế độ CALIB (học mẫu thật trên sân),
// dùng phân loại "tâm màu gần nhất" trong không gian chromaticity (r,g chuẩn hóa) để loại
// được cả màu cam/vàng gần đỏ.
#pragma once
#include <stdint.h>
#include "hal.h"
#include "params.h"

namespace core {

enum ColorClass : uint8_t { kRed = 0, kYellow, kGreen, kPurple, kNumClasses };

struct Calib {
  static constexpr uint32_t kMagic = 0xCA1B0003u;  // đổi định dạng -> bản CALIB cũ bị bỏ qua
  uint32_t magic = kMagic;
  uint16_t count[kNumClasses] = {};      // số mẫu đã học (0 = chưa có)
  float cent[kNumClasses][2] = {};       // tâm chromaticity (r, g)
  bool has(uint8_t cls) const { return count[cls] > 0; }
};

void chroma(const RawColor& c, float out[3]);
void calibAdd(Calib& cal, uint8_t cls, const RawColor& c);
// Có vật đủ gần trước cảm biến (đủ sáng để đọc màu)?
bool colorPresent(const RawColor& c, const Params& p);
// Vật đủ gần để đọc màu tin cậy (thấy RÕ)?
bool colorNear(const RawColor& c, const Params& p);
bool isRedSample(const RawColor& c, const Params& p, const Calib& cal);

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
