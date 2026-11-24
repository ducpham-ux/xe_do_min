#include "gripper.h"

namespace core {

void Gripper::apply(uint8_t step, Hal& hal) {
  switch (step) {
    case 0: hal.servo(1, p_.gripOpenDeg); break;   // mở kẹp
    case 1: hal.servo(0, p_.armDownDeg); break;    // hạ tay
    case 2: hal.servo(1, p_.gripCloseDeg); break;  // kẹp
    case 3: hal.servo(0, p_.armUpDeg); break;      // nâng
    default: break;
  }
}

void Gripper::start(uint32_t now, Hal& hal) {
  step_ = 0;
  at_ = now;
  apply(0, hal);
}

bool Gripper::update(uint32_t now, Hal& hal) {
  if (step_ >= 4) return false;
  if (now - at_ < p_.gripStepMs) return true;
  ++step_;
  at_ = now;
  if (step_ >= 4) return false;
  apply(step_, hal);
  return true;
}

void Gripper::park(Hal& hal) {
  hal.servo(0, p_.armUpDeg);
  hal.servo(1, p_.gripCloseDeg);
}

}  // namespace core
