// alert.h - bộ hẹn giờ cảnh báo (còi + LED) không chặn
#pragma once
#include <stdint.h>

namespace core {
class Alert {
 public:
  void start(uint32_t now, uint32_t ms) { start_ = now; ms_ = ms; active_ = true; }
  // trả về true khi vẫn đang cảnh báo
  bool update(uint32_t now) { if (active_ && now - start_ >= ms_) active_ = false; return active_; }
  bool active() const { return active_; }
 private:
  uint32_t start_ = 0, ms_ = 0;
  bool active_ = false;
};
}  // namespace core
