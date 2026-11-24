#include "world.h"
#include <algorithm>
#include <cmath>

namespace sim {

namespace {
const double kPi = 3.14159265358979323846;
const int kEdges[12][2] = {{0, 1}, {1, 2}, {0, 3}, {1, 4}, {2, 5}, {3, 4},
                           {4, 5}, {3, 6}, {4, 7}, {5, 8}, {6, 7}, {7, 8}};
const double kHalfLine = 1.5;  // line rộng 3 cm

double distSeg(double px, double py, const Seg& s) {
  double dx = s.x2 - s.x1, dy = s.y2 - s.y1;
  double t = ((px - s.x1) * dx + (py - s.y1) * dy) / (dx * dx + dy * dy);
  t = std::max(0.0, std::min(1.0, t));
  double qx = s.x1 + t * dx - px, qy = s.y1 + t * dy - py;
  return std::sqrt(qx * qx + qy * qy);
}
}  // namespace

const char* colorName(Color c) {
  switch (c) {
    case Color::Red: return "red";
    case Color::Yellow: return "gold";
    case Color::Orange: return "orange";
    case Color::Green: return "green";
    default: return "purple";
  }
}

void nodeXY(int node, double& x, double& y) {
  x = (node % 3) * 150.0;
  y = (2 - node / 3) * 150.0;
}

World::World(const Options& o) : opt(o), rng_(o.seed) {
  for (auto& e : kEdges) {
    double x1, y1, x2, y2;
    nodeXY(e[0], x1, y1); nodeXY(e[1], x2, y2);
    segs.push_back({x1, y1, x2, y2});
  }
  segs.push_back({150, 0, 150, -45});    // nhánh XUẤT PHÁT
  segs.push_back({150, 300, 150, 340});  // nhánh VÒNG 1
  // loa bo cung tròn R = 15 cm ở 4 góc phần tư của B, E, H
  const int flareNodes[3] = {1, 4, 7};
  const double R = 15;
  for (int n : flareNodes) {
    double nx, ny; nodeXY(n, nx, ny);
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2) {
        Arc a;
        a.cx = nx + sx * R; a.cy = ny + sy * R; a.R = R;
        a.xmin = std::min(nx, a.cx); a.xmax = std::max(nx, a.cx);
        a.ymin = std::min(ny, a.cy); a.ymax = std::max(ny, a.cy);
        arcs.push_back(a);
      }
  }

  std::uniform_real_distribution<double> u01(0, 1);
  std::uniform_real_distribution<double> g(-1, 1);
  gainL_ = 1 + opt.motorGainSpread * g(rng_);
  gainR_ = 1 + opt.motorGainSpread * g(rng_);

  auto place = [&](int edge, Color c) {
    for (int tries = 0; tries < 200; ++tries) {
      double t = 30 + 90 * u01(rng_);
      int side = u01(rng_) < 0.5 ? 0 : 1;
      double d = 3 + 4 * u01(rng_);  // khoảng cách mép line -> mép vật: 5 +- 2 cm
      double r = 3;
      bool ok = true;
      for (auto& ob : objects) {
        if (ob.edge != edge || ob.side != side) continue;
        const Seg& s = segs[edge];
        double ux = (s.x2 - s.x1) / 150, uy = (s.y2 - s.y1) / 150;
        double tt = (ob.x - s.x1) * ux + (ob.y - s.y1) * uy;
        if (std::fabs(tt - t) < 15) ok = false;
      }
      if (!ok) continue;
      const Seg& s = segs[edge];
      double ux = (s.x2 - s.x1) / 150, uy = (s.y2 - s.y1) / 150;
      double nxv = -uy, nyv = ux;  // pháp tuyến trái
      double off = kHalfLine + d + r;
      double sgn = side == 0 ? 1 : -1;
      objects.push_back({s.x1 + ux * t + sgn * nxv * off, s.y1 + uy * t + sgn * nyv * off, r, c, edge, side, 0});
      return;
    }
  };
  std::vector<int> es(12);
  for (int i = 0; i < 12; ++i) es[i] = i;
  std::shuffle(es.begin(), es.end(), rng_);
  for (int i = 0; i < opt.reds; ++i) place(es[i % 12], Color::Red);
  std::vector<Color> dc = {Color::Yellow, Color::Green, Color::Purple};
  if (opt.orange) dc.push_back(Color::Orange);
  for (int i = 0; i < opt.distractors; ++i) place((int)(u01(rng_) * 12) % 12, dc[i % dc.size()]);
}

bool World::onLine(double px, double py) const {
  for (auto& s : segs) if (distSeg(px, py, s) <= kHalfLine) return true;
  for (auto& a : arcs) {
    if (px < a.xmin - kHalfLine || px > a.xmax + kHalfLine || py < a.ymin - kHalfLine || py > a.ymax + kHalfLine) continue;
    double d = std::hypot(px - a.cx, py - a.cy);
    if (std::fabs(d - a.R) <= kHalfLine) return true;
  }
  return false;
}

