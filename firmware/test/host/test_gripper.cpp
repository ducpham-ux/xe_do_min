#include <vector>
#include "minitest.h"
#include "gripper.h"

using namespace core;

namespace {
struct ServoHal : Hal {
  uint32_t t = 0;
  std::vector<std::pair<int, int>> writes;  // (kênh, góc)
  uint32_t millis() override { return t; }
  uint8_t readLine() override { return 0; }
  bool obstacle(Side) override { return false; }
  bool readColor(RawColor&) override { return false; }
  void setMotors(int16_t, int16_t) override {}
  void buzzer(bool) override {}
  void led(bool) override {}
  void log(const char*) override {}
  void servo(uint8_t ch, uint8_t deg) override { writes.push_back({ch, deg}); }
};
}  // namespace

TEST(gripper_runs_open_lower_close_lift) {
  Params p; ServoHal h; Gripper g(p);
  g.start(0, h);
  bool busy = true; uint32_t doneAt = 0;
  for (h.t = 0; h.t < 5000 && busy; ++h.t) { busy = g.update(h.t, h); if (!busy) doneAt = h.t; }
  CHECK(!busy);
  CHECK(doneAt >= 4u * p.gripStepMs);
  CHECK(doneAt < 4u * p.gripStepMs + 10);
  CHECK_EQ(h.writes.size(), 4u);
  CHECK_EQ(h.writes[0].first, 1); CHECK_EQ(h.writes[0].second, p.gripOpenDeg);
  CHECK_EQ(h.writes[1].first, 0); CHECK_EQ(h.writes[1].second, p.armDownDeg);
  CHECK_EQ(h.writes[2].first, 1); CHECK_EQ(h.writes[2].second, p.gripCloseDeg);
  CHECK_EQ(h.writes[3].first, 0); CHECK_EQ(h.writes[3].second, p.armUpDeg);
}

TEST(gripper_park_lifts_and_closes) {
  Params p; ServoHal h; Gripper g(p);
  g.park(h);
  CHECK_EQ(h.writes.size(), 2u);
  CHECK_EQ(h.writes[0].second, p.armUpDeg);
}
