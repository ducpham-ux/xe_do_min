#include "minitest.h"
#include "node_detector.h"

using namespace core;

TEST(node_needs_consecutive_samples) {
  Params p;  // nodeConfirmSamples = 3
  NodeDetector d(p);
  CHECK(!d.feedCandidate(kL2 | kM));
  CHECK(!d.feedCandidate(kL2 | kM));
  CHECK(!d.feedCandidate(kM));          // đứt chuỗi
  CHECK(!d.feedCandidate(kL2 | kM));
  CHECK(!d.feedCandidate(kL2 | kL1));
  CHECK(d.feedCandidate(kR2 | kM | kR1));  // 3 mẫu liên tiếp có mắt ngoài + mắt giữa
}

TEST(node_outer_eye_alone_is_not_candidate) {
  Params p;
  NodeDetector d(p);
  for (int i = 0; i < 10; ++i) CHECK(!d.feedCandidate(kL2));
}

TEST(node_scan_left_with_straight) {
  Params p;
  NodeDetector d(p);
  d.beginScan();
  d.accumulate(kL2 | kL1 | kM);
  d.accumulate(kL1 | kM);
  NodeShape s = d.finish(kM);
  CHECK(s.left); CHECK(!s.right); CHECK(s.straight);
}

TEST(node_scan_right_corner_no_straight) {
  Params p;
  NodeDetector d(p);
  d.beginScan();
  d.accumulate(kR2 | kR1 | kM);
  NodeShape s = d.finish(0);
  CHECK(!s.left); CHECK(s.right); CHECK(!s.straight);
}

TEST(node_shape_counts_branches) {
  NodeShape s{true, true, false};
  CHECK_EQ(s.branches(), 2);
  CHECK(s.isNode());
  NodeShape none{false, false, true};
  CHECK(!none.isNode());
}
