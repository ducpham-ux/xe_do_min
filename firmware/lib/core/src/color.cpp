#include "color.h"
#include <math.h>

namespace core {

void chroma(Sensor s, const RawColor& c, float out[3]) {
  float r, g, b;
  if (s == Sensor::Tcs3200) {
    // chu kỳ -> cường độ
    r = c.r ? 1.0f / c.r : 0.0f;
    g = c.g ? 1.0f / c.g : 0.0f;
    b = c.b ? 1.0f / c.b : 0.0f;
  } else {
    r = c.r; g = c.g; b = c.b;
  }
  float sum = r + g + b;
  if (sum <= 0) { out[0] = out[1] = out[2] = 0; return; }
  out[0] = r / sum; out[1] = g / sum; out[2] = b / sum;
}

void calibAdd(Calib& cal, Sensor s, uint8_t cls, const RawColor& c) {
  if (cls >= kNumClasses) return;
  float ch[3];
  chroma(s, c, ch);
  uint8_t si = (uint8_t)s;
  uint16_t& n = cal.count[si][cls];
  float* m = cal.cent[si][cls];
  ++n;
  m[0] += (ch[0] - m[0]) / n;  // trung bình cộng dồn
  m[1] += (ch[1] - m[1]) / n;
}

static bool present(Sensor s, const RawColor& c, const Params& p) {
  if (s == Sensor::Tcs3200) return c.r > 0 && c.g > 0 && c.b > 0 && c.r < p.t3200MaxR;
  return c.c > p.t34725MinC;
}

bool isRedSample(Sensor s, const RawColor& c, const Params& p, const Calib& cal) {
  if (!present(s, c, p)) return false;
  if (cal.has(s, kRed)) {
    float ch[3];
    chroma(s, c, ch);
    uint8_t si = (uint8_t)s;
    auto dist = [&](uint8_t k) {
      float dx = ch[0] - cal.cent[si][k][0], dy = ch[1] - cal.cent[si][k][1];
      return sqrtf(dx * dx + dy * dy);
    };
    float dr = dist(kRed);
    if (dr > p.calibRadius) return false;
    for (uint8_t k = 1; k < kNumClasses; ++k)
      if (cal.has(s, k) && dist(k) - dr < p.calibMargin) return false;
    return true;
  }
  if (s == Sensor::Tcs3200)
    return c.r < c.g * p.t3200Ratio && c.r < c.b * p.t3200Ratio;
  return c.r > c.g * p.t34725RG && c.r > c.b * p.t34725RB;
}

}  // namespace core
