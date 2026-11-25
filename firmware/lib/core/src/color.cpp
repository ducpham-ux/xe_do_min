#include "color.h"
#include <math.h>

namespace core {

void chroma(const RawColor& c, float out[3]) {
  // chu kỳ -> cường độ
  float r = c.r ? 1.0f / c.r : 0.0f;
  float g = c.g ? 1.0f / c.g : 0.0f;
  float b = c.b ? 1.0f / c.b : 0.0f;
  float sum = r + g + b;
  if (sum <= 0) { out[0] = out[1] = out[2] = 0; return; }
  out[0] = r / sum; out[1] = g / sum; out[2] = b / sum;
}

void calibAdd(Calib& cal, uint8_t cls, const RawColor& c) {
  if (cls >= kNumClasses) return;
  float ch[3];
  chroma(c, ch);
  uint16_t& n = cal.count[cls];
  float* m = cal.cent[cls];
  ++n;
  m[0] += (ch[0] - m[0]) / n;  // trung bình cộng dồn
  m[1] += (ch[1] - m[1]) / n;
}

bool colorPresent(const RawColor& c, const Params& p) {
  if (c.r == 0 || c.g == 0 || c.b == 0) return false;
  uint16_t mn = c.r < c.g ? c.r : c.g;
  if (c.b < mn) mn = c.b;
  return mn < p.presenceMaxUs;
}

bool colorNear(const RawColor& c, const Params& p) {
  if (c.r == 0 || c.g == 0 || c.b == 0) return false;
  uint16_t mn = c.r < c.g ? c.r : c.g;
  if (c.b < mn) mn = c.b;
  return mn < p.nearMaxUs;
}

bool isRedSample(const RawColor& c, const Params& p, const Calib& cal) {
  if (!(c.r > 0 && c.g > 0 && c.b > 0 && c.r < p.t3200MaxR)) return false;
  if (cal.has(kRed)) {
    float ch[3];
    chroma(c, ch);
    auto dist = [&](uint8_t k) {
      float dx = ch[0] - cal.cent[k][0], dy = ch[1] - cal.cent[k][1];
      return sqrtf(dx * dx + dy * dy);
    };
    float dr = dist(kRed);
    if (dr > p.calibRadius) return false;
    for (uint8_t k = 1; k < kNumClasses; ++k)
      if (cal.has(k) && dist(k) - dr < p.calibMargin) return false;
    return true;
  }
  return c.r < c.g * p.t3200Ratio && c.r < c.b * p.t3200Ratio;
}

}  // namespace core
