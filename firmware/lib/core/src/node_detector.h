// node_detector.h - phát hiện và phân loại nút giao với thanh 5 mắt THẲNG HÀNG.
//
// Vạch ngang thật (vuông góc): cả dải đen liền từ M ra mắt ngoài, kéo dài ~2,8 cm theo hướng đi,
// sau đó mắt ngoài trắng cùng lúc.
// Cung bo 45° ở ngã tư (B, E, H): khi đi tới, cung tách dần khỏi line ->
//   1) line "phình" 3 mắt (L1 M R1) vài cm,
//   2) phình hết thanh rất ngắn (~1-1,5 cm) - dễ nhầm là vạch ngang,
//   3) rồi TÁCH thành dải riêng ở mắt ngoài (mẫu "split": L2 . M . R2).
// Quy tắc: sự kiện vạch ngang chỉ được nhận khi đủ dài (minCrossCm, hoặc suspectCrossCm nếu
// ngay trước đó có phình 3 mắt) và kết thúc KHÔNG phải bằng mẫu tách khi còn ngắn.
// Kết quả chỉ báo nhánh trái/phải; lối thẳng do Robot xét sau khi bò thêm straightCheckCm.
#pragma once
#include <stdint.h>
#include "hal.h"
#include "line_pos.h"
#include "params.h"

namespace core {

struct NodeShape {
  bool left = false, right = false, straight = false;
  int branches() const { return (left ? 1 : 0) + (right ? 1 : 0) + (straight ? 1 : 0); }
  bool isNode() const { return left || right; }
  bool operator==(const NodeShape& o) const { return left == o.left && right == o.right && straight == o.straight; }
};

// Xác nhận một điều kiện khi nó đúng N mẫu liên tiếp (lọc nhiễu 1 mẫu của cảm biến số).
class Confirm {
 public:
  explicit Confirm(uint8_t need) : need_(need) {}
  bool feed(bool hit) { run_ = hit ? (uint8_t)(run_ < 255 ? run_ + 1 : 255) : 0; return run_ >= need_; }
  void reset() { run_ = 0; }
 private:
  uint8_t need_, run_ = 0;
};

class NodeDetector {
 public:
  explicit NodeDetector(const Params& p) : p_(p) {}
  void reset();
  // Gọi mỗi mẫu khi đang bám line trong cổng xét nút; ds = quãng vừa đi (cm).
  // Trả về true đúng 1 lần khi vừa đi qua HẾT một vạch ngang thật; shape() cho trái/phải.
  bool feed(const LineScan& s, float ds);
  NodeShape shape() const { return shape_; }
  bool flareSeen() const { return flare_; }
  // Nút có cung bo (B, E, H): chỉ nhận vạch ngang SAU khi đã thấy mẫu tách (cung bo rời line).
  void setRequireSplit(bool on) { requireSplit_ = on; }
  bool inCross() const { return inCross_; }
  uint8_t rejected() const { return rejected_; }  // số sự kiện bị loại (cung bo / nhiễu)

 private:
  void finishEvent(bool endedBySplit);
  const Params& p_;
  bool inCross_ = false, suspect_ = false, flare_ = false, confirmed_ = false, requireSplit_ = false;
  float crossDist_ = 0, wideDist_ = 0, narrowDist_ = 0;
  uint16_t nL_ = 0, nR_ = 0;
  uint8_t endRun_ = 0, rejected_ = 0;
  bool endSplit_ = false, flareAtStart_ = false;
  NodeShape shape_;
};

}  // namespace core
