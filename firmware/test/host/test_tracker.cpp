#include "minitest.h"
#include "object_tracker.h"
#include "backup_codec.h"

using namespace core;

TEST(tracker_same_place_other_pass_is_duplicate) {
  Params p; ObjectTracker t(p);   // dupTolCm = 10
  CHECK(t.isNew(3, Side::Left, 40, 1));
  t.mark(3, Side::Left, 40, ObjClass::Red, 1);
  CHECK(!t.isNew(3, Side::Left, 45, 2));
  CHECK(!t.isNew(3, Side::Left, 32, 2));
  CHECK(t.isNew(3, Side::Left, 52, 2));
  CHECK_EQ(t.count(), 1);
}

TEST(tracker_adjacent_objects_in_same_pass_stay_separate) {
  // cụm vàng - đỏ - xanh đứng sát nhau (cách 7 cm) trên cùng một lượt
  Params p; ObjectTracker t(p);
  t.mark(10, Side::Left, 60, ObjClass::Other, 4);
  CHECK(t.find(10, Side::Left, 67, 4) < 0);      // cùng lượt: không bị gộp
  CHECK(t.isNew(10, Side::Left, 67, 4));
  t.mark(10, Side::Left, 67, ObjClass::Red, 4);
  t.mark(10, Side::Left, 74, ObjClass::Other, 4);
  CHECK_EQ(t.count(), 3);
  CHECK_EQ(t.countOf(ObjClass::Red), 1);
}

TEST(tracker_pending_is_upgraded_when_read) {
  Params p; ObjectTracker t(p);
  t.mark(5, Side::Right, 80, ObjClass::Pending, 2);
  int8_t k = t.find(5, Side::Right, 83, 7);
  CHECK(k >= 0);
  CHECK(t.at((uint8_t)k).cls == ObjClass::Pending);
  t.mark(5, Side::Right, 83, ObjClass::Red, 7);
  CHECK_EQ(t.count(), 1);
  CHECK(t.at(0).cls == ObjClass::Red);
  t.mark(5, Side::Right, 83, ObjClass::Pending, 9);  // Pending không ghi đè kết quả đã đọc
  CHECK(t.at(0).cls == ObjClass::Red);
}

TEST(tracker_other_side_or_edge_is_new) {
  Params p; ObjectTracker t(p);
  t.mark(3, Side::Left, 40, ObjClass::Red, 1);
  CHECK(t.isNew(3, Side::Right, 40, 2));
  CHECK(t.isNew(4, Side::Left, 40, 2));
}

TEST(tracker_capacity_is_bounded) {
  Params p; ObjectTracker t(p);
  for (int i = 0; i < 80; ++i) t.mark((uint8_t)(i % 12), Side::Left, (float)(i * 3), ObjClass::Other, (uint8_t)i);
  CHECK_EQ(t.count(), ObjectTracker::kMax);
}

TEST(backup_roundtrip_keeps_only_reds) {
  Params p; ObjectTracker t(p), u(p);
  t.mark(0, Side::Left, 12.4f, ObjClass::Red, 1);
  t.mark(11, Side::Right, 149.0f, ObjClass::Red, 2);
  t.mark(6, Side::Right, 75.0f, ObjClass::Red, 3);
  t.mark(6, Side::Left, 20.0f, ObjClass::Other, 3);
  t.mark(7, Side::Left, 20.0f, ObjClass::Pending, 3);
  uint16_t regs[kBackupRegs];
  CHECK(encodeBackup(regs, 0x0A5F, t, 2));
  uint16_t cov = 0; uint8_t restarts = 0;
  CHECK(decodeBackup(regs, cov, u, restarts));
  CHECK_EQ(cov, 0x0A5F); CHECK_EQ(restarts, 2); CHECK_EQ(u.count(), 3);
  CHECK(!u.isNew(0, Side::Left, 12, 0));
  CHECK(!u.isNew(11, Side::Right, 149, 0));
  CHECK(!u.isNew(6, Side::Right, 75, 0));
  CHECK(u.isNew(6, Side::Left, 75, 0));
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
  t.mark(kStartStubEdge, Side::Left, 20, ObjClass::Red, 0);
  uint16_t regs[kBackupRegs];
  encodeBackup(regs, 0, t, 0);
  uint16_t cov; uint8_t r;
  CHECK(decodeBackup(regs, cov, u, r));
  CHECK(!u.isNew(kStartStubEdge, Side::Left, 22, 0));
}

TEST(tracker_side_is_relative_to_edge_not_robot) {
  // cùng 1 vật: đi A->D (từ nút nhỏ sang lớn) thấy bên PHẢI xe; đi D->A thấy bên TRÁI xe
  CHECK(edgeSide(Side::Right, true) == Side::Right);
  CHECK(edgeSide(Side::Left, false) == Side::Right);
  CHECK(edgeSide(Side::Right, false) == Side::Left);
  Params p; ObjectTracker t(p);
  t.mark(2, edgeSide(Side::Right, true), 66, ObjClass::Pending, 1);
  CHECK(t.find(2, edgeSide(Side::Left, false), 66, 5) >= 0);
}

TEST(backup_keeps_elapsed_match_seconds) {
  Params p; ObjectTracker t(p), u(p);
  for (int i = 0; i < 9; ++i) t.mark((uint8_t)i, Side::Left, 10.0f * i, ObjClass::Red, (uint8_t)i);  // đầy 9 vật
  uint16_t regs[kBackupRegs];
  encodeBackup(regs, 0x0FFF, t, 3, 287);
  uint16_t cov; uint8_t r; uint16_t el = 0;
  CHECK(decodeBackup(regs, cov, u, r, &el));
  CHECK_EQ(el, 287); CHECK_EQ(u.count(), 9); CHECK_EQ(r, 3);
}
