// world.h - mô hình sân thi đấu + xe để chạy thử firmware trên PC.
// Đơn vị: cm, giây. Gốc tọa độ tại góc G (dưới-trái), trục y hướng Bắc.
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
  bool readable = false;// đã từng nằm trong tầm đọc màu của cảm biến bên tương ứng
};

struct Seg { double x1, y1, x2, y2; };
struct Arc { double cx, cy, R, xmin, xmax, ymin, ymax; };

struct Options {
  uint32_t seed = 1;
  int reds = 8;
  int distractors = 16;
  bool orange = false;          // thêm vật nhiễu màu cam (khó với TCS34725)
  double lineNoise = 0.002;     // xác suất mỗi mắt đọc sai mỗi lần
  double motorGainSpread = 0.06;// lệch hệ số giữa 2 động cơ
  double speedGain = 0.35;      // cm/s mỗi PWM trên vùng chết
  double deadband = 30;
  double tau = 0.06;            // quán tính bánh (s)
  double colorNoise = 0.04;
  int i2cFailAtMs = -1;         // >=0: cảm biến phải hỏng từ thời điểm này
  double sideY = 4.0;           // cảm biến sườn (E18 + màu) cách tâm xe (cm)
  double colorRange = 4.0;      // tầm đọc màu tối đa (cm)
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
  bool colorReading(int side, uint16_t& r, uint16_t& g, uint16_t& b, uint16_t& c, uint32_t nowMs);
  int objectBeside(int side, double maxDist) const;  // chỉ số vật đang ở cạnh cảm biến, -1 nếu không có
  bool onLine(double px, double py) const;

  std::vector<Object> objects;
  std::vector<Seg> segs;
  std::vector<Arc> arcs;
  std::vector<std::pair<double, double>> trail;
  Options opt;

  static constexpr double kWheelBase = 14.0;
  static constexpr double kE18Range = 12.0;
  void markReadable();

 private:
  void sensorPos(int i, double& px, double& py) const;
  double rayHit(int side, const Object& o) const;  // khoảng cách dọc tia tới vật, <0 nếu không trúng
  std::mt19937 rng_;
  double gainL_ = 1, gainR_ = 1;
};

void nodeXY(int node, double& x, double& y);

}  // namespace sim
