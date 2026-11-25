#include "line_pos.h"

namespace core {

LineScan scanLine(uint8_t bits, const Params& p, float last) {
  LineScan s;
  int8_t lo[3], hi[3];
  uint8_t n = 0;
  for (int8_t i = 0; i < 5; ++i) {
    if (!(bits & (1u << i))) continue;
    if (n > 0 && hi[n - 1] == i - 1) { hi[n - 1] = i; continue; }
    if (n < 3) { lo[n] = i; hi[n] = i; ++n; }
  }
  s.nRuns = n;
  if (n == 0) return s;
  // dải chính = dải có tâm gần vị trí line lần trước nhất
  int8_t best = 0;
  float bestD = 1e9f, bestPos = 0;
  for (uint8_t k = 0; k < n; ++k) {
    float sum = 0;
    for (int8_t i = lo[k]; i <= hi[k]; ++i) sum += p.sensorY[i];
    float c = sum / (hi[k] - lo[k] + 1);
    float d = c - last;
    if (d < 0) d = -d;
    if (d < bestD) { bestD = d; best = (int8_t)k; bestPos = c; }
  }
  s.seen = true;
  s.mainLo = lo[best];
  s.mainHi = hi[best];
  s.pos = bestPos;
  uint8_t len = s.mainLen();
  if (len >= 3) {
    // Line thật (2,8 cm) chỉ phủ tối đa 2 mắt: dải rộng hơn là line + cung bo / vạch ngang.
    // Lái theo cửa sổ 2 mắt trong dải gần vị trí cũ nhất để không bị cung bo kéo lệch.
    // Ứng viên: từng mắt đơn và từng cặp mắt kề nhau trong dải.
    float bd = 1e9f;
    for (int8_t i = s.mainLo; i <= s.mainHi; ++i) {
      for (int8_t w = 0; w < 2 && i + w <= s.mainHi; ++w) {
        float c = (p.sensorY[i] + p.sensorY[i + w]) * 0.5f;
        float d = c - last;
        if (d < 0) d = -d;
        if (d < bd) { bd = d; s.pos = c; }
      }
    }
  }
  s.crossL = len >= 3 && s.mainLo == 0;
  s.crossR = len >= 3 && s.mainHi == 4;
  for (uint8_t k = 0; k < n; ++k) {
    if ((int8_t)k == best) continue;
    if (lo[k] == 0 || hi[k] == 4) s.split = true;
  }
  return s;
}

float linePosition(uint8_t bits, const Params& p, float last, bool* seen) {
  LineScan s = scanLine(bits, p, last);
  if (seen) *seen = s.seen;
  if (s.seen) return s.pos;
  return last < 0 ? -p.lostPos : p.lostPos;
}

}  // namespace core
