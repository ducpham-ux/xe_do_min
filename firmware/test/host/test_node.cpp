#include "minitest.h"
#include "node_detector.h"

using namespace core;

namespace {
// Cho bộ dò "đi" qua một chuỗi mẫu, mỗi mẫu kéo dài cm (bước 0,025 cm ~ 1 ms ở 25 cm/s).
struct Feeder {
  Params p;
  NodeDetector d{p};
  int hits = 0;
  NodeShape last;
  void go(uint8_t bits, float cm) {
    for (float x = 0; x < cm; x += 0.025f) {
      if (d.feed(scanLine(bits, p, 0), 0.025f)) { ++hits; last = d.shape(); }
    }
  }
};
const uint8_t kAll = kL2 | kL1 | kM | kR1 | kR2;
}  // namespace

TEST(node_real_cross_is_detected_at_trailing_edge) {
  Feeder f;
  f.go(kM, 10);
  f.go(kAll, 2.8f);       // vạch ngang 2,8 cm
  CHECK_EQ(f.hits, 0);    // chưa báo khi còn đang trên vạch
  f.go(kM, 1);
  CHECK_EQ(f.hits, 1);
  CHECK(f.last.left); CHECK(f.last.right);
}

TEST(node_t_junction_left_only_without_straight) {
  Feeder f;
  f.go(kM, 10);
  f.go(kL2 | kL1 | kM, 2.8f);
  f.go(0, 1);
  CHECK_EQ(f.hits, 1);
  CHECK(f.last.left); CHECK(!f.last.right);
}

TEST(node_flare_merge_blob_is_rejected) {
  // Cung bo tiến tới ngã tư: phình 3 mắt vài cm -> phình hết thanh ~1,2 cm -> tách L2 . M . R2
  Feeder f;
  f.go(kM, 10);
  f.go(kL1 | kM | kR1, 4);
  f.go(kAll, 1.2f);
  f.go(kL2 | kM | kR2, 3);
  f.go(kM, 2);
  CHECK_EQ(f.hits, 0);
  CHECK(f.d.flareSeen());
  CHECK(f.d.rejected() >= 1);
  // rồi mới tới vạch ngang thật
  f.go(kAll, 2.8f);
  f.go(kM, 1);
  CHECK_EQ(f.hits, 1);
}

TEST(node_suspect_needs_longer_cross) {
  // phình 3 mắt ngay trước, vạch "ngang" chỉ 1,9 cm -> vẫn nghi là cung bo
  Feeder f;
  f.go(kL1 | kM | kR1, 3);
  f.go(kAll, 1.9f);
  f.go(kM, 1);
  CHECK_EQ(f.hits, 0);
}

TEST(node_short_glitch_is_not_a_node) {
  Feeder f;
  f.go(kM, 5);
  f.go(kAll, 0.5f);
  f.go(kM, 5);
  CHECK_EQ(f.hits, 0);
}

TEST(node_single_sample_dropout_inside_cross_is_tolerated) {
  Feeder f;
  f.go(kM, 5);
  f.go(kAll, 1.0f);
  f.go(kM, 0.025f);       // 1 mẫu nhiễu giữa vạch
  f.go(kAll, 1.8f);
  f.go(kM, 1);
  CHECK_EQ(f.hits, 1);
}

TEST(node_yawed_trailing_split_still_counts) {
  // xe hơi lệch góc: mép sau vạch rời R1 trước R2 -> vài mẫu R2 . M (tách) sau vạch dài
  Feeder f;
  f.go(kM, 5);
  f.go(kAll, 2.8f);
  f.go(kM | kR2, 0.2f);
  f.go(kM, 1);
  CHECK_EQ(f.hits, 1);
}

TEST(node_shape_counts_branches) {
  NodeShape s{true, true, false};
  CHECK_EQ(s.branches(), 2);
  CHECK(s.isNode());
  NodeShape none{false, false, true};
  CHECK(!none.isNode());
}

TEST(confirm_requires_consecutive_hits) {
  Confirm c(2);
  CHECK(!c.feed(true));
  CHECK(!c.feed(false));   // nhiễu 1 mẫu bị bỏ
  CHECK(!c.feed(true));
  CHECK(c.feed(true));     // 2 mẫu liên tiếp
  c.reset();
  CHECK(!c.feed(true));
}
