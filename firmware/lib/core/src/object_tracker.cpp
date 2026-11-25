#include "object_tracker.h"

namespace core {

static uint8_t clampS(float s) {
  if (s < 0) return 0;
  if (s > 255) return 255;
  return (uint8_t)(s + 0.5f);
}

int8_t ObjectTracker::find(uint8_t edge, Side side, float sFromLow, uint8_t curPass) const {
  int8_t best = -1;
  float bd = p_.dupTolCm;
  for (uint8_t i = 0; i < n_; ++i) {
    if (e_[i].edge != edge || e_[i].side != side || e_[i].pass == curPass) continue;
    float d = sFromLow - (float)e_[i].s;
    if (d < 0) d = -d;
    if (d < bd) { bd = d; best = (int8_t)i; }
  }
  return best;
}

bool ObjectTracker::isNew(uint8_t edge, Side side, float sFromLow, uint8_t curPass) const {
  for (uint8_t i = 0; i < n_; ++i) {
    const Entry& e = e_[i];
    if (e.edge != edge || e.side != side || e.cls != ObjClass::Red || e.pass == curPass) continue;
    float d = sFromLow - (float)e.s;
    if (d < 0) d = -d;
    if (d < p_.dupTolCm) return false;
  }
  return true;
}

int8_t ObjectTracker::mark(uint8_t edge, Side side, float sFromLow, ObjClass cls, uint8_t pass) {
  int8_t i = find(edge, side, sFromLow, pass);
  if (i >= 0) {
    Entry& e = e_[i];
    // Red là kết luận cuối; Pending không ghi đè kết quả đã đọc
    if (cls == ObjClass::Pending) return i;
    if (e.cls != ObjClass::Red) e.cls = cls;
    e.s = clampS(sFromLow);
    e.pass = pass;
    return i;
  }
  if (n_ >= kMax) return -1;
  e_[n_] = Entry{edge, side, clampS(sFromLow), cls, pass};
  return (int8_t)n_++;
}

uint8_t ObjectTracker::countOf(ObjClass c) const {
  uint8_t k = 0;
  for (uint8_t i = 0; i < n_; ++i) k += e_[i].cls == c;
  return k;
}

}  // namespace core
