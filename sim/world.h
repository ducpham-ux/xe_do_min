// world.h - mô hình sân thi đấu + xe để chạy thử firmware trên PC.
// Đơn vị: cm, giây. Gốc tọa độ tại góc G (dưới-trái), trục y hướng Bắc.
// Hình học xe theo test lần 1: thân 26 x 16 cm, tâm quay ở giữa; thanh 5 mắt THẲNG 7 cm ở mũi,
// cách tâm quay 12 cm; E18 hai sườn (tầm 7 cm); TCS3200 sườn trái (đọc tốt ở ~1,5 cm).
#pragma once
#include <cstdint>
#include <random>
#include <vector>

namespace sim {

enum class Color { Red, Yellow, Orange, Green, Purple };
const char* colorName(Color c);

struct Object {
  double x, y, r;       // tâm, bán kính
  Color color;
  int edge;             // cung 0..11
  int side;             // 0 = trái, 1 = phải (nhìn theo chiều từ nút chỉ số nhỏ -> lớn)
  int reports = 0;      // số lần xe báo vật này
  bool hit = false;     // thân xe đã va vào vật
};

struct Seg { double x1, y1, x2, y2; };
struct Arc { double cx, cy, R, xmin, xmax, ymin, ymax; };

struct Options {
  uint32_t seed = 1;
  int reds = 8;
  int distractors = 16;
  bool orange = false;          // thêm vật nhiễu màu cam
  double lineNoise = 0.002;     // xác suất mỗi mắt đọc sai mỗi lần
  double motorGainSpread = 0.06;// lệch hệ số giữa 2 động cơ
  double speedGain = 0.35;      // cm/s mỗi PWM trên vùng chết
  double deadband = 30;
  double tau = 0.06;            // quán tính bánh (s)
  double colorNoise = 0.04;
  double flareR = 20;           // bán kính cung bo ở B, E, H
  double gapMin = 1, gapMax = 7;// khe hở từ SƯỜN xe (khi xe nằm giữa line) tới mặt vật
  double barAhead = 12;         // thanh mắt trước tâm quay
  double tcsX = 0, e18X = 0;    // vị trí dọc của TCS3200 và E18 so với tâm quay
};

class World {
 public:
  explicit World(const Options& o);
  // xe
  double x = 150, y = -35, th = 1.5707963267948966;  // đặt trên nhánh XUẤT PHÁT, hướng Bắc
  double vl = 0, vr = 0;
  void step(double dt, int pwmL, int pwmR);
  uint8_t lineBits();
  bool e18(int side) const;               // side 0 = trái
  bool colorReading(uint16_t& r, uint16_t& g, uint16_t& b);   // TCS3200 sườn trái
  int objectBeside(int side, double x0, double maxDist, double* dist = nullptr) const;
  bool onLine(double px, double py) const;
  void checkCollisions();
  int collisions() const;

  std::vector<Object> objects;
  std::vector<Seg> segs;
  std::vector<Arc> arcs;
  std::vector<std::pair<double, double>> trail;
  Options opt;

  static constexpr double kWheelBase = 14.0;
  static constexpr double kHalfW = 8.0, kHalfL = 13.0;
  static constexpr double kE18Range = 7.0;

 private:
  void sensorPos(int i, double& px, double& py) const;
  double rayHit(int side, double x0, const Object& o) const;  // khoảng cách dọc tia tới vật, <0 nếu không trúng
  std::mt19937 rng_;
  double gainL_ = 1, gainR_ = 1;
};

void nodeXY(int node, double& x, double& y);

}  // namespace sim
