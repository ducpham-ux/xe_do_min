// main.cpp - chạy firmware (lõi core) trên mô hình sân, nhiều lượt ngẫu nhiên, in thống kê.
// Dùng: run_sim.exe [--seeds N] [--start S] [--svg file] [--orange] [--calib]
//                    [--i2c-fail MS] [--restart-at MS] [--gripper] [--verbose]
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "robot.h"
#include "world.h"

using namespace core;

namespace {
int g_traceFrom = -1, g_traceTo = -1;
const char* g_trail = nullptr;

struct Metrics {
  int redsReadable = 0, readableHit = 0;
  int redsTotal = 0, redsHit = 0, falseAlarms = 0, duplicates = 0, phantom = 0;
  int covered = 0;
  bool finished = false, fallback = false;
  double timeS = 0;
  int mismatches = 0, restarts = 0, grabbed = 0;
};

struct SimHal : Hal {
  sim::World& w;
  uint32_t t = 0;
  bool buz = false, verbose = false, haveCalib = false;
  Calib cal;
  uint16_t bk[kBackupRegs] = {};
  bool bkValid = false;
  Metrics* m = nullptr;
  std::vector<std::pair<double, double>> alertsAt;
  int lastL = 0, lastR = 0;
  int lastColorSide = 0;

  explicit SimHal(sim::World& world) : w(world) {}
  uint32_t millis() override { return t; }
  uint8_t readLine() override { return w.lineBits(); }
  bool obstacle(Side s) override { return w.e18((int)s); }
  bool readColor(Side s, RawColor& o) override { lastColorSide = (int)s; return w.colorReading((int)s, o.r, o.g, o.b, o.c, t); }
  void setMotors(int16_t l, int16_t r) override { lastL = l; lastR = r; }
  void buzzer(bool on) override {
    if (on && !buz) {
      alertsAt.push_back({w.x, w.y});
      // vật được chấm là vật nằm ở bên vừa quét màu
      int best = w.objectBeside(lastColorSide, sim::World::kE18Range);
      if (best < 0) { m->phantom++; }
      else {
        auto& o = w.objects[best];
        o.reports++;
        if (o.color != sim::Color::Red) m->falseAlarms++;
        else if (o.reports > 1) m->duplicates++;
      }
    }
    buz = on;
  }
  void led(bool) override {}
  void log(const char* msg) override { if (verbose) std::printf("  [%6.2fs] (%.0f,%.0f) %s\n", t / 1000.0, w.x, w.y, msg); }
  bool loadBackup(uint16_t* r, uint8_t n) override { if (!bkValid) return false; for (int i = 0; i < n; ++i) r[i] = bk[i]; return true; }
  void saveBackup(const uint16_t* r, uint8_t n) override { for (int i = 0; i < n; ++i) bk[i] = r[i]; bkValid = true; }
  bool loadCalib(Calib& c) override { if (!haveCalib) return false; c = cal; return true; }
};

// Mô phỏng chế độ CALIB: học các mẫu chuẩn (không nhiễu) của từng màu
Calib nominalCalib() {
  Calib c;
  RawColor t3[4], t4[4];
  auto s = [](uint16_t r, uint16_t g, uint16_t b, uint16_t cc) { RawColor x; x.r = r; x.g = g; x.b = b; x.c = cc; return x; };
  t3[kRed] = s(250, 600, 550, 0); t3[kYellow] = s(215, 300, 530, 0); t3[kGreen] = s(500, 300, 450, 0); t3[kPurple] = s(300, 600, 350, 0);
  t4[kRed] = s(200, 60, 50, 310); t4[kYellow] = s(140, 150, 60, 350); t4[kGreen] = s(60, 200, 90, 350); t4[kPurple] = s(120, 70, 110, 300);
  for (uint8_t k = 0; k < kNumClasses; ++k) {
    calibAdd(c, Sensor::Tcs3200, k, t3[k]);
    calibAdd(c, Sensor::Tcs34725, k, t4[k]);
  }
  return c;
}

int coverage(const sim::World& w) {
  int n = 0;
  for (int e = 0; e < 12; ++e) {
    const auto& s = w.segs[e];
    bool bins[15] = {};
    double ux = (s.x2 - s.x1) / 150, uy = (s.y2 - s.y1) / 150;
    for (auto& p : w.trail) {
      double t = (p.first - s.x1) * ux + (p.second - s.y1) * uy;
      double d = std::fabs((p.first - s.x1) * -uy + (p.second - s.y1) * ux);
      if (d < 6 && t >= 0 && t < 150) bins[(int)(t / 10)] = true;
    }
    int c = 0;
    for (bool b : bins) c += b;
    if (c >= 13) ++n;
  }
  return n;
}

void writeTrail(const char* path, const sim::World& w, const SimHal& h) {
  FILE* f = std::fopen(path, "w");
  if (!f) return;
  for (size_t i = 0; i < w.trail.size(); i += 2) std::fprintf(f, "T,%.1f,%.1f\n", w.trail[i].first, w.trail[i].second);
  for (auto& o : w.objects) std::fprintf(f, "O,%.1f,%.1f,%s,%d\n", o.x, o.y, sim::colorName(o.color), o.reports);
  for (auto& a : h.alertsAt) std::fprintf(f, "A,%.1f,%.1f\n", a.first, a.second);
  std::fclose(f);
}

void writeSvg(const char* path, const sim::World& w, const SimHal& h) {
  FILE* f = std::fopen(path, "w");
  if (!f) return;
  auto X = [](double x) { return 60 + x * 1.6; };
  auto Y = [](double y) { return 620 - y * 1.6; };
  std::fprintf(f, "<svg xmlns='http://www.w3.org/2000/svg' width='600' height='720' viewBox='0 0 600 720'><rect width='600' height='720' fill='white'/>\n");
  for (auto& s : w.segs) std::fprintf(f, "<line x1='%.1f' y1='%.1f' x2='%.1f' y2='%.1f' stroke='black' stroke-width='4.8'/>\n", X(s.x1), Y(s.y1), X(s.x2), Y(s.y2));
  for (auto& a : w.arcs) {
    // vẽ cung bằng đa giác
    std::fprintf(f, "<polyline fill='none' stroke='black' stroke-width='4.8' points='");
    for (int i = 0; i <= 20; ++i) {
      double ang = i * 2 * 3.14159265 / 80;
      for (int q = 0; q < 4; ++q) {
        double px = a.cx + a.R * std::cos(ang + q * 1.5707963), py = a.cy + a.R * std::sin(ang + q * 1.5707963);
        if (px >= a.xmin - 0.01 && px <= a.xmax + 0.01 && py >= a.ymin - 0.01 && py <= a.ymax + 0.01) std::fprintf(f, "%.1f,%.1f ", X(px), Y(py));
      }
    }
    std::fprintf(f, "'/>\n");
  }
  std::fprintf(f, "<polyline fill='none' stroke='#2E75B6' stroke-width='1.4' stroke-opacity='0.8' points='");
  for (size_t i = 0; i < w.trail.size(); i += 3) std::fprintf(f, "%.1f,%.1f ", X(w.trail[i].first), Y(w.trail[i].second));
  std::fprintf(f, "'/>\n");
  for (auto& o : w.objects)
    std::fprintf(f, "<circle cx='%.1f' cy='%.1f' r='%.1f' fill='%s' stroke='%s' stroke-width='2'/>\n", X(o.x), Y(o.y), o.r * 1.6, sim::colorName(o.color), o.reports ? "black" : "none");
  for (auto& a : h.alertsAt)
    std::fprintf(f, "<circle cx='%.1f' cy='%.1f' r='7' fill='none' stroke='#C62828' stroke-width='2.5'/>\n", X(a.first), Y(a.second));
  std::fprintf(f, "<text x='20' y='700' font-family='Arial' font-size='14'>Xanh: quy dao xe · Vong do: vi tri bao dong · Vat vien den: da duoc bao</text>\n</svg>\n");
  std::fclose(f);
}

Metrics runOne(const sim::Options& o, bool calib, int restartAt, bool gripper, bool verbose, const char* svg) {
  sim::World w(o);
  SimHal h(w);
  Metrics m;
  h.m = &m; h.verbose = verbose;
  if (calib) { h.cal = nominalCalib(); h.haveCalib = true; }
  Params p;
  p.enableGripper = gripper;
  p.sensorAheadCm = 11.5;
  // p.crawlCm: để 0 -> bò theo thời gian 135 ms như xe thật  // hình học mô phỏng: nút được phát hiện khi trục bánh cách tâm ~10-13 cm
  for (auto& ob : w.objects) if (ob.color == sim::Color::Red) m.redsTotal++;
  auto robot = std::make_unique<Robot>(h, p);
  robot->begin(BootMode::Run);
  const uint32_t limit = 320000;
  bool restarted = false;
  for (h.t = 0; h.t < limit; ++h.t) {
    robot->tick();
    w.step(0.001, h.lastL, h.lastR);
    if (h.t % 20 == 0) w.trail.push_back({w.x, w.y});
    if (h.t % 5 == 0) w.markReadable();
    if (g_traceFrom >= 0 && (int)h.t >= g_traceFrom && (int)h.t <= g_traceTo && h.t % 20 == 0)
      std::printf("    t=%u x=%.1f y=%.1f th=%.0f bits=%02X L=%d R=%d st=%d e18=%d%d\n", h.t, w.x, w.y, w.th * 57.2958, w.lineBits(), h.lastL, h.lastR, (int)robot->state(), w.e18(0), w.e18(1));
    if (!restarted && restartAt > 0 && (int)h.t == restartAt) {
      // trọng tài cho restart: đặt xe về XUẤT PHÁT, nhấn RESET (bộ nhớ Backup còn nguyên)
      restarted = true;
      w.x = 150; w.y = -35; w.th = 1.5707963267948966; w.vl = w.vr = 0;
      robot = std::make_unique<Robot>(h, p);
      robot->begin(BootMode::Run);
      // thời gian thi vẫn tính liên tục: firmware mới bắt đầu đếm lại, nên giới hạn tổng ở ngoài
    }
    if (robot->state() == State::Goal) break;
  }
  m.timeS = h.t / 1000.0;
  double ax, ay; sim::nodeXY(0, ax, ay);
  m.finished = robot->state() == State::Goal && std::hypot(w.x - ax, w.y - ay) < 25 && h.t <= 300000;
  for (auto& ob : w.objects) {
    if (ob.color != sim::Color::Red) continue;
    if (ob.reports > 0) m.redsHit++;
    if (ob.readable) { m.redsReadable++; if (ob.reports > 0) m.readableHit++; }
  }
  m.covered = coverage(w);
  m.fallback = robot->stats().fallback;
  m.mismatches = robot->stats().mismatches;
  m.restarts = robot->stats().restarts;
  m.grabbed = robot->stats().grabbed;
  if (verbose) {
    for (auto& ob : w.objects) {
      const auto& sg = w.segs[ob.edge];
      double ux = (sg.x2 - sg.x1) / 150, uy = (sg.y2 - sg.y1) / 150;
      double t = (ob.x - sg.x1) * ux + (ob.y - sg.y1) * uy;
      double lat = std::fabs((ob.x - sg.x1) * -uy + (ob.y - sg.y1) * ux) - ob.r - 1.5;
      if ((ob.color == sim::Color::Red && ob.reports != 1) || (ob.color != sim::Color::Red && ob.reports))
        std::printf("  !! vat %s c%d ben %d s=%.0f cach_mep_line=%.1f bao=%d\n", sim::colorName(ob.color), ob.edge + 1, ob.side, t, lat, ob.reports);
    }
  }
  if (svg) writeSvg(svg, w, h);
  if (g_trail) writeTrail(g_trail, w, h);
  return m;
}

}  // namespace

