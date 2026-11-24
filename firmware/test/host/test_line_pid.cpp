#include "minitest.h"
#include "line_pos.h"
#include "pid.h"

using namespace core;

TEST(line_center_only_is_zero) {
  Params p; bool seen = false;
  CHECK_NEAR(linePosition(kM, p, false, 0, &seen), 0.0, 1e-6);
  CHECK(seen);
}

TEST(line_two_sensors_averages_positions) {
  Params p; bool seen = false;
  // L1 (-2.5) + M (0) -> -1.25 : line lệch trái
  CHECK_NEAR(linePosition(kL1 | kM, p, false, 0, &seen), -1.25, 1e-4);
  CHECK_NEAR(linePosition(kR1 | kR2, p, false, 0, &seen), (2.5 + 4.33) / 2, 1e-4);
}

TEST(line_lost_keeps_sign_of_last) {
  Params p; bool seen = true;
  CHECK_NEAR(linePosition(0, p, false, -1.0f, &seen), -p.lostPos, 1e-6);
  CHECK(!seen);
  CHECK_NEAR(linePosition(0, p, false, 2.0f, &seen), p.lostPos, 1e-6);
}

TEST(line_inner_only_ignores_outer_eyes) {
  Params p; bool seen = false;
  // chỉ L2 (mắt ngoài) + M: innerOnly bỏ L2 -> 0
  CHECK_NEAR(linePosition(kL2 | kM, p, true, 0, &seen), 0.0, 1e-6);
  // chỉ có mắt ngoài và innerOnly -> coi như mất line nhưng có hướng
  CHECK_NEAR(linePosition(kR2, p, true, 0, &seen), p.lostPos, 1e-6);
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
