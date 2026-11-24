#include "line_pos.h"

namespace core {

float linePosition(uint8_t bits, const Params& p, bool innerOnly, float last, bool* seen) {
  float sum = 0;
  int n = 0;
  for (int i = 0; i < 5; ++i) {
    if (innerOnly && (i == 0 || i == 4)) continue;
    if (bits & (1u << i)) { sum += p.sensorY[i]; ++n; }
  }
  if (n > 0) {
    if (seen) *seen = true;
    return sum / n;
  }
  if (seen) *seen = false;
  // Mất line: nếu đang chỉ dùng mắt trong mà mắt ngoài vẫn thấy line -> theo hướng mắt ngoài
  if (innerOnly) {
    if (bits & kL2) return -p.lostPos;
    if (bits & kR2) return p.lostPos;
  }
  return last < 0 ? -p.lostPos : p.lostPos;
}

}  // namespace core
