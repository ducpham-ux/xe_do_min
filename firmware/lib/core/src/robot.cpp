#include "robot.h"
#include <math.h>
#include <stdio.h>

namespace core {

namespace {
inline Sensor sensorOf(Side s) { return s == Side::Left ? Sensor::Tcs3200 : Sensor::Tcs34725; }
inline int16_t clampPwm(float v, int16_t lim) {
  if (v > lim) return lim;
  if (v < -lim) return (int16_t)-lim;
  return (int16_t)v;
}
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
}  // namespace

Robot::Robot(Hal& hal, const Params& p)
    : hal_(hal), p_(p), nd_(p), pid_(p.kp, p.ki, p.kd, p.iLimit), tracker_(p) {
  vEst_ = p.vBaseCmS;
}

void Robot::enter(State s) {
  if (s == State::Recover) lineFound_.reset();
  state_ = s;
  stateAt_ = hal_.millis();
}

// ---------------------------------------------------------------- khởi động
void Robot::begin(BootMode mode) {
  Calib c;
  if (hal_.loadCalib(c) && c.magic == Calib::kMagic) cal_ = c;
  hal_.setMotors(0, 0);
  hal_.buzzer(false);
  hal_.led(false);

  if (mode == BootMode::Calib) {
    cal_ = Calib();
    calibStep_ = 0; calibCount_ = 0; calibWaitClear_ = false;
    enter(State::Calib);
    hal_.log("CALIB: dua mau DO vao ben TRAI");
    return;
  }
  if (mode == BootMode::ClearMemory) {
    uint16_t zero[kBackupRegs] = {};
    hal_.saveBackup(zero, kBackupRegs);
  }

  uint16_t regs[kBackupRegs];
  uint16_t covered = 0; uint8_t restarts = 0;
  bool restored = mode == BootMode::Run && hal_.loadBackup(regs, kBackupRegs) &&
                  decodeBackup(regs, covered, tracker_, restarts);
  if (restored) {
    stats_.restarts = (uint8_t)(restarts + 1);
    stats_.covered = covered;
    stats_.alerts = tracker_.count();
    planLen_ = planCoverage(kH, Dir::N, covered, kA, plan_, sizeof(plan_) / sizeof(plan_[0]));
    char buf[64];
    snprintf(buf, sizeof buf, "RESTART #%u: da phu %03X, da bao %u vat", stats_.restarts, covered, stats_.alerts);
    hal_.log(buf);
  } else {
    tracker_.clear();
    planLen_ = 0;
  }
  if (planLen_ == 0) {
    for (uint8_t i = 0; i < kDefaultRouteLen; ++i) plan_[i] = kDefaultRoute[i];
    planLen_ = kDefaultRouteLen;
  }
  planIdx_ = 0;
  lastNode_ = kStart; node_ = kH; heading_ = Dir::N;
  edgeLen_ = p_.startStubCm; dist_ = 0;
  persist();
  enter(State::Ready);
}

// ---------------------------------------------------------------- vòng lặp
void Robot::tick() {
  uint32_t now = hal_.millis();
  if (ticked_ && now == lastTick_) return;  // tối đa 1 lần / ms
  float dt = ticked_ ? (now - lastTick_) / 1000.0f : 0.001f;
  ticked_ = true;
  lastTick_ = now;
  uint8_t bits = hal_.readLine();

  bool running = state_ != State::Boot && state_ != State::Calib && state_ != State::Ready && state_ != State::Goal;
  if (running) {
    uint32_t el = now - matchStart_;
    if (el >= p_.matchMs) { hal_.log("HET GIO"); finish(now); return; }
    if (el >= p_.goHomeMs && nav_ == NavMode::Map && !stats_.wentHome && state_ == State::Follow) {
      uint8_t n = planPath(node_, heading_, kA, plan_, sizeof(plan_) / sizeof(plan_[0]));
      if (n) { planLen_ = n; planIdx_ = 0; stats_.wentHome = true; hal_.log("GAN HET GIO -> VE DICH"); }
    }
  }

  switch (state_) {
    case State::Boot: break;
    case State::Calib: doCalib(now); break;
    case State::Ready: doReady(now); break;
    case State::Follow: doFollow(now, bits, dt); break;
    case State::NodeScan: doNodeScan(now, bits, dt); break;
    case State::Turn: doTurn(now, bits); break;
    case State::ScanObject: doScanObject(now, dt); break;
    case State::AlertOn: doAlert(now); break;
    case State::Grip: doGrip(now); break;
    case State::Recover: doRecover(now, bits); break;
    case State::Goal: hal_.setMotors(0, 0); break;
  }
}

void Robot::doReady(uint32_t now) {
  hal_.setMotors(0, 0);
  hal_.led(((now - stateAt_) / 250) % 2 == 0);
  if (now - stateAt_ >= p_.startDelayMs) {
    hal_.led(false);
    matchStart_ = now;
    lastNodeAt_ = now;
    lastCtrl_ = now;
    pid_.reset();
    enter(State::Follow);
    hal_.log("START");
  }
}

// ---------------------------------------------------------------- động cơ
void Robot::drive(int16_t l, int16_t r) {
  cmdL_ = l; cmdR_ = r;
  hal_.setMotors(clampPwm(l * pwmScale_, 255), clampPwm(r * pwmScale_, 255));
}

void Robot::advance(int16_t l, int16_t r, float dt) {
  // vận tốc ~ (PWM - vùng chết), chuẩn hóa theo baseSpeed
  float eff = (l + r) * 0.5f - p_.pwmDeadband;
  if (eff <= 0) return;
  float ds = vEst_ * (eff / (p_.baseSpeed - p_.pwmDeadband)) * dt;
  dist_ += ds;
  odo_ += ds;
}

// ---------------------------------------------------------------- vật cản
bool Robot::obstacleEdge(Side s, uint32_t) {
  uint8_t i = (uint8_t)s;
  if (hal_.obstacle(s)) {
    if (obsRun_[i] < 255) ++obsRun_[i];
  } else {
    obsRun_[i] = 0;
    obsArmed_[i] = true;  // đã thấy LOW -> vật tiếp theo là vật mới
  }
  float since = odo_ - distAtScan_[i];
  if (!obsArmed_[i] && since > 20.0f) obsArmed_[i] = true;  // E18 kẹt HIGH quá lâu
  if (obsRun_[i] >= p_.obstacleDebounce && obsArmed_[i] && since >= p_.obstacleRearmCm) {
    obsArmed_[i] = false;
    distAtScan_[i] = odo_;
    return true;
  }
  return false;
}

// ---------------------------------------------------------------- bám line
void Robot::doFollow(uint32_t now, uint8_t bits, float dt) {
  for (uint8_t i = 0; i < 2; ++i) {
    Side s = (Side)i;
    if (obstacleEdge(s, now)) {
      scanSide_ = s;
      drive(0, 0);
      samples_ = 0; sampleErrors_ = 0;
      scanStartOdo_ = odo_; scanLowRun_ = 0;
      vote_.reset();
      nextSampleAt_ = now + p_.settleMs;
      enter(State::ScanObject);
      return;
    }
  }

  // mất line
  if (bits == 0) {
    if (!lost_) { lost_ = true; lostSince_ = now; }
    else if (now - lostSince_ >= p_.lostConfirmMs) {
      lost_ = false;
      if (nav_ == NavMode::Fallback) {
        if (now - matchStart_ < p_.fallbackMinExploreMs) startTurn(now, Act::UTurn, true);
        else finish(now);
      } else {
        enter(State::Recover);
      }
      return;
    }
  } else {
    lost_ = false;
  }

  // nút giao
  bool gate = nav_ == NavMode::Map ? dist_ >= p_.nodeGateFrac * edgeLen_ : dist_ >= p_.fallbackGateCm;
  if (gate) {
    if (nd_.feedCandidate(bits)) {
      learnDist_ = dist_;
      nd_.beginScan();
      nd_.accumulate(bits);
      drive(p_.scanSpeed, p_.scanSpeed);
      enter(State::NodeScan);
      return;
    }
  } else {
    nd_.reset();
  }

  // PID
  bool innerOnly = dist_ < p_.flareZoneCm || gate;
  float pos = linePosition(bits, p_, innerOnly, lastPos_, nullptr);
  lastPos_ = pos;
  float v = p_.baseSpeed;
  if (nav_ == NavMode::Map && dist_ > p_.approachFrac * edgeLen_) v = p_.approachSpeed;
  if (fabsf(pos) >= 4.0f) v *= 0.7f;
  if (now - lastCtrl_ >= p_.ctrlMs) {
    float u = pid_.update(pos, (now - lastCtrl_) / 1000.0f);
    lastCtrl_ = now;
    drive(clampPwm(v + u, p_.maxPwm), clampPwm(v - u, p_.maxPwm));
  }
  advance(cmdL_, cmdR_, dt);
}

// ---------------------------------------------------------------- nút giao
void Robot::doNodeScan(uint32_t now, uint8_t bits, float dt) {
  drive(p_.scanSpeed, p_.scanSpeed);
  advance(cmdL_, cmdR_, dt);
  nd_.accumulate(bits);
  uint32_t t = now - stateAt_;
  bool done = p_.crawlCm > 0 ? (dist_ - learnDist_ >= p_.crawlCm || t > 1500) : t >= p_.scanTimeMs;
  if (!done) return;
  NodeShape shape = nd_.finish(bits);
  nd_.reset();
  if (!shape.isNode()) {
    hal_.log("nut gia -> bo qua");
    enter(State::Follow);
    return;
  }
  onNode(now, shape);
}

void Robot::onNode(uint32_t now, const NodeShape& shape) {
  stats_.nodesSeen++;
  lastNodeAt_ = now;
  char buf[80];

  if (nav_ == NavMode::Map) {
    // học vận tốc thật từ chiều dài cung đã biết
    if (learnOk_ && edgeLen_ >= p_.edgeLenCm - 1 && learnDist_ > 10) {
      float real = p_.edgeLenCm - p_.sensorAheadCm;
      float vTrue = vEst_ * real / learnDist_;
      vEst_ += p_.vLearnAlpha * (vTrue - vEst_);
      vEst_ = clampf(vEst_, 0.5f * p_.vBaseCmS, 2.0f * p_.vBaseCmS);
      if (p_.adaptSpeed) {
        float ns = clampf(pwmScale_ * sqrtf(p_.vBaseCmS / vEst_), p_.adaptMin, p_.adaptMax);
        vEst_ *= ns / pwmScale_;
        pwmScale_ = ns;
      }
    }
    NodeShape exp = expectedShape(node_, heading_);
    if (exp.left != shape.left || exp.right != shape.right) {
      stats_.mismatches++;
      snprintf(buf, sizeof buf, "LECH BAN DO tai %c: do L%dR%d, ky vong L%dR%d", nodeName(node_), shape.left, shape.right, exp.left, exp.right);
      hal_.log(buf);
      if (stats_.mismatches >= p_.mismatchToFallback) {
        nav_ = NavMode::Fallback;
        stats_.fallback = true;
        hal_.log("-> CHE DO DU PHONG (bam phai)");
      }
    }
  }

  if (nav_ == NavMode::Map) {
    int8_t e = currentEdge();
    if (e >= 0 && e < kNumEdges) stats_.covered |= (uint16_t)(1u << e);
    Act a = planIdx_ < planLen_ ? plan_[planIdx_++] : Act::Stop;
    snprintf(buf, sizeof buf, "NUT %c huong %d -> act %d (v=%.1f k=%.2f)", nodeName(node_), (int)heading_, (int)a, (double)vEst_, (double)pwmScale_);
    hal_.log(buf);
    if (a == Act::Stop) { persist(); finish(now); return; }
    startTurn(now, a, false);
  } else {
    int8_t e = currentEdge();
    if (e >= 0 && e < kNumEdges) stats_.covered |= (uint16_t)(1u << e);
    startTurn(now, fallbackChoice(shape), false);
  }
  persist();
}

Act Robot::fallbackChoice(const NodeShape& s) {
  Act a;
  if (s.branches() == 1) {
    a = s.left ? Act::Left : (s.right ? Act::Right : Act::Straight);
  } else {
    bool banR = sameTurn_ >= 2 && lastTurn_ == Act::Right;
    bool banL = sameTurn_ >= 2 && lastTurn_ == Act::Left;
    if (s.right && !banR) a = Act::Right;
    else if (s.straight) a = Act::Straight;
    else if (s.left && !banL) a = Act::Left;
    else a = s.right ? Act::Right : Act::Left;
  }
  if (a != Act::Straight && a == lastTurn_) sameTurn_++;
  else sameTurn_ = a == Act::Straight ? 0 : 1;
  if (a != Act::Straight) lastTurn_ = a;
  return a;
}

// ---------------------------------------------------------------- quay
void Robot::startTurn(uint32_t, Act a, bool deadEnd) {
  if (deadEnd) {
    heading_ = applyAct(heading_, Act::UTurn);
    uint8_t t = lastNode_; lastNode_ = node_; node_ = t;
    edgeLen_ = dist_ > 40 ? dist_ : 40;
    dist_ = 0;
    learnOk_ = false;
  } else {
    heading_ = applyAct(heading_, a);
    int8_t nb = neighbor(node_, heading_);
    float crawled = dist_ - learnDist_;  // quãng đã bò kể từ lúc phát hiện nút
    lastNode_ = node_;
    node_ = nb >= 0 ? (uint8_t)nb : node_;
    edgeLen_ = (nb >= 0 && nb < kNumNodes) ? p_.edgeLenCm : 40;
    // đi thẳng: trục bánh chưa tới tâm nút; quay tại chỗ: trục coi như ở tâm cung mới
    dist_ = (a == Act::Straight) ? crawled - p_.sensorAheadCm : 0;
    if (dist_ > 0) dist_ = 0;
    learnOk_ = nb >= 0 && nb < kNumNodes;
  }
  turnAct_ = a;
  turnPhase_ = 0;
  lineFound_.reset();
  pid_.reset();
  lastPos_ = 0;
  if (a == Act::Straight) {
    lastCtrl_ = hal_.millis();
    enter(State::Follow);
  } else {
    enter(State::Turn);
  }
}

void Robot::doTurn(uint32_t now, uint8_t bits) {
  int8_t dir = (turnAct_ == Act::Left) ? -1 : 1;  // -1: quay trái (bánh trái lùi)
  uint32_t t = now - stateAt_;
  uint16_t minMs = turnAct_ == Act::UTurn ? p_.uturnMinMs : p_.turnMinMs;
  if (t > p_.turnTimeoutMs) { hal_.log("quay qua lau -> tim line"); enter(State::Recover); return; }
  switch (turnPhase_) {
    case 0:  // quay nhanh trong thời gian tối thiểu: rời line cũ, bỏ qua vùng giao cắt
      drive((int16_t)(dir * p_.turnSpeed), (int16_t)(-dir * p_.turnSpeed));
      if (t >= minMs) { turnPhase_ = 1; lineFound_.reset(); }
      break;
    case 1: {  // quay chậm tìm line mới (cần 2 mẫu liên tiếp)
      drive((int16_t)(dir * p_.turnSlowSpeed), (int16_t)(-dir * p_.turnSlowSpeed));
      uint8_t want = kM | (dir < 0 ? kL1 : kR1);
      if (lineFound_.feed((bits & want) != 0)) {
        drive((int16_t)(-dir * p_.brakePwm), (int16_t)(dir * p_.brakePwm));  // phanh ngược
        turnPhase_ = 2;
        phaseAt_ = now;
      }
      break;
    }
    default:
      if (now - phaseAt_ >= p_.brakeMs) {
        lastCtrl_ = now;
        pid_.reset();
        enter(State::Follow);
      }
  }
}

void Robot::doRecover(uint32_t now, uint8_t bits) {
  uint32_t t = now - stateAt_;
  if (lineFound_.feed((bits & (kL1 | kM | kR1)) != 0)) { pid_.reset(); lastCtrl_ = now; enter(State::Follow); return; }
  if (t > p_.recoverTimeoutMs) {
    // không tìm lại được line: coi như ngõ cụt -> quay đầu; định vị không còn tin cậy
    if (nav_ == NavMode::Map) { nav_ = NavMode::Fallback; stats_.fallback = true; hal_.log("NGO CUT ngoai du kien -> du phong"); }
    startTurn(now, Act::UTurn, true);
    return;
  }
  // quét trái/phải với biên độ tăng dần: 300 ms trái, 600 phải, 900 trái, ...
  uint32_t acc = 0; uint8_t k = 0;
  while (acc + 300u * (k + 1) <= t) { acc += 300u * (k + 1); ++k; }
  int8_t dir = (k % 2 == 0) ? -1 : 1;
  if (lastPos_ > 0) dir = (int8_t)-dir;  // line mất về phía phải -> quét phải trước
  drive((int16_t)(dir * p_.turnSlowSpeed), (int16_t)(-dir * p_.turnSlowSpeed));
}

// ---------------------------------------------------------------- vật thể
int8_t Robot::currentEdge() const {
  if (lastNode_ == kStart && node_ == kH) return kStartStubEdge;
  if (lastNode_ >= kNumNodes || node_ >= kNumNodes) return -1;
  return edgeOf(lastNode_, node_);
}

float Robot::sFromLow() const {
  if (lastNode_ == kStart) return clampf(dist_, 0, 255);
  float s = lastNode_ < node_ ? dist_ : edgeLen_ - dist_;
  return clampf(s, 0, edgeLen_);
}

void Robot::doScanObject(uint32_t now, float dt) {
  // Quét trượt: dừng ngắn, rồi bò chậm qua vật trong khi lấy mẫu màu.
  if (now - stateAt_ < p_.settleMs) drive(0, 0);
  else drive(p_.creepPwm, p_.creepPwm);
  advance(cmdL_, cmdR_, dt);
  scanLowRun_ = hal_.obstacle(scanSide_) ? 0 : (uint8_t)(scanLowRun_ < 255 ? scanLowRun_ + 1 : 255);
  if (now >= nextSampleAt_ && samples_ < 255) {
    RawColor c;
    if (hal_.readColor(scanSide_, c)) {
      vote_.add(isRedSample(sensorOf(scanSide_), c, p_, cal_));
    } else {
      vote_.add(false);
      ++sampleErrors_;
      ++stats_.sensorErrors;
    }
    ++samples_;
    nextSampleAt_ = now + p_.colorGapMs;
  }
  bool enough = vote_.reds() >= p_.colorNeed;
  bool passed = odo_ - scanStartOdo_ >= p_.creepMaxCm || (scanLowRun_ >= 3 && samples_ >= 3);
  if (!enough && !passed) return;
  drive(0, 0);

  char buf[72];
  snprintf(buf, sizeof buf, "quet %s: %u/%u mau do, c%d s=%.0f", scanSide_ == Side::Left ? "T" : "P", vote_.reds(), samples_, currentEdge() + 1, (double)sFromLow());
  hal_.log(buf);
  if (sampleErrors_ == samples_ && samples_ > 0) {
    snprintf(buf, sizeof buf, "LOI cam bien mau %s", scanSide_ == Side::Left ? "TRAI" : "PHAI");
    hal_.log(buf);
  }
  if (enough) {
    int8_t e = currentEdge();
    Side es = edgeSide(scanSide_, lastNode_ < node_ || lastNode_ == kStart);
    bool fresh = (e < 0 || nav_ == NavMode::Fallback) ? true : tracker_.isNew((uint8_t)e, es, sFromLow());
    if (fresh) {
      if (e >= 0 && nav_ == NavMode::Map) tracker_.mark((uint8_t)e, es, sFromLow());
      stats_.alerts++;
      alert_.start(now, p_.alertMs);
      hal_.buzzer(true);
      hal_.led(true);
      snprintf(buf, sizeof buf, "DO! cung c%d ben %s s=%.0f (tong %u)", e + 1, scanSide_ == Side::Left ? "T" : "P", (double)sFromLow(), stats_.alerts);
      hal_.log(buf);
      persist();
      enter(State::AlertOn);
      return;
    }
    hal_.log("do nhung DA BAO -> bo qua");
  }
  lastCtrl_ = now;
  enter(State::Follow);
}

void Robot::doAlert(uint32_t now) {
  drive(0, 0);
  if (alert_.update(now)) return;
  hal_.buzzer(false);
  hal_.led(false);
  lastCtrl_ = now;
  if (p_.enableGripper) { gripStep_ = 0; enter(State::Grip); }
  else enter(State::Follow);
}

void Robot::doGrip(uint32_t now) {
  lastCtrl_ = now;
  enter(State::Follow);
}

// ---------------------------------------------------------------- kết thúc
void Robot::finish(uint32_t now) {
  drive(0, 0);
  hal_.buzzer(false);
  stats_.finishMs = now - matchStart_;
  enter(State::Goal);
  hal_.led(true);
  char buf[64];
  snprintf(buf, sizeof buf, "KET THUC: %lu ms, bao %u vat", (unsigned long)stats_.finishMs, stats_.alerts);
  hal_.log(buf);
}

void Robot::persist() {
  uint16_t regs[kBackupRegs];
  encodeBackup(regs, stats_.covered, tracker_, stats_.restarts);
  hal_.saveBackup(regs, kBackupRegs);
}

// ---------------------------------------------------------------- hiệu chỉnh màu
// 8 bước: lớp (Đỏ, Vàng, Xanh, Tím) x bên (Trái trước, Phải sau).
// LED nháy (số lớp + 1) lần mỗi chu kỳ; đưa mẫu vào bên tương ứng cho tới khi bíp, rồi rút ra.
void Robot::doCalib(uint32_t now) {
  hal_.setMotors(0, 0);
  if (calibStep_ >= 2 * kNumClasses) return;
  uint8_t cls = calibStep_ / 2;
  Side side = (calibStep_ % 2) ? Side::Right : Side::Left;
  uint32_t ph = (now - stateAt_) % 2000;
  if (!calibWaitClear_ && calibCount_ == 0) hal_.led(ph < (uint32_t)(cls + 1) * 300 && (ph % 300) < 150);

  bool obs = hal_.obstacle(side);
  if (calibWaitClear_) {
    if (now - stateAt_ >= 150) hal_.buzzer(false);
    if (!obs && now - stateAt_ >= 150) { calibWaitClear_ = false; hal_.buzzer(false); }
    return;
  }
  if (!obs) { calibCount_ = 0; return; }
  if (now < nextSampleAt_) return;
  if (calibCount_ == 0) { cal_.count[(uint8_t)sensorOf(side)][cls] = 0; cal_.cent[(uint8_t)sensorOf(side)][cls][0] = 0; cal_.cent[(uint8_t)sensorOf(side)][cls][1] = 0; }
  RawColor c;
  if (hal_.readColor(side, c)) { calibAdd(cal_, sensorOf(side), cls, c); ++calibCount_; }
  nextSampleAt_ = now + p_.colorGapMs;
  if (calibCount_ >= p_.calibSamples) {
    hal_.buzzer(true);  // bíp ngắn: xong bước này
    stateAt_ = now;
    calibCount_ = 0;
    calibWaitClear_ = true;
    ++calibStep_;
    char buf[48];
    snprintf(buf, sizeof buf, "CALIB lop %u ben %s xong", cls, side == Side::Left ? "TRAI" : "PHAI");
    hal_.log(buf);
    if (calibStep_ >= 2 * kNumClasses) {
      cal_.magic = Calib::kMagic;
      hal_.saveCalib(cal_);
      hal_.buzzer(false);
      hal_.log("CALIB: da luu Flash. Nhan RESET de chay.");
      enter(State::Goal);
      hal_.led(true);
    }
  }
}

}  // namespace core
