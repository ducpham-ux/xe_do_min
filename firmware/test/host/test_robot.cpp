#include <string>
#include "minitest.h"
#include "robot.h"

using namespace core;

namespace {
// HAL giả: thời gian do test điều khiển, line luôn ở giữa, vật cản/màu đặt tay.
struct FakeHal : Hal {
  uint32_t t = 0;
  uint8_t line = kM;
  bool obs[2] = {false, false};
  bool colorOk = true;
  RawColor color;          // TCS3200 sườn trái
  int colorReads = 0;
  // mô phỏng thô: vật chỉ "hiện" trước TCS sau khi xe dịch ngang đủ số bước
  int presentAfterPivots = 0, pivots = 0;
  int16_t ml = 0, mr = 0, prevL = 0, prevR = 0;
  bool buz = false, ledOn = false;
  uint32_t buzOnAt = 0, buzOnFor = 0;
  int buzRises = 0;
  uint16_t bk[kBackupRegs] = {};
  bool bkValid = false;
  std::string logs;

  uint32_t millis() override { return t; }
  uint8_t readLine() override { return line; }
  bool obstacle(Side s) override { return obs[(int)s]; }
  bool readColor(RawColor& out) override {
    ++colorReads;
    if (pivots / 2 < presentAfterPivots) { out.r = 1500; out.g = 1600; out.b = 1400; return colorOk; }
    out = color;
    return colorOk;
  }
  void setMotors(int16_t l, int16_t r) override {
    // đếm số lần BẮT ĐẦU quay tại chỗ từ trạng thái đứng yên (xung phanh ngược không tính)
    if (prevL == 0 && prevR == 0 && ((l < 0 && r > 0) || (l > 0 && r < 0))) ++pivots;
    prevL = l; prevR = r; ml = l; mr = r;
  }
  void buzzer(bool on) override {
    if (on && !buz) { buzOnAt = t; ++buzRises; }
    if (!on && buz) buzOnFor = t - buzOnAt;
    buz = on;
  }
  void led(bool on) override { ledOn = on; }
  void log(const char* m) override { logs += m; logs += "\n"; }
  bool loadBackup(uint16_t* r, uint8_t n) override { if (!bkValid) return false; for (int i = 0; i < n; ++i) r[i] = bk[i]; return true; }
  void saveBackup(const uint16_t* r, uint8_t n) override { for (int i = 0; i < n; ++i) bk[i] = r[i]; bkValid = true; }
};

RawColor red3200() { RawColor c; c.r = 250; c.g = 600; c.b = 550; return c; }
RawColor green3200() { RawColor c; c.r = 500; c.g = 300; c.b = 450; return c; }

void run(Robot& r, FakeHal& h, uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i) { h.t++; r.tick(); }
}
template <typename Pred>
bool runUntil(Robot& r, FakeHal& h, uint32_t maxMs, Pred done) {
  for (uint32_t i = 0; i < maxMs; ++i) { h.t++; r.tick(); if (done()) return true; }
  return false;
}
// cho xe đi qua một vạch ngang dài cm (theo quãng đường ước lượng của chính xe)
void crossLine(Robot& r, FakeHal& h, uint8_t bits, float cm) {
  float d0 = r.distOnEdge();
  h.line = bits;
  runUntil(r, h, 3000, [&] { return r.distOnEdge() - d0 >= cm || r.state() != State::Follow; });
}
// Nút có cung bo (B, E, H): cung nhập vào line (phình 3 mắt) -> tách (L2 . M . R2) -> line -> vạch ngang
void passFlaredNode(Robot& r, FakeHal& h) {
  crossLine(r, h, kL1 | kM | kR1, 2.0f);
  crossLine(r, h, kL2 | kM | kR2, 3.0f);
  crossLine(r, h, kM, 1.5f);
  crossLine(r, h, kL2 | kL1 | kM | kR1 | kR2, 3.0f);
}
}  // namespace

TEST(robot_starts_following_after_delay) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 500);
  CHECK(r.state() == State::Ready);
  CHECK_EQ(h.ml, 0);
  run(r, h, 600);
  CHECK(r.state() == State::Follow);
  CHECK(h.ml > 0); CHECK(h.mr > 0);
  CHECK(h.ml <= p.baseSpeed);   // tăng tốc mềm, không vượt tốc độ nền
}

TEST(robot_left_red_object_alerts_at_least_2s_then_returns) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color = red3200();
  h.obs[0] = true;
  run(r, h, 10);
  CHECK(r.state() == State::Approach);
  CHECK_EQ(h.ml, h.mr);                 // tiến thẳng ngắn cho TCS ngang tâm vật (không lái)
  CHECK_EQ(h.colorReads, 0);            // chưa đọc khi còn đang căn
  CHECK(runUntil(r, h, 1500, [&] { return r.state() == State::AlertOn; }));
  CHECK(h.buz); CHECK(h.ledOn);
  CHECK_EQ(h.ml, 0);
  CHECK_EQ(r.stats().sideSteps, 0);   // vật đã đủ gần: không cần dịch ngang
  h.obs[0] = false;
  run(r, h, 2500);
  CHECK(!h.buz);
  CHECK(h.buzOnFor >= 2000);
  CHECK_EQ(r.stats().alerts, 1);
  CHECK(r.state() == State::Follow);
}

