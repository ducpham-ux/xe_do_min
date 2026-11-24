#include "minitest.h"
#include "object_tracker.h"
#include "backup_codec.h"

using namespace core;

TEST(tracker_same_place_is_duplicate) {
  Params p; ObjectTracker t(p);
  CHECK(t.isNew(3, Side::Left, 40));
  t.mark(3, Side::Left, 40);
  CHECK(!t.isNew(3, Side::Left, 45));
  CHECK(!t.isNew(3, Side::Left, 31));
  CHECK_EQ(t.count(), 1);
}

TEST(tracker_two_reds_20cm_apart_are_distinct) {
  Params p; ObjectTracker t(p);   // dupTolCm = 16
  t.mark(3, Side::Left, 40);
  CHECK(t.isNew(3, Side::Left, 60));
  CHECK(t.isNew(3, Side::Left, 20));
  CHECK(!t.isNew(3, Side::Left, 54));
}

TEST(tracker_other_side_or_edge_is_new) {
  Params p; ObjectTracker t(p);
  t.mark(3, Side::Left, 40);
  CHECK(t.isNew(3, Side::Right, 40));
  CHECK(t.isNew(4, Side::Left, 40));
}

TEST(tracker_capacity_is_bounded) {
  Params p; ObjectTracker t(p);
  for (int i = 0; i < 40; ++i) t.mark((uint8_t)(i % 12), Side::Left, (float)(i * 3));
  CHECK_EQ(t.count(), ObjectTracker::kMax);
}

TEST(backup_roundtrip) {
  Params p; ObjectTracker t(p), u(p);
  t.mark(0, Side::Left, 12.4f);
  t.mark(11, Side::Right, 149.0f);
  t.mark(6, Side::Right, 75.0f);
  uint16_t regs[kBackupRegs];
  CHECK(encodeBackup(regs, 0x0A5F, t, 2));
  uint16_t cov = 0; uint8_t restarts = 0;
  CHECK(decodeBackup(regs, cov, u, restarts));
  CHECK_EQ(cov, 0x0A5F); CHECK_EQ(restarts, 2); CHECK_EQ(u.count(), 3);
  CHECK(!u.isNew(0, Side::Left, 12));
  CHECK(!u.isNew(11, Side::Right, 149));
  CHECK(!u.isNew(6, Side::Right, 75));
  CHECK(u.isNew(6, Side::Left, 75));
}

TEST(backup_rejects_garbage) {
  Params p; ObjectTracker u(p);
  uint16_t regs[kBackupRegs] = {0x1234, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint16_t cov; uint8_t r;
  CHECK(!decodeBackup(regs, cov, u, r));
  uint16_t zeros[kBackupRegs] = {};
  CHECK(!decodeBackup(zeros, cov, u, r));
}

TEST(backup_keeps_start_stub_objects) {
  Params p; ObjectTracker t(p), u(p);
  t.mark(kStartStubEdge, Side::Left, 20);
  uint16_t regs[kBackupRegs];
  encodeBackup(regs, 0, t, 0);
  uint16_t cov; uint8_t r;
  CHECK(decodeBackup(regs, cov, u, r));
  CHECK(!u.isNew(kStartStubEdge, Side::Left, 22));
}

TEST(tracker_side_is_relative_to_edge_not_robot) {
  // cùng 1 vật: đi A->D (từ nút nhỏ sang lớn) thấy bên PHẢI xe; đi D->A thấy bên TRÁI xe
  CHECK(edgeSide(Side::Right, true) == Side::Right);
  CHECK(edgeSide(Side::Left, false) == Side::Right);
  CHECK(edgeSide(Side::Right, false) == Side::Left);
  Params p; ObjectTracker t(p);
  t.mark(2, edgeSide(Side::Right, true), 66);
  CHECK(!t.isNew(2, edgeSide(Side::Left, false), 66));
}

TEST(backup_keeps_elapsed_match_seconds) {
  Params p; ObjectTracker t(p), u(p);
  for (int i = 0; i < 9; ++i) t.mark((uint8_t)i, Side::Left, 10.0f * i);  // đầy 9 vật
  uint16_t regs[kBackupRegs];
  encodeBackup(regs, 0x0FFF, t, 3, 287);
  uint16_t cov; uint8_t r; uint16_t el = 0;
  CHECK(decodeBackup(regs, cov, u, r, &el));
  CHECK_EQ(el, 287); CHECK_EQ(u.count(), 9); CHECK_EQ(r, 3);
}
