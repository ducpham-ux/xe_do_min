// line_pos.h - phân tích 5 mắt thẳng hàng: tách các DẢI đen liền, chọn dải chính (gần vị trí
// line lần trước nhất), tính vị trí line (cm, dương = line lệch phải) và nhận dạng mẫu nút.
//
// Vì sao theo "dải": line rộng 2,8 cm, mắt cách 1,75 cm -> line thật phủ 1-2 mắt liền nhau.
// Cung bo 45° ở ngã tư hiện ra như một dải RIÊNG ở mép thanh (cách line bởi mắt trắng).
// Chỉ lái theo dải chính nên xe không bị cung bo kéo lệch.
#pragma once
#include <stdint.h>
#include "hal.h"
#include "params.h"

namespace core {

struct LineScan {
  uint8_t nRuns = 0;
  int8_t mainLo = -1, mainHi = -1;  // chỉ số mắt (0 = L2 ... 4 = R2) của dải chính, -1 nếu không có
  float pos = 0;                    // vị trí dải chính (cm)
  bool seen = false;
  uint8_t mainLen() const { return seen ? (uint8_t)(mainHi - mainLo + 1) : 0; }
  // Dải chính chạy liền từ giữa ra tới mắt ngoài (>= 3 mắt): dấu hiệu vạch ngang.
  bool crossL = false, crossR = false;
  // Mắt ngoài đen nhưng KHÔNG thuộc dải chính: dấu hiệu cung bo (hoặc nhánh xiên).
  bool split = false;
  bool cross() const { return crossL || crossR; }
};

LineScan scanLine(uint8_t bits, const Params& p, float last);

// Tiện ích: vị trí để lái (khi mất line trả về +-lostPos theo dấu lần cuối).
float linePosition(uint8_t bits, const Params& p, float last, bool* seen);

}  // namespace core
