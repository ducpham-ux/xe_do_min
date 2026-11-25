// object_tracker.h - "bản đồ vật thể" tự dựng trong MCU: nhớ mọi vật đã gặp theo vị trí
// (cung, bên của cung, khoảng cách dọc cung) và trạng thái của nó:
//   Red     - đã báo đỏ (không báo lại),
//   Other   - đã đọc, không phải đỏ (không dừng đọc lại -> tiết kiệm thời gian),
//   Pending - mới thấy bằng E18 PHẢI, chưa đọc được (sẽ đi ngược cung để đọc bằng sườn trái).
// s tính từ nút có chỉ số NHỎ hơn của cung (cm), nên không phụ thuộc chiều đi.
//
// Mỗi vật ghi kèm "lượt đi" (pass). Chỉ so trùng với vật của LƯỢT KHÁC: trong cùng một lượt,
// các vật đứng sát nhau (cụm 2-3 vật cách vài cm) luôn được tách bằng sườn LOW->HIGH của E18.
#pragma once
#include <stdint.h>
#include "hal.h"
#include "params.h"

namespace core {

enum class ObjClass : uint8_t { Red = 0, Other = 1, Pending = 2 };
constexpr uint8_t kRestoredPass = 255;  // vật khôi phục sau restart

// Đổi "bên của xe" thành "bên của cung" (nhìn theo chiều từ nút chỉ số nhỏ sang lớn),
// để cùng một vật luôn có cùng khóa dù xe đi chiều nào.
inline Side edgeSide(Side robotSide, bool lowToHigh) {
  if (lowToHigh) return robotSide;
  return robotSide == Side::Left ? Side::Right : Side::Left;
}

class ObjectTracker {
 public:
  static constexpr uint8_t kMax = 32;
  struct Entry { uint8_t edge; Side side; uint8_t s; ObjClass cls; uint8_t pass; };
  explicit ObjectTracker(const Params& p) : p_(p) {}
  void clear() { n_ = 0; }
  // vật của lượt KHÁC gần vị trí này nhất (trong dupTolCm), -1 nếu không có
  int8_t find(uint8_t edge, Side side, float sFromLow, uint8_t curPass) const;
  // true nếu chưa có vật ĐỎ (của lượt khác) đã báo tại vị trí này
  bool isNew(uint8_t edge, Side side, float sFromLow, uint8_t curPass = kRestoredPass - 1) const;
  // ghi vật; nếu trùng vật của lượt khác thì cập nhật vật đó. Trả về chỉ số, -1 nếu đầy.
  int8_t mark(uint8_t edge, Side side, float sFromLow, ObjClass cls = ObjClass::Red, uint8_t pass = 0);
  uint8_t count() const { return n_; }
  uint8_t countOf(ObjClass c) const;
  const Entry& at(uint8_t i) const { return e_[i]; }

 private:
  const Params& p_;
  Entry e_[kMax];
  uint8_t n_ = 0;
};

}  // namespace core
