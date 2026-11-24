// pid.h - bộ điều khiển PID rời rạc có kẹp tích phân
#pragma once

namespace core {
class Pid {
 public:
  Pid(float kp, float ki, float kd, float iLimit);
  float update(float err, float dt);  // dt tính bằng giây
  void reset();
  void setGains(float kp, float ki, float kd) { kp_ = kp; ki_ = ki; kd_ = kd; }

 private:
  float kp_, ki_, kd_, iLimit_;
  float integ_ = 0, prevErr_ = 0;
  bool hasPrev_ = false;
};
}  // namespace core
