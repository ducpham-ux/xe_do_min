#include "world.h"
#include <algorithm>
#include <cmath>

namespace sim {

namespace {
const int kEdges[12][2] = {{0, 1}, {1, 2}, {0, 3}, {1, 4}, {2, 5}, {3, 4},
                           {4, 5}, {3, 6}, {4, 7}, {5, 8}, {6, 7}, {7, 8}};
const double kHalfLine = 1.4;  // line rộng 2,8 cm

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
  // loa bo cung tròn ở 4 góc phần tư của B, E, H
  const int flareNodes[3] = {1, 4, 7};
  const double R = opt.flareR;
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
      double r = 2 + 2 * u01(rng_);                         // đường kính 4-8 cm
      double gap = opt.gapMin + (opt.gapMax - opt.gapMin) * u01(rng_);
      bool ok = true;
      const Seg& s = segs[edge];
      double ux = (s.x2 - s.x1) / 150, uy = (s.y2 - s.y1) / 150;
      for (auto& ob : objects) {
        if (ob.edge != edge || ob.side != side) continue;
        double tt = (ob.x - s.x1) * ux + (ob.y - s.y1) * uy;
        if (std::fabs(tt - t) < ob.r + r + 3) ok = false;   // cho phép cụm vật sát nhau
      }
      if (!ok) continue;
      double nxv = -uy, nyv = ux;  // pháp tuyến trái
      double off = kHalfW + gap + r;
      double sgn = side == 0 ? 1 : -1;
      objects.push_back({s.x1 + ux * t + sgn * nxv * off, s.y1 + uy * t + sgn * nyv * off, r, c, edge, side});
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
  // i: 0=L2 1=L1 2=M 3=R1 4=R2 ; trái = +y trong hệ xe; thanh thẳng, mắt cách 1,75 cm
  const double off[5] = {3.5, 1.75, 0, -1.75, -3.5};
  double fx = opt.barAhead, fy = off[i];
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

// Tia vuông góc từ sườn xe (điểm dọc x0 so với tâm quay) ra ngoài.
double World::rayHit(int side, double x0, const Object& o) const {
  double sy = side == 0 ? kHalfW : -kHalfW;
  double ox = x + x0 * std::cos(th) - sy * std::sin(th), oy = y + x0 * std::sin(th) + sy * std::cos(th);
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

int World::objectBeside(int side, double x0, double maxDist, double* dist) const {
  int best = -1; double bd = 1e9;
  for (size_t i = 0; i < objects.size(); ++i) {
    double d = rayHit(side, x0, objects[i]);
    if (d >= 0 && d <= maxDist && d < bd) { bd = d; best = (int)i; }
  }
  if (dist) *dist = bd;
  return best;
}

bool World::e18(int side) const { return objectBeside(side, opt.e18X, kE18Range) >= 0; }

bool World::colorReading(uint16_t& r, uint16_t& g, uint16_t& b) {
  std::normal_distribution<double> nz(1.0, opt.colorNoise);
  double d = 0;
  int idx = objectBeside(0, opt.tcsX, 6.0, &d);
  double R, G, B;
  if (idx < 0) { R = 1500; G = 1600; B = 1400; }   // không có vật: nền tối, chu kỳ dài
  else {
    switch (objects[idx].color) {
      case Color::Red: R = 250; G = 600; B = 550; break;
      case Color::Yellow: R = 220; G = 260; B = 500; break;
      case Color::Orange: R = 235; G = 430; B = 560; break;
      case Color::Green: R = 500; G = 300; B = 450; break;
      default: R = 300; G = 600; B = 350; break;
    }
    // ánh sáng phản xạ giảm nhanh theo khoảng cách: tốt nhất ~1,5 cm
    double f = 1 + 0.5 * std::max(0.0, d - 1.5);
    f *= f;
    R *= f; G *= f; B *= f;
  }
  auto cl = [](double v) { return (uint16_t)std::min(3000.0, std::max(1.0, v)); };
  r = cl(R * nz(rng_)); g = cl(G * nz(rng_)); b = cl(B * nz(rng_));
  return true;
}

// Thân xe (hình chữ nhật 26 x 16) chạm vật nào thì đánh dấu.
void World::checkCollisions() {
  double c = std::cos(th), s = std::sin(th);
  for (auto& o : objects) {
    if (o.hit) continue;
    double dx = o.x - x, dy = o.y - y;
    double lx = dx * c + dy * s, ly = -dx * s + dy * c;   // vào hệ xe
    double qx = std::max(-kHalfL, std::min(kHalfL, lx)), qy = std::max(-kHalfW, std::min(kHalfW, ly));
    if (std::hypot(lx - qx, ly - qy) < o.r) o.hit = true;
  }
}

int World::collisions() const {
  int n = 0;
  for (auto& o : objects) n += o.hit;
  return n;
}

}  // namespace sim
