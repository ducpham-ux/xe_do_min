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
  bool colorOk[2] = {true, true};
  RawColor color[2];
  int16_t ml = 0, mr = 0;
  bool buz = false, ledOn = false;
  uint32_t buzOnAt = 0, buzOnFor = 0;
  int buzRises = 0;
  uint16_t bk[kBackupRegs] = {};
  bool bkValid = false;
  std::string logs;

  uint32_t millis() override { return t; }
  uint8_t readLine() override { return line; }
  bool obstacle(Side s) override { return obs[(int)s]; }
  bool readColor(Side s, RawColor& out) override { out = color[(int)s]; return colorOk[(int)s]; }
  void setMotors(int16_t l, int16_t r) override { ml = l; mr = r; }
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
}

TEST(robot_red_object_alerts_at_least_2s_and_stops) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color[0] = red3200();
  h.obs[0] = true;
  run(r, h, 10);
  CHECK(r.state() == State::ScanObject);
  CHECK_EQ(h.ml, 0); CHECK_EQ(h.mr, 0);
  run(r, h, 400);
  CHECK(r.state() == State::AlertOn);
  CHECK(h.buz); CHECK(h.ledOn);
  CHECK_EQ(h.ml, 0);
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
  h.color[0] = green3200();
  h.obs[0] = true;
  run(r, h, 600);
  CHECK_EQ(h.buzRises, 0);
  CHECK_EQ(r.stats().alerts, 0);
  CHECK(r.state() == State::Follow);
}

TEST(robot_same_object_not_reported_twice) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color[0] = red3200();
  h.obs[0] = true;
  run(r, h, 2500);           // báo lần 1 (~2,35 s), rồi đi tiếp vài cm, vật vẫn trong tầm E18
  h.obs[0] = false; run(r, h, 20);
  h.obs[0] = true;           // E18 nháy lại khi xe mới đi < 12 cm
  run(r, h, 300);
  h.obs[0] = false;
  run(r, h, 2700);
  CHECK_EQ(h.buzRises, 1);
  CHECK_EQ(r.stats().alerts, 1);
}

TEST(robot_right_sensor_failure_does_not_hang) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.colorOk[1] = false;
  h.obs[1] = true;
  run(r, h, 200);   // xe dừng quét, cảm biến phải lỗi
  h.obs[1] = false;
  run(r, h, 600);
  CHECK(r.state() == State::Follow);
  CHECK_EQ(r.stats().alerts, 0);
  CHECK(r.stats().sensorErrors > 0);
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
    h.color[0] = red3200(); h.obs[0] = true;
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

TEST(robot_calib_mode_learns_red_on_both_sensors) {
  struct CalHal : FakeHal {
    Calib saved; bool didSave = false;
    void saveCalib(const Calib& c) override { saved = c; didSave = true; }
  } h;
  Params p; Robot r(h, p);
  r.begin(BootMode::Calib);
  CHECK(r.state() == State::Calib);
  RawColor red34725; red34725.r = 200; red34725.g = 60; red34725.b = 50; red34725.c = 300;
  h.color[0] = red3200(); h.color[1] = red34725;
  // 4 lớp x 2 bên: đưa vật vào đúng bên, chờ, rút ra
  for (int step = 0; step < 8; ++step) {
    int side = step % 2;
    run(r, h, 300);
    h.obs[side] = true; run(r, h, 800);
    h.obs[side] = false; run(r, h, 300);
  }
  CHECK(h.didSave);
  CHECK(h.saved.has(Sensor::Tcs3200, kRed));
  CHECK(h.saved.has(Sensor::Tcs34725, kRed));
  CHECK(h.saved.has(Sensor::Tcs34725, kPurple));
  CHECK(r.state() == State::Goal);
}

TEST(robot_creeps_past_object_edge_to_read_color) {
  // E18 kích ở mép vật: vài mẫu đầu cảm biến màu chưa nhìn thấy vật (đọc ra nền)
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color[0] = green3200();
  h.obs[0] = true;
  run(r, h, 120);
  CHECK(r.state() == State::ScanObject);
  run(r, h, 100);
  CHECK(h.ml > 0); CHECK(h.mr > 0);       // đang bò chậm tới
  CHECK(h.ml < p.baseSpeed);
  h.color[0] = red3200();                   // cảm biến đã nhìn thẳng vào vật
  run(r, h, 400);
  CHECK(r.state() == State::AlertOn);
  CHECK_EQ(h.ml, 0);                        // dừng hẳn khi kêu còi
}

TEST(robot_creep_scan_gives_up_when_e18_clears) {
  Params p; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color[0] = green3200();
  h.obs[0] = true;
  run(r, h, 200);
  h.obs[0] = false;
  run(r, h, 200);
  CHECK(r.state() == State::Follow);
  CHECK_EQ(h.buzRises, 0);
}

TEST(robot_crawl_by_distance_into_node_center) {
  // crawlCm > 0: bò vào nút theo quãng đường ước lượng thay vì thời gian cố định
  Params p; p.crawlCm = 10; p.startStubCm = 0; FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1100);
  h.line = kL2 | kL1 | kM | kR1 | kR2;      // vạch ngang của nút H
  run(r, h, 5);
  CHECK(r.state() == State::NodeScan);
  run(r, h, 200);                            // 200 ms chưa đủ 10 cm ở tốc độ bò
  CHECK(r.state() == State::NodeScan);
  h.line = kM;
  run(r, h, 600);
  CHECK(r.state() != State::NodeScan);
}

TEST(robot_gripper_grabs_red_on_its_side_and_counts) {
  Params p; p.enableGripper = true; p.gripSide = Side::Left;
  FakeHal h; Robot r(h, p);
  r.begin(BootMode::Run);
  run(r, h, 1500);
  h.color[0] = red3200(); h.obs[0] = true;
  run(r, h, 2500);            // quét + cảnh báo 2,2 s
  h.obs[0] = false;
  CHECK(r.state() == State::Grip);
  run(r, h, 4 * p.gripStepMs + 50);
  CHECK(r.state() == State::Follow);
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
  run(r2, h, 1000 + 45000);            // thêm 45 s -> tổng > 240 s
  CHECK(r2.stats().wentHome);
}