int main(int argc, char** argv) {
  int seeds = 20, start = 1, i2cFail = -1, restartAt = -1;
  double noise = 0.002, sideY = 4.0, colorRange = 4.0;
  bool orange = false, calib = false, verbose = false, gripper = false;
  const char* svg = nullptr;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--seeds") seeds = std::atoi(argv[++i]);
    else if (a == "--start") start = std::atoi(argv[++i]);
    else if (a == "--svg") svg = argv[++i];
    else if (a == "--orange") orange = true;
    else if (a == "--calib") calib = true;
    else if (a == "--i2c-fail") i2cFail = std::atoi(argv[++i]);
    else if (a == "--restart-at") restartAt = std::atoi(argv[++i]);
    else if (a == "--gripper") gripper = true;
    else if (a == "--verbose") verbose = true;
    else if (a == "--trace") { g_traceFrom = std::atoi(argv[++i]); g_traceTo = std::atoi(argv[++i]); }
    else if (a == "--trail") g_trail = argv[++i];
    else if (a == "--side-y") sideY = std::atof(argv[++i]);
    else if (a == "--color-range") colorRange = std::atof(argv[++i]);
    else if (a == "--noise") noise = std::atof(argv[++i]);
  }
  int rdb = 0, rdbHit = 0, grabbed = 0;
  int reds = 0, hits = 0, fa = 0, dup = 0, ph = 0, fin = 0, cov12 = 0, fb = 0;
  std::vector<double> times;
  std::printf("seed  do(trung/tong)  nham  lap  ma  phu  ve_dich  t(s)  lech  du_phong\n");
  for (int s = start; s < start + seeds; ++s) {
    sim::Options o;
    o.seed = (uint32_t)s; o.orange = orange; o.i2cFailAtMs = i2cFail; o.lineNoise = noise; o.sideY = sideY; o.colorRange = colorRange;
    Metrics m = runOne(o, calib, restartAt, gripper, verbose, (svg && s == start) ? svg : nullptr);
    std::printf("%4d   %2d/%2d          %2d    %2d   %2d   %2d   %s     %5.1f  %2d   %s\n", s, m.redsHit, m.redsTotal, m.falseAlarms,
                m.duplicates, m.phantom, m.covered, m.finished ? "co " : "KHONG", m.timeS, m.mismatches, m.fallback ? "co" : "-");
    grabbed += m.grabbed;
    rdb += m.redsReadable; rdbHit += m.readableHit;
    reds += m.redsTotal; hits += m.redsHit; fa += m.falseAlarms; dup += m.duplicates; ph += m.phantom;
    fin += m.finished; cov12 += m.covered == 12; fb += m.fallback;
    times.push_back(m.timeS);
  }
  std::sort(times.begin(), times.end());
  double med = times[times.size() / 2];
  double hitRate = reds ? 100.0 * hits / reds : 0;
  double fwRate = rdb ? 100.0 * rdbHit / rdb : 0;
  std::printf("\nFIRMWARE: bat dung %.1f%% vat do NAM TRONG TAM cam bien (%d/%d); HE THONG: %.1f%% tong vat do\n", fwRate, rdbHit, rdb, hitRate);
  std::printf("\nTONG KET %d luot: bat do %.1f%% (%d/%d), bao nham %d, bao lap %d, bao ma %d, ve dich %d/%d, phu du 12 cung %d/%d, du phong %d, t trung vi %.1fs\n",
              seeds, hitRate, hits, reds, fa, dup, ph, fin, seeds, cov12, seeds, fb, med);
  if (gripper) std::printf("GAP (Vong 2): %d vat da gap len xe\n", grabbed);
  bool pass = fwRate >= 95 && fa == 0 && dup == 0 && ph == 0 && fin * 100 >= 98 * seeds && cov12 * 100 >= 98 * seeds && med < 180;
  std::printf("KET QUA: %s\n", pass ? "DAT" : "CHUA DAT");
  return pass ? 0 : 1;
}
