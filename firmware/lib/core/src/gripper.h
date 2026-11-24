// gripper.h - khung điều khiển cơ cấu gắp 2 servo cho Vòng 2 (chưa có cơ khí thật:
// góc servo và thời gian mỗi bước là tham số trong params.h).
// Chuỗi: MỞ kẹp -> HẠ tay -> KẸP -> NÂNG tay. Kênh 0 = servo nâng (PA6), kênh 1 = servo kẹp (PA7).
#pragma once
#include <stdint.h>
#include "hal.h"
#include "params.h"

namespace core {
class Gripper {
 public:
  explicit Gripper(const Params& p) : p_(p) {}
  void start(uint32_t now, Hal& hal);
  bool update(uint32_t now, Hal& hal);  // true khi còn đang gắp
  void park(Hal& hal);                   // tư thế an toàn khi chạy: nâng tay, đóng kẹp
 private:
  void apply(uint8_t step, Hal& hal);
  const Params& p_;
  uint8_t step_ = 4;
  uint32_t at_ = 0;
};
}  // namespace core
