// line_pos.h - ước lượng vị trí vạch đen so với tâm xe (cm, dương = line lệch phải)
#pragma once
#include <stdint.h>
#include "hal.h"
#include "params.h"

namespace core {
// bits: bit cảm biến (xem LineBit). innerOnly: chỉ dùng L1,M,R1 để tính vị trí
// (gần nút giao, mắt ngoài dễ chạm cung loa bo). last: vị trí lần trước.
// seen (tùy chọn): true nếu có mắt (được dùng) thấy line.
float linePosition(uint8_t bits, const Params& p, bool innerOnly, float last, bool* seen);
}  // namespace core
