// node_detector.h - phát hiện và phân loại nút giao.
// Thanh mắt xếp bán nguyệt nên khi qua vạch ngang các mắt chạm đen LẦN LƯỢT
// (M -> L1/R1 -> L2/R2). Vì vậy không đòi hỏi mọi mắt đen cùng lúc mà:
//  1) ứng viên: mắt ngoài + ít nhất 1 mắt giữa đen trong N mẫu liên tiếp;
//  2) bò thẳng SCAN_TIME, tích lũy (OR) mắt ngoài trái/phải;
//  3) kết thúc: còn mắt giữa thấy line -> còn lối thẳng.
#pragma once
#include <stdint.h>
#include "hal.h"
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
  bool feedCandidate(uint8_t bits);  // gọi mỗi mẫu khi đang bám line
  void beginScan();
  void accumulate(uint8_t bits);     // gọi mỗi mẫu trong lúc bò vào tâm nút
  NodeShape finish(uint8_t bitsAfter) const;

 private:
  const Params& p_;
  uint8_t run_ = 0;
  bool sawL_ = false, sawR_ = false;
  uint8_t runL_ = 0, runR_ = 0;  // số mẫu liên tiếp (lọc nhiễu 1 mẫu)
};

}  // namespace core