void World::step(double dt, int pwmL, int pwmR) {
  auto target = [&](int p, double gain) {
    double m = std::abs(p) - opt.deadband;
    if (m < 0) m = 0;
    return (p >= 0 ? 1 : -1) * m * opt.speedGain * gain;
  };
  vl += (target(pwmL, gainL_) - vl) * dt / opt.tau;
  vr += (target(pwmR, gainR_) - vr) * dt / opt.tau;
  double v = (vl + vr) / 2, w = (vr - vl) / kWheelBase;
  x += v * std::cos(th) * dt;
  y += v * std::sin(th) * dt;
  th += w * dt;
}

void World::sensorPos(int i, double& px, double& py) const {
  // i: 0=L2 1=L1 2=M 3=R1 4=R2 ; trái = +y trong hệ xe
  const double ang[5] = {60, 30, 0, -30, -60};
  double a = ang[i] * kPi / 180;
  double fx = 6 + 5 * std::cos(a), fy = 5 * std::sin(a);
  px = x + fx * std::cos(th) - fy * std::sin(th);
  py = y + fx * std::sin(th) + fy * std::cos(th);
}

uint8_t World::lineBits() {
  std::uniform_real_distribution<double> u01(0, 1);
  uint8_t b = 0;
  for (int i = 0; i < 5; ++i) {
    double px, py; sensorPos(i, px, py);
    bool on = onLine(px, py);
    if (u01(rng_) < opt.lineNoise) on = !on;
    if (on) b |= (uint8_t)(1u << i);
  }
  return b;
}

double World::rayHit(int side, const Object& o) const {
  double sy = side == 0 ? opt.sideY : -opt.sideY;
  double ox = x - sy * std::sin(th), oy = y + sy * std::cos(th);
  double dx = side == 0 ? -std::sin(th) : std::sin(th);
  double dy = side == 0 ? std::cos(th) : -std::cos(th);
  double cx = o.x - ox, cy = o.y - oy;
  double proj = cx * dx + cy * dy;
  double perp2 = cx * cx + cy * cy - proj * proj;
  if (perp2 > o.r * o.r) return -1;
  double tHit = proj - std::sqrt(o.r * o.r - perp2);
  if (proj < 0) return -1;
  return tHit < 0 ? 0 : tHit;
}

int World::objectBeside(int side, double maxDist) const {
  int best = -1; double bd = 1e9;
  for (size_t i = 0; i < objects.size(); ++i) {
    double d = rayHit(side, objects[i]);
    if (d >= 0 && d <= maxDist && d < bd) { bd = d; best = (int)i; }
  }
  return best;
}

void World::markReadable() {
  for (int s = 0; s < 2; ++s) {
    int i = objectBeside(s, opt.colorRange);
    if (i >= 0) objects[i].readable = true;
  }
}

bool World::e18(int side) const { return objectBeside(side, kE18Range) >= 0; }

bool World::colorReading(int side, uint16_t& r, uint16_t& g, uint16_t& b, uint16_t& c, uint32_t nowMs) {
  if (side == 1 && opt.i2cFailAtMs >= 0 && (int)nowMs >= opt.i2cFailAtMs) return false;
  std::normal_distribution<double> nz(1.0, opt.colorNoise);
  int idx = objectBeside(side, opt.colorRange);
  double d = 0;
  Color col = Color::Red;
  bool have = idx >= 0;
  if (have) { col = objects[idx].color; d = rayHit(side, objects[idx]); }
  if (side == 0) {  // TCS3200: chu kỳ (us), nhỏ = sáng
    double R, G, B;
    if (!have) { R = 900; G = 950; B = 850; }
    else switch (col) {
      case Color::Red: R = 250; G = 600; B = 550; break;
      case Color::Yellow: R = 220; G = 260; B = 500; break;
      case Color::Orange: R = 215; G = 380; B = 560; break;
      case Color::Green: R = 500; G = 300; B = 450; break;
      default: R = 300; G = 600; B = 350; break;
    }
    double f = 1 + 0.15 * std::max(0.0, d - 1.5);
    r = (uint16_t)(R * f * nz(rng_)); g = (uint16_t)(G * f * nz(rng_)); b = (uint16_t)(B * f * nz(rng_)); c = 0;
  } else {  // TCS34725: số đếm, lớn = sáng (LED cam làm kênh G cao hơn thực tế)
    double R, G, B;
    if (!have) { R = 20; G = 15; B = 12; }
    else switch (col) {
      case Color::Red: R = 200; G = 60; B = 50; break;
      case Color::Yellow: R = 140; G = 150; B = 60; break;
      case Color::Orange: R = 230; G = 150; B = 55; break;
      case Color::Green: R = 60; G = 200; B = 90; break;
      default: R = 120; G = 70; B = 110; break;
    }
    double f = 1 / (1 + 0.2 * std::max(0.0, d - 1.5));
    r = (uint16_t)(R * f * nz(rng_)); g = (uint16_t)(G * f * nz(rng_)); b = (uint16_t)(B * f * nz(rng_));
    c = (uint16_t)(r + g + b);
  }
  return true;
}

}  // namespace sim