TEST(robot_non_red_object_is_ignored) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color = green3200();
  h.obs[0] = true;
  run(r, h, 10);
  CHECK(runUntil(r, h, 3000, [&] { return r.state() == State::Follow; }));
  h.obs[0] = false;
  run(r, h, 200);
  CHECK_EQ(h.buzRises, 0);
  CHECK_EQ(r.stats().alerts, 0);
  CHECK_EQ(r.stats().objectsRead, 1);
  CHECK(r.state() == State::Follow);
}

TEST(robot_side_steps_toward_far_object_then_back) {
  // vật cách 5 cm: TCS3200 chỉ thấy sau 2 bước dịch ngang
  Params p; FakeHal h; Robot r(h, p);
  h.presentAfterPivots = 2;
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color = red3200();
  h.obs[0] = true;
  CHECK(runUntil(r, h, 8000, [&] { return r.state() == State::AlertOn; }));
  CHECK_EQ(r.stats().sideSteps, 2);
  h.obs[0] = false;
  CHECK(runUntil(r, h, 10000, [&] { return r.state() == State::Follow; }));
  CHECK_EQ(h.pivots, 8);   // 2 bước tới + 2 bước về, mỗi bước 2 lần quay
  CHECK_EQ(r.stats().alerts, 1);
}

TEST(robot_gives_up_after_max_side_steps) {
  Params p; FakeHal h; Robot r(h, p);
  h.presentAfterPivots = 100;    // không bao giờ thấy vật (E18 báo nhầm)
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.obs[0] = true;
  run(r, h, 10);
  CHECK(runUntil(r, h, 15000, [&] { return r.state() == State::Follow; }));
  CHECK_EQ(r.stats().sideSteps, p.sideStepMax);
  CHECK_EQ(h.buzRises, 0);
}

TEST(robot_right_object_is_remembered_not_stopped_for) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color = red3200();
  h.obs[1] = true;
  run(r, h, 50);
  CHECK(r.state() == State::Follow);   // không dừng
  CHECK(h.ml > 0);
  CHECK_EQ(h.buzRises, 0);
  CHECK_EQ(h.colorReads, 0);
}

TEST(robot_right_object_on_map_edge_requests_reverse_pass) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1100);
  // qua nút H (vạch ngang) -> đi thẳng lên cung H-E
  CHECK(runUntil(r, h, 3000, [&] { return r.distOnEdge() >= 0.5f * p.startStubCm; }));
  passFlaredNode(r, h);
  h.line = kM;
  CHECK(runUntil(r, h, 2000, [&] { return r.nextNode() == kE && r.state() == State::Follow; }));
  run(r, h, 1500);
  h.obs[1] = true; run(r, h, 30); h.obs[1] = false;
  CHECK_EQ(r.stats().pendingSeen, 1);
  CHECK(r.pendingDirMask() & (1ul << dirIndex(kE, kH)));
}

TEST(robot_same_object_not_reread_while_e18_stays_high) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color = red3200();
  h.obs[0] = true;
  run(r, h, 10);
  CHECK(runUntil(r, h, 5000, [&] { return r.state() == State::Follow; }));  // đọc + báo + trở về
  run(r, h, 300);             // vật vẫn trong tầm E18 thêm vài cm
  h.obs[0] = false;
  run(r, h, 500);
  CHECK_EQ(h.buzRises, 1);
  CHECK_EQ(r.stats().objectsRead, 1);
}

TEST(robot_color_sensor_failure_does_not_hang) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.colorOk = false;
  h.obs[0] = true;
  run(r, h, 10);
  CHECK(runUntil(r, h, 15000, [&] { return r.state() == State::Follow; }));
  CHECK_EQ(r.stats().alerts, 0);
  CHECK(r.stats().sensorErrors > 0);
}

