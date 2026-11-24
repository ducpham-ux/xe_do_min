#include "object_tracker.h"

namespace core {

static uint8_t clampS(float s) {
  if (s < 0) return 0;
  if (s > 255) return 255;
  return (uint8_t)(s + 0.5f);
}

bool ObjectTracker::isNew(uint8_t edge, Side side, float sFromLow) const {
  for (uint8_t i = 0; i < n_; ++i) {
    if (e_[i].edge != edge || e_[i].side != side) continue;
    float d = sFromLow - (float)e_[i].s;
    if (d < 0) d = -d;
    if (d < p_.dupTolCm) return false;
  }
  return true;
}

void ObjectTracker::mark(uint8_t edge, Side side, float sFromLow) {
  if (n_ >= kMax) return;
  e_[n_++] = Entry{edge, side, clampS(sFromLow)};
}

}  // namespace core
