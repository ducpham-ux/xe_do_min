// object_tracker.h - nhớ các vật ĐỎ đã báo theo vị trí (cung, bên, khoảng cách dọc cung)
// để không báo lặp khi đi lại cung đó (kể cả đi ngược chiều).
// s tính từ nút có chỉ số NHỎ hơn của cung (cm), nên không phụ thuộc chiều đi.
#pragma once
#include <stdint.h>
#include "hal.h"
#include "params.h"

namespace core {

// Đổi "bên của xe" thành "bên của cung" (nhìn theo chiều từ nút chỉ số nhỏ sang lớn),
// để cùng một vật luôn có cùng khóa dù xe đi chiều nào.
inline Side edgeSide(Side robotSide, bool lowToHigh) {
  if (lowToHigh) return robotSide;
  return robotSide == Side::Left ? Side::Right : Side::Left;
}

class ObjectTracker {
 public:
  static constexpr uint8_t kMax = 16;
  struct Entry { uint8_t edge; Side side; uint8_t s; };
  explicit ObjectTracker(const Params& p) : p_(p) {}
  void clear() { n_ = 0; }
  bool isNew(uint8_t edge, Side side, float sFromLow) const;
  void mark(uint8_t edge, Side side, float sFromLow);
  uint8_t count() const { return n_; }
  const Entry& at(uint8_t i) const { return e_[i]; }

 private:
  const Params& p_;
  Entry e_[kMax];
  uint8_t n_ = 0;
};

}  // namespace core
