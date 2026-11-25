#include "minitest.h"
#include "line_pos.h"
#include "pid.h"

using namespace core;

TEST(line_center_only_is_zero) {
  Params p; bool seen = false;
  CHECK_NEAR(linePosition(kM, p, 0, &seen), 0.0, 1e-6);
  CHECK(seen);
}

TEST(line_two_sensors_averages_positions) {
  Params p; bool seen = false;
  // L1 (-1.75) + M (0) -> -0.875 : line lệch trái
  CHECK_NEAR(linePosition(kL1 | kM, p, 0, &seen), -0.875, 1e-4);
  CHECK_NEAR(linePosition(kR1 | kR2, p, 0, &seen), (1.75 + 3.5) / 2, 1e-4);
}

TEST(line_lost_keeps_sign_of_last) {
  Params p; bool seen = true;
  CHECK_NEAR(linePosition(0, p, -1.0f, &seen), -p.lostPos, 1e-6);
  CHECK(!seen);
  CHECK_NEAR(linePosition(0, p, 2.0f, &seen), p.lostPos, 1e-6);
}

TEST(line_fillet_run_is_ignored_for_steering) {
  // cung bo hiện thành dải riêng ở mép: L2 . M . R2 -> chỉ lái theo dải giữa
  Params p;
  LineScan s = scanLine(kL2 | kM | kR2, p, 0);
  CHECK(s.seen);
  CHECK_NEAR(s.pos, 0.0, 1e-6);
  CHECK(s.split);
  CHECK(!s.cross());
  CHECK_EQ(s.nRuns, 3);
}

TEST(line_main_run_is_the_one_nearest_last_position) {
  Params p;
  // line ở L1, cung bo ở R2; lần trước line lệch trái -> giữ L1
  LineScan s = scanLine(kL1 | kR2, p, -1.5f);
  CHECK_NEAR(s.pos, -1.75, 1e-6);
  CHECK(s.split);
  // cùng mẫu nhưng lần trước line ở mép phải -> chọn R2
  s = scanLine(kL1 | kR2, p, 3.0f);
  CHECK_NEAR(s.pos, 3.5, 1e-6);
}

TEST(line_cross_patterns) {
  Params p;
  LineScan s = scanLine(kL2 | kL1 | kM | kR1 | kR2, p, 0);
  CHECK(s.crossL); CHECK(s.crossR); CHECK(!s.split);
  s = scanLine(kL2 | kL1 | kM, p, 0);
  CHECK(s.crossL); CHECK(!s.crossR);
  s = scanLine(kL1 | kM | kR1, p, 0);   // phình 3 mắt: chưa phải vạch ngang
  CHECK(!s.cross()); CHECK_EQ(s.mainLen(), 3);
}

TEST(pid_proportional_only) {
  Pid pid(10.0f, 0.0f, 0.0f, 50.0f);
  CHECK_NEAR(pid.update(2.0f, 0.002f), 20.0, 1e-4);
}

TEST(pid_integral_is_clamped) {
  Pid pid(0.0f, 100.0f, 0.0f, 30.0f);
  float u = 0;
  for (int i = 0; i < 1000; ++i) u = pid.update(5.0f, 0.01f);
  CHECK_NEAR(u, 30.0, 1e-3);
}

TEST(pid_derivative_and_reset) {
  Pid pid(0.0f, 0.0f, 1.0f, 50.0f);
  pid.update(0.0f, 0.01f);
  CHECK_NEAR(pid.update(1.0f, 0.01f), 100.0, 1e-3);  // de/dt = 100
  pid.reset();
  CHECK_NEAR(pid.update(1.0f, 0.01f), 0.0, 1e-6);    // sau reset: không có khâu D giả
}

TEST(pid_filtered_derivative_damps_sensor_steps) {
  // mắt số nhảy bậc 0,875 cm trong 4 ms: khâu D thô = 219 cm/s; lọc 25 ms giảm còn ~1/7
  Pid raw(0.0f, 0.0f, 1.0f, 50.0f), filt(0.0f, 0.0f, 1.0f, 50.0f, 0.025f);
  raw.update(0, 0.004f); filt.update(0, 0.004f);
  float a = raw.update(0.875f, 0.004f), b = filt.update(0.875f, 0.004f);
  CHECK(a > 200);
  CHECK(b < a / 5);
  CHECK(b > 0);
}
