// pid.h - bộ điều khiển PID rời rạc có kẹp tích phân và lọc thông thấp khâu D.
// Vị trí line từ mắt số nhảy bậc (0,875 cm) -> đạo hàm thô có gai rất lớn; lọc với hằng số dTau.
#pragma once

namespace core {
class Pid {
 public:
  Pid(float kp, float ki, float kd, float iLimit, float dTau = 0.0f);
  float update(float err, float dt);  // dt tính bằng giây
  void reset();
  void setGains(float kp, float ki, float kd) { kp_ = kp; ki_ = ki; kd_ = kd; }

 private:
  float kp_, ki_, kd_, iLimit_, dTau_;
  float integ_ = 0, prevErr_ = 0, dFilt_ = 0;
  bool hasPrev_ = false;
};
}  // namespace core