TEST(robot_turn_happens_only_after_crawling_to_node_center) {
  // Thẳng qua H, thẳng qua E, tới B lộ trình là RẼ TRÁI.
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1100);
  for (int node = 0; node < 2; ++node) {
    CHECK(runUntil(r, h, 20000, [&] { return r.distOnEdge() >= 0.6f * (node == 0 ? p.startStubCm : p.edgeLenCm); }));
    passFlaredNode(r, h);
    h.line = kM;
    run(r, h, 30);
    CHECK(runUntil(r, h, 2000, [&] { return r.state() == State::Follow; }));
  }
  CHECK_EQ(r.nextNode(), kB);
  CHECK(runUntil(r, h, 20000, [&] { return r.distOnEdge() >= 0.6f * p.edgeLenCm; }));
  passFlaredNode(r, h);
  h.line = kM;
  CHECK(runUntil(r, h, 500, [&] { return r.state() == State::NodeScan; }) || r.state() == State::NodeScan);
  // còn đang bò vào tâm nút, chưa quay
  run(r, h, 150);
  CHECK(r.state() == State::NodeScan);
  CHECK(h.ml > 0); CHECK(h.mr > 0);
  CHECK(runUntil(r, h, 2000, [&] { return r.state() == State::Turn; }));
  // quay trái tại chỗ, và không nhận line trước góc tối thiểu
  run(r, h, 20);
  CHECK(h.ml < 0); CHECK(h.mr > 0);
  run(r, h, (uint32_t)(0.8f * p.turnMinDeg / p.turnRateDegS * 1000));
  CHECK(r.state() == State::Turn);
  CHECK(runUntil(r, h, 3000, [&] { return r.state() == State::Follow; }));
  CHECK_EQ(r.nextNode(), kA);
}

TEST(robot_flare_does_not_trigger_node) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1100);
  CHECK(runUntil(r, h, 3000, [&] { return r.distOnEdge() >= 0.6f * p.startStubCm; }));
  crossLine(r, h, kL1 | kM | kR1, 4.0f);
  crossLine(r, h, kL2 | kL1 | kM | kR1 | kR2, 1.2f);
  crossLine(r, h, kL2 | kM | kR2, 3.0f);
  h.line = kM;
  run(r, h, 100);
  CHECK(r.state() == State::Follow);
  CHECK_EQ(r.stats().nodesSeen, 0);
}

TEST(robot_lost_line_at_edge_seeks_toward_that_side) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.line = kR2;  run(r, h, 30);   // line trôi ra mép phải
  h.line = 0;    run(r, h, 5);
  CHECK(r.state() == State::Seek);
  CHECK(h.ml > 0); CHECK(h.mr < 0);   // quay phải tại chỗ
  h.line = kR1;  run(r, h, 5);
  CHECK(r.state() == State::Follow);
  CHECK_EQ(r.stats().lineLost, 1);
}

TEST(robot_stops_when_match_time_is_over) {
  Params p; p.matchMs = 5000; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 7000);
  CHECK(r.state() == State::Goal);
  CHECK_EQ(h.ml, 0); CHECK_EQ(h.mr, 0);
}

TEST(robot_restores_backup_after_reset) {
  Params p; FakeHal h;
  {
    Robot r(h, p);
    r.begin(BootMode::Run);
    run(r, h, 1500);
    h.color = red3200(); h.obs[0] = true;
    run(r, h, 3000);
    CHECK_EQ(r.stats().alerts, 1);
  }
  CHECK(h.bkValid);
  Robot r2(h, p);
  r2.begin(BootMode::Run);
  CHECK_EQ(r2.stats().restarts, 1);
  CHECK_EQ(r2.stats().alerts, 1);  // nhớ vật đã báo
  Robot r3(h, p);
  r3.begin(BootMode::ClearMemory);
  CHECK_EQ(r3.stats().alerts, 0);
}

TEST(robot_calib_mode_learns_four_colors_on_tcs3200) {
  struct CalHal : FakeHal {
    Calib saved; bool didSave = false;
    void saveCalib(const Calib& c) override { saved = c; didSave = true; }
  } h;
  Params p; Robot r(h, p);
  r.begin(BootMode::Calib);
  CHECK(r.state() == State::Calib);
  h.color = red3200();
  for (int step = 0; step < 4; ++step) {
    run(r, h, 300);
    h.obs[0] = true; run(r, h, 800);
    h.obs[0] = false; run(r, h, 300);
  }
  CHECK(h.didSave);
  CHECK(h.saved.has(kRed));
  CHECK(h.saved.has(kPurple));
  CHECK(r.state() == State::Goal);
}

TEST(robot_gripper_grabs_red_on_left_and_counts) {
  Params p; p.enableGripper = true;
  FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color = red3200(); h.obs[0] = true;
  CHECK(runUntil(r, h, 4000, [&] { return r.state() == State::Grip; }));
  h.obs[0] = false;
  CHECK(runUntil(r, h, 4 * p.gripStepMs + 3000, [&] { return r.state() == State::Follow; }));
  CHECK_EQ(r.stats().grabbed, 1);
}

TEST(robot_match_clock_continues_after_restart) {
  // Trọng tài tính giờ liên tục qua restart: firmware phải nhớ thời gian đã chạy
  Params p; FakeHal h;
  {
    Robot r(h, p);
    r.begin(BootMode::Run);
    run(r, h, 1000 + 200000);          // đã chạy 200 s rồi bị restart
  }
  Robot r2(h, p);
  r2.begin(BootMode::Run);
  run(r2, h, 1000 + 50000);            // thêm 50 s -> tổng > 245 s
  CHECK(r2.stats().wentHome);
}
