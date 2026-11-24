#include "pid.h"

namespace core {

Pid::Pid(float kp, float ki, float kd, float iLimit) : kp_(kp), ki_(ki), kd_(kd), iLimit_(iLimit) {}

float Pid::update(float err, float dt) {
  if (dt <= 0) dt = 1e-3f;
  integ_ += ki_ * err * dt;
  if (integ_ > iLimit_) integ_ = iLimit_;
  if (integ_ < -iLimit_) integ_ = -iLimit_;
  float d = hasPrev_ ? (err - prevErr_) / dt : 0.0f;
  prevErr_ = err;
  hasPrev_ = true;
  return kp_ * err + integ_ + kd_ * d;
}

void Pid::reset() {
  integ_ = 0;
  prevErr_ = 0;
  hasPrev_ = false;
}

}  // namespace core
