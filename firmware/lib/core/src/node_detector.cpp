#include "node_detector.h"

namespace core {

void NodeDetector::reset() { run_ = 0; sawL_ = sawR_ = false; }

bool NodeDetector::feedCandidate(uint8_t bits) {
  bool outer = bits & (kL2 | kR2);
  bool center = bits & (kL1 | kM | kR1);
  if (outer && center) {
    if (run_ < 255) ++run_;
  } else {
    run_ = 0;
  }
  return run_ >= p_.nodeConfirmSamples;
}

void NodeDetector::beginScan() { sawL_ = sawR_ = false; run_ = 0; }

void NodeDetector::accumulate(uint8_t bits) {
  if (bits & kL2) sawL_ = true;
  if (bits & kR2) sawR_ = true;
}

NodeShape NodeDetector::finish(uint8_t bitsAfter) const {
  NodeShape s;
  s.left = sawL_;
  s.right = sawR_;
  s.straight = (bitsAfter & (kL1 | kM | kR1)) != 0;
  return s;
}

}  // namespace core
