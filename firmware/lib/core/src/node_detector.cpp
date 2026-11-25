#include "node_detector.h"

namespace core {

void NodeDetector::reset() {
  inCross_ = suspect_ = flare_ = confirmed_ = endSplit_ = flareAtStart_ = false;
  crossDist_ = wideDist_ = narrowDist_ = 0;
  nL_ = nR_ = 0;
  endRun_ = 0;
  rejected_ = 0;
  shape_ = NodeShape();
}

void NodeDetector::finishEvent(bool endedBySplit) {
  inCross_ = false;
  endRun_ = 0;
  float need = suspect_ ? p_.suspectCrossCm : p_.minCrossCm;
  // Tách ngay khi vạch còn ngắn = cung bo vừa rời line, không phải vạch ngang.
  if (crossDist_ < need || (endedBySplit && suspect_) || (requireSplit_ && !flareAtStart_)) {
    ++rejected_;
    return;
  }
  shape_ = NodeShape();
  shape_.left = nL_ >= 2;
  shape_.right = nR_ >= 2;
  confirmed_ = shape_.isNode();
  if (!confirmed_) ++rejected_;
}

bool NodeDetector::feed(const LineScan& s, float ds) {
  confirmed_ = false;
  bool cross = s.cross();
  // theo dõi "phình 3 mắt" (cung bo đang nhập vào line)
  if (!cross && !s.split && s.mainLen() >= 3) { wideDist_ += ds; narrowDist_ = 0; }
  else if (!cross && !s.split) { narrowDist_ += ds; if (narrowDist_ > 1.0f) wideDist_ = 0; }
  if (s.split) flare_ = true;

  if (cross) {
    if (!inCross_) {
      inCross_ = true;
      suspect_ = wideDist_ >= p_.mergeWideCm;
      flareAtStart_ = flare_;
      crossDist_ = 0;
      nL_ = nR_ = 0;
    }
    endRun_ = 0;
    crossDist_ += ds;
    if (s.crossL) ++nL_;
    if (s.crossR) ++nR_;
    return false;
  }
  if (!inCross_) return false;
  // 2 mẫu liên tiếp không còn vạch ngang mới coi là hết (lọc 1 mẫu nhiễu)
  if (++endRun_ == 1) { endSplit_ = s.split; crossDist_ += ds; return false; }
  finishEvent(endSplit_ || s.split);
  if (s.split || endSplit_) wideDist_ = 0;
  return confirmed_;
}

}  // namespace core
