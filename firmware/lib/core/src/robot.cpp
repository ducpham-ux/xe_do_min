#include "robot.h"
#include <math.h>
#include <stdio.h>

namespace core {

namespace {
inline int16_t clampPwm(float v, int16_t lim) {
  if (v > lim) return lim;
  if (v < -lim) return (int16_t)-lim;
  return (int16_t)v;
}
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float absf(float v) { return v < 0 ? -v : v; }
constexpr float kDeg = 3.14159265f / 180.0f;
// Nút trên trục dọc giữa có loa bo góc ở cả 4 góc phần tư (theo sơ đồ thể lệ).
inline bool isFlared(uint8_t n) { return n == kB || n == kE || n == kH; }

// các pha tiếp cận vật
enum : uint8_t { kApSettle = 0, kApCheck, kApStep, kApRead, kApReturn, kApMerge, kApRejoin };
}  // namespace

Robot::Robot(Hal& hal, const Params& p)
    : hal_(hal), p_(p), nd_(p), pid_(p.kp, p.ki, p.kd, p.iLimit, p.dTauS), tracker_(p), gripper_(p) {
  vEst_ = p.vBaseCmS;
  turnRate_ = p.turnRateDegS;
}

void Robot::enter(State s) {
  if (s == State::Recover || s == State::Seek) lineFound_.reset();
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
  if (p_.enableGripper) gripper_.park(hal_);

  if (mode == BootMode::Calib) {
    cal_ = Calib();
    calibStep_ = 0; calibCount_ = 0; calibWaitClear_ = false;
    enter(State::Calib);
    hal_.log("CALIB: dua mau DO sat TCS3200 (suon trai)");
    return;
  }
  if (mode == BootMode::ClearMemory) {
    uint16_t zero[kBackupRegs] = {};
    hal_.saveBackup(zero, kBackupRegs);
  }

  uint16_t regs[kBackupRegs];
  uint16_t covered = 0, elapsedS = 0; uint8_t restarts = 0;
  bool restored = mode == BootMode::Run && hal_.loadBackup(regs, kBackupRegs) &&
                  decodeBackup(regs, covered, tracker_, restarts, &elapsedS);
  priorElapsedMs_ = restored ? elapsedS * 1000u : 0;
  if (restored) {
    stats_.restarts = (uint8_t)(restarts + 1);
    stats_.covered = covered;
    stats_.alerts = tracker_.countOf(ObjClass::Red);
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
  loc_.reset(kH, Dir::N);
  edgeLen_ = p_.startStubCm; dist_ = 0;
  pendingDir_ = 0; pass_ = 0;
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
    if (now - lastPersist_ >= 5000) { lastPersist_ = now; persist(); }
    if (el >= p_.matchMs) { hal_.log("HET GIO"); finish(now); return; }
    if (el >= p_.goHomeMs && nav_ == NavMode::Map && !stats_.wentHome && state_ == State::Follow) {
      uint8_t n = planPath(node_, heading_, kA, plan_, sizeof(plan_) / sizeof(plan_[0]));
      if (n) {
        planLen_ = n; planIdx_ = 0; stats_.wentHome = true;
        pendingDir_ = 0; replan_ = fullReplan_ = false;
        hal_.log("GAN HET GIO -> VE DICH");
      }
    }
  }

  switch (state_) {
    case State::Boot: break;
    case State::Calib: doCalib(now); break;
    case State::Ready: doReady(now); break;
    case State::Follow: doFollow(now, bits, dt); break;
    case State::Seek: doSeek(now, bits); break;
    case State::NodeScan: doNodeScan(now, bits, dt); break;
    case State::Turn: doTurn(now, bits); break;
    case State::Approach: doApproach(now, bits, dt); if (state_ == State::Approach && apPhase_ == kApRejoin) {
        LineScan sc = scanLine(bits, p_, 0);
        postReadOdo_ = odo_;
        if (sc.seen) { lastPos_ = sc.pos; resumeFollow(now); }
        else { lastPos_ = 2.0f; enter(State::Recover); }  // xe thường lệch trái line -> tìm bên phải trước
      }
      break;
    case State::AlertOn: doAlert(now); break;
    case State::Grip: doGrip(now); break;
    case State::Recover: doRecover(now, bits); break;
    case State::Goal: doGoal(now); break;
  }
}

void Robot::doReady(uint32_t now) {
  hal_.setMotors(0, 0);
  hal_.led(((now - stateAt_) / 250) % 2 == 0);
  if (now - stateAt_ >= p_.startDelayMs) {
    hal_.led(false);
    matchStart_ = now - priorElapsedMs_;  // đồng hồ trọng tài không dừng khi restart
    lastNodeAt_ = now;
    resumeFollow(now);
    hal_.log("START");
  }
}

void Robot::resumeFollow(uint32_t now) {
  pid_.reset();
  lastCtrl_ = now;
  vCmd_ = p_.minFollowPwm;
  lost_ = false;
  enter(State::Follow);
}

// ---------------------------------------------------------------- động cơ
void Robot::drive(int16_t l, int16_t r) {
  cmdL_ = l; cmdR_ = r;
  hal_.setMotors(clampPwm(l * pwmScale_, 255), clampPwm(r * pwmScale_, 255));
}

float Robot::speedAt(int16_t pwm) const {
  float eff = (float)pwm - p_.pwmDeadband;
  if (eff <= 0) return 0;
  return vEst_ * eff / (p_.baseSpeed - p_.pwmDeadband);
}

void Robot::advance(int16_t l, int16_t r, float dt) {
  // vận tốc ~ (PWM - vùng chết); hai bánh ngược chiều (quay tại chỗ) không tiến
  float ds = (speedAt(l) + speedAt(r)) * 0.5f * dt;
  if (l < 0 && r < 0) ds = -(speedAt((int16_t)-l) + speedAt((int16_t)-r)) * 0.5f * dt;
  dist_ += ds;
  odo_ += ds;
}

// ---------------------------------------------------------------- vật cản
bool Robot::obstacleEdge(Side s) {
  uint8_t i = (uint8_t)s;
  bool on = hal_.obstacle(s);
  if (on) {
    if (obsRun_[i] < 255) ++obsRun_[i];
  } else {
    obsRun_[i] = 0;
    obsArmed_[i] = true;  // đã thấy LOW -> vật tiếp theo là vật mới
  }
  float since = odo_ - distAtTrig_[i];
  if (!obsArmed_[i] && since > 25.0f) obsArmed_[i] = true;  // E18 kẹt HIGH quá lâu
  float rearm = s == Side::Left ? p_.obstacleRearmCm : p_.rightRearmCm;
  if (obsRun_[i] >= p_.obstacleDebounce && obsArmed_[i] && since >= rearm) {
    obsArmed_[i] = false;
    distAtTrig_[i] = odo_;
    return true;
  }
  return false;
}

// Vật bên PHẢI: không dừng (TCS3200 ở sườn trái), chỉ ghi nhớ và đặt nhu cầu đi ngược cung này.
void Robot::onRightObject() {
  if (!p_.readRightObjects || nav_ != NavMode::Map || stats_.wentHome) return;
  int8_t e = currentEdge();
  if (e < 0 || e >= kNumEdges || lastNode_ >= kNumNodes) return;
  float s = sFromLow();
  Side es = edgeSide(Side::Right, lowToHigh());
  if (tracker_.find((uint8_t)e, es, s, pass_) >= 0) return;  // đã biết từ lượt trước
  tracker_.mark((uint8_t)e, es, s, ObjClass::Pending, pass_);
  stats_.pendingSeen++;
  uint8_t di = dirIndex(node_, lastNode_);
  char buf[64];
  snprintf(buf, sizeof buf, "vat PHAI c%d s=%.0f -> doc sau (di nguoc %c->%c)", e + 1, (double)s, nodeName(node_), nodeName(lastNode_));
  hal_.log(buf);
  if (di == 255 || (pendingDir_ & (1ul << di))) return;
  pendingDir_ |= 1ul << di;
  if (!planTraverses(node_, heading_, plan_ + planIdx_, (uint8_t)(planLen_ - planIdx_), node_, lastNode_)) replan_ = true;
}

// ---------------------------------------------------------------- bám line
void Robot::doFollow(uint32_t now, uint8_t bits, float dt) {
  bool trigL = obstacleEdge(Side::Left);
  bool trigR = obstacleEdge(Side::Right);
  if (trigR) onRightObject();
  if (trigL && odo_ - postReadOdo_ >= p_.postReadCm && startApproach(now)) return;

  LineScan sc = scanLine(bits, p_, lastPos_);
  if (!sc.seen) {
    // góc chữ L / ngã ba không có lối thẳng: hết vạch ngang là toàn trắng -> vẫn phải báo bộ dò nút
    bool gateNow = nav_ == NavMode::Map ? dist_ >= p_.nodeGateFrac * edgeLen_ : odo_ - lastNodeOdo_ >= p_.fallbackGateCm;
    if (gateNow && nd_.inCross()) {
      float before = odo_;
      advance(cmdL_, cmdR_, dt);
      if (nd_.feed(sc, odo_ - before)) {
        nodeShape_ = nd_.shape();
        nodePhase_ = 0; nodeOdo_ = odo_; centerRun_ = 0; learnDist_ = dist_;
        lost_ = false;
        enter(State::NodeScan);
      }
      return;
    }
    if (!lost_) { lost_ = true; lostSince_ = now; }
    uint32_t lostMs = now - lostSince_;
    // line trôi ra mép thanh (>= 3 mẫu liên tiếp, bỏ nhiễu 1 mẫu): quay ngay về phía đó
    // (thanh cách tâm quay 12 cm -> quay vài độ là thấy lại)
    if (lostMs >= 3 && absf(lastPos_) >= p_.seekMinPos && !nd_.inCross()) {
      stats_.lineLost++;
      seekDir_ = lastPos_ > 0 ? 1 : -1;
      enter(State::Seek);
      return;
    }
    if (lostMs >= p_.lostConfirmMs) {
      stats_.lineLost++;
      lost_ = false;
      if (nav_ == NavMode::Fallback) {
        if (now - matchStart_ < p_.fallbackMinExploreMs) startTurn(now, Act::UTurn, true);
        else finish(now);
      } else {
        enter(State::Recover);
      }
      return;
    }
    advance(cmdL_, cmdR_, dt);  // giữ nguyên lệnh trong lúc chờ xác nhận
    return;
  }
  lost_ = false;

  // tốc độ: giảm khi gần nút / thấy cung bo / line lệch xa tâm; tăng mềm
  bool gate = nav_ == NavMode::Map ? dist_ >= p_.nodeGateFrac * edgeLen_ : odo_ - lastNodeOdo_ >= p_.fallbackGateCm;
  float target = p_.baseSpeed;
  if ((nav_ == NavMode::Map && dist_ > p_.approachFrac * edgeLen_) || (gate && nd_.flareSeen())) target = p_.approachSpeed;
  float e = clampf(absf(sc.pos) / 3.5f, 0, 1);
  target *= 1.0f - p_.slowOnErr * e;
  if (target < p_.minFollowPwm) target = p_.minFollowPwm;
  if (vCmd_ < target) { vCmd_ += p_.accelPwmPerS * dt; if (vCmd_ > target) vCmd_ = target; }
  else vCmd_ = target;

  // Vùng cung bo (gần B, E, H): cung bo nhập/tách khỏi line làm dải đen phình hoặc tách thành
  // nhiều dải -> không đoán được line. Chỉ lái khi mẫu "sạch" (1 dải, <= 2 mắt); mẫu bẩn thì
  // giữ thẳng hướng hiện tại (đóng băng PID) cho tới khi qua vùng.
  bool flareZone = nav_ == NavMode::Map &&
                   ((isFlared(node_) && dist_ > edgeLen_ - 45.0f) || (isFlared(lastNode_) && dist_ < 32.0f));
  bool clean = sc.nRuns == 1 && sc.mainLen() <= 2;
  if (now - lastCtrl_ >= p_.ctrlMs) {
    float dtc = (now - lastCtrl_) / 1000.0f;
    lastCtrl_ = now;
    if (!flareZone || clean) {
      lastPos_ = sc.pos;
      float u = pid_.update(sc.pos, dtc);
      drive(clampPwm(vCmd_ + u, p_.maxPwm), clampPwm(vCmd_ - u, p_.maxPwm));
    } else {
      drive((int16_t)vCmd_, (int16_t)vCmd_);
    }
  }
  float before = odo_;
  advance(cmdL_, cmdR_, dt);

  // nút giao. Ở nút có cung bo phải thấy cung bo TÁCH khỏi line trước, trừ khi đã đi quá vị trí
  // dự kiến của vạch ngang 4 cm (an toàn khi không thấy mẫu tách).
  if (gate) {
    float crossAt = edgeLen_ - (p_.sensorAheadCm - p_.lineWidthCm * 0.5f);
    nd_.setRequireSplit(nav_ == NavMode::Map && isFlared(node_) && dist_ < crossAt + 4.0f);
    if (nd_.feed(sc, odo_ - before)) {
      nodeShape_ = nd_.shape();
      nodePhase_ = 0;
      nodeOdo_ = odo_;
      centerRun_ = 0;
      learnDist_ = dist_;
      enter(State::NodeScan);
    }
  } else {
    nd_.reset();
  }
}

void Robot::doSeek(uint32_t now, uint8_t bits) {
  LineScan sc = scanLine(bits, p_, seekDir_ * 3.5f);
  bool center = sc.seen && sc.mainHi >= 1 && sc.mainLo <= 3;
  if (lineFound_.feed(center)) { lastPos_ = sc.pos; resumeFollow(now); return; }
  if (now - stateAt_ > p_.seekMaxMs) {
    lastPos_ = seekDir_ * p_.lostPos;
    enter(State::Recover);
    return;
  }
  drive((int16_t)(seekDir_ * p_.seekPwm), (int16_t)(-seekDir_ * p_.seekPwm));
}

// ---------------------------------------------------------------- nút giao
// Pha 0: vừa qua hết vạch ngang -> bò thêm straightCheckCm, xét còn lối thẳng, chọn hành động.
// Pha 1: (rẽ) bò tiếp cho TÂM QUAY vào giữa nút rồi mới quay tại chỗ.
void Robot::doNodeScan(uint32_t now, uint8_t bits, float dt) {
  LineScan sc = scanLine(bits, p_, lastPos_);
  bool center = sc.seen && sc.mainLen() <= 2 && sc.mainLo >= 1 && sc.mainHi <= 3;
  float u = center ? p_.kp * 0.5f * sc.pos : 0.0f;
  if (center) lastPos_ = sc.pos;
  drive(clampPwm(p_.scanSpeed + u, p_.maxPwm), clampPwm(p_.scanSpeed - u, p_.maxPwm));
  advance(cmdL_, cmdR_, dt);
  float traveled = odo_ - nodeOdo_;

  if (nodePhase_ == 0) {
    centerRun_ = center ? (uint8_t)(centerRun_ < 255 ? centerRun_ + 1 : 255) : 0;
    if (centerRun_ >= 3 && traveled >= 0.3f) nodeShape_.straight = true;
    if (traveled < p_.straightCheckCm && now - stateAt_ < 1500) return;
    Act a = onNode(now, nodeShape_);
    if (state_ == State::Goal) return;
    if (a == Act::Straight) { startTurn(now, a, false); return; }
    nodePhase_ = 1;
    pendingAct_ = a;
    crawlTarget_ = p_.sensorAheadCm - p_.lineWidthCm * 0.5f + p_.crawlExtraCm;
    return;
  }
  if (traveled >= crawlTarget_ || now - stateAt_ > 3000) startTurn(now, pendingAct_, false);
}

Act Robot::onNode(uint32_t now, const NodeShape& shape) {
  stats_.nodesSeen++;
  lastNodeAt_ = now;
  lastNodeOdo_ = odo_;
  char buf[96];

  if (nav_ == NavMode::Map) {
    // học vận tốc thật từ chiều dài cung đã biết (mép sau vạch ngang = tâm nút + nửa bề rộng line)
    if (learnOk_ && edgeLen_ >= p_.edgeLenCm - 1 && learnDist_ > 10) {
      float real = p_.edgeLenCm - (p_.sensorAheadCm - p_.lineWidthCm * 0.5f);
      float vTrue = vEst_ * real / learnDist_;
      vEst_ += p_.vLearnAlpha * (vTrue - vEst_);
      vEst_ = clampf(vEst_, 0.5f * p_.vBaseCmS, 2.0f * p_.vBaseCmS);
      if (p_.adaptSpeed) {
        float ns = clampf(pwmScale_ * sqrtf(p_.vBaseCmS / vEst_), p_.adaptMin, p_.adaptMax);
        vEst_ *= ns / pwmScale_;
        pwmScale_ = ns;
      }
    }
    localize(shape);
  }
  if (nav_ == NavMode::Fallback) {
    int8_t e = currentEdge();
    if (e >= 0 && e < kNumEdges) stats_.covered |= (uint16_t)(1u << e);
    Act a = fallbackChoice(shape);
    persist();
    return a;
  }

  int8_t e = currentEdge();
  if (e >= 0 && e < kNumEdges) stats_.covered |= (uint16_t)(1u << e);
  uint8_t di = dirIndex(lastNode_, node_);
  if (di != 255) pendingDir_ &= ~(1ul << di);  // đã đi cung này theo chiều này
  if (replan_ || fullReplan_) replan(true);
  Act a = planIdx_ < planLen_ ? plan_[planIdx_++] : Act::Stop;
  snprintf(buf, sizeof buf, "NUT %c huong %d hinh L%dR%dS%d -> act %d (d=%.0f v=%.1f q=%.0f)", nodeName(node_), (int)heading_,
           shape.left, shape.right, shape.straight, (int)a, (double)learnDist_, (double)vEst_, (double)turnRate_);
  hal_.log(buf);
  if (a == Act::Stop) { persist(); finish(now); return a; }
  loc_.act(a);
  persist();
  return a;
}

// Định vị đa giả thuyết: lọc tập (nút, hướng) bằng hình dạng nút vừa đo.
void Robot::localize(const NodeShape& shape) {
  NodeShape exp = expectedShape(node_, heading_);
  bool match = exp.left == shape.left && exp.right == shape.right;
  char buf[80];
  if (!match) {
    stats_.mismatches++;
    snprintf(buf, sizeof buf, "LECH BAN DO tai %c: do L%dR%d, ky vong L%dR%d", nodeName(node_), shape.left, shape.right, exp.left, exp.right);
    hal_.log(buf);
  }
  uint8_t n = loc_.observe(shape);
  if (n == 0) {
    // không giả thuyết nào khớp: lần đầu coi là đo nhiễu, lần thứ hai dựng lại tập giả thuyết
    if (++noisyObs_ < 2) return;
    noisyObs_ = 0;
    loc_.resetAll(shape);
    if (++ambNodes_ > p_.relocMaxNodes) {
      nav_ = NavMode::Fallback;
      stats_.fallback = true;
      hal_.log("-> CHE DO DU PHONG (bam phai)");
      return;
    }
    hal_.log("MAT DINH VI -> dung lai tap gia thuyet");
  } else {
    noisyObs_ = 0;
  }
  if (loc_.contains(node_, heading_)) return;
  uint8_t nn = node_; Dir nh = heading_;
  if (!loc_.nearest(node_, heading_, nn, nh)) return;
  int8_t back = neighbor(nn, applyAct(nh, Act::UTurn));
  snprintf(buf, sizeof buf, "TU DINH VI LAI: %c huong %d -> %c huong %d (%u gia thuyet)", nodeName(node_), (int)heading_, nodeName(nn), (int)nh, loc_.count());
  hal_.log(buf);
  node_ = nn; heading_ = nh;
  lastNode_ = back >= 0 ? (uint8_t)back : kStart;
  learnOk_ = false;
  fullReplan_ = true;
  stats_.relocalized++;
}

// Lập lại lộ trình. full: tham lam toàn bộ (sau khi tự định vị lại). Ngược lại giữ phần còn lại của
// lộ trình phủ tối ưu và nối thêm các cung cần ĐI NGƯỢC để đọc vật bên phải.
void Robot::replan(bool full) {
  Act tmp[sizeof(plan_) / sizeof(plan_[0])];
  const uint8_t cap = sizeof(tmp) / sizeof(tmp[0]);
  uint8_t n = 0;
  if (full) {
    n = planGreedy(node_, heading_, (uint16_t)(~stats_.covered & kAllEdges), pendingDir_, kA, tmp, cap);
  } else {
    uint8_t r = 0, nd = node_;
    Dir hd = heading_;
    uint32_t need = pendingDir_;
    for (uint8_t i = planIdx_; i < planLen_ && plan_[i] != Act::Stop && r < cap; ++i) {
      tmp[r++] = plan_[i];
      hd = applyAct(hd, plan_[i]);
      int8_t nb = neighbor(nd, hd);
      if (nb < 0 || nb >= kNumNodes) break;
      uint8_t di = dirIndex(nd, (uint8_t)nb);
      if (di != 255) need &= ~(1ul << di);
      nd = (uint8_t)nb;
    }
    uint8_t tail = planGreedy(nd, hd, 0, need, kA, tmp + r, (uint8_t)(cap - r));
    n = tail ? (uint8_t)(r + tail) : 0;
  }
  replan_ = fullReplan_ = false;
  if (n == 0) { hal_.log("khong lap duoc lo trinh moi -> giu lo trinh cu"); return; }
  for (uint8_t i = 0; i < n; ++i) plan_[i] = tmp[i];
  planLen_ = n;
  planIdx_ = 0;
  char buf[48];
  snprintf(buf, sizeof buf, "LAP LAI LO TRINH: %u buoc", n);
  hal_.log(buf);
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
void Robot::startTurn(uint32_t now, Act a, bool deadEnd) {
  if (deadEnd) {
    heading_ = applyAct(heading_, Act::UTurn);
    uint8_t t = lastNode_; lastNode_ = node_; node_ = t;
    edgeLen_ = dist_ > 40 ? dist_ : 40;
    dist_ = 0;
    learnOk_ = false;
  } else {
    heading_ = applyAct(heading_, a);
    int8_t nb = neighbor(node_, heading_);
    lastNode_ = node_;
    node_ = nb >= 0 ? (uint8_t)nb : node_;
    edgeLen_ = (nb >= 0 && nb < kNumNodes) ? p_.edgeLenCm : 40;
    // đi thẳng: tâm quay còn ở TRƯỚC nút; quay tại chỗ: tâm quay ở tâm nút
    dist_ = (a == Act::Straight) ? (odo_ - nodeOdo_) + p_.lineWidthCm * 0.5f - p_.sensorAheadCm : 0;
    learnOk_ = nb >= 0 && nb < kNumNodes;
  }
  if (++pass_ == kRestoredPass) pass_ = 0;
  turnAct_ = a;
  turnPhase_ = 0;
  lineFound_.reset();
  pid_.reset();
  nd_.reset();
  lastPos_ = 0;
  if (a == Act::Straight) {
    lastCtrl_ = now;
    vCmd_ = p_.scanSpeed;
    lost_ = false;
    enter(State::Follow);
  } else {
    enter(State::Turn);
  }
}

// Quay tại chỗ: pha 0 quay nhanh tối thiểu turnMinDeg (bỏ qua cung bo 45°), pha 1 quay chậm tới
// khi M nằm giữa một dải HẸP (line thật vuông góc thanh), pha 2 phanh ngược. Tự học tốc độ quay.
void Robot::doTurn(uint32_t now, uint8_t bits) {
  int8_t dir = (turnAct_ == Act::Left) ? -1 : 1;  // -1: quay trái (bánh trái lùi)
  bool uturn = turnAct_ == Act::UTurn;
  float target = uturn ? 180.0f : 90.0f;
  float minDeg = uturn ? p_.uturnMinDeg : p_.turnMinDeg;
  float kS = (p_.turnSlowSpeed - p_.pwmDeadband) / (p_.turnSpeed - p_.pwmDeadband);
  uint32_t t = now - stateAt_;
  switch (turnPhase_) {
    case 0:
      fastMs_ = (uint32_t)(minDeg / turnRate_ * 1000.0f);
      drive((int16_t)(dir * p_.turnSpeed), (int16_t)(-dir * p_.turnSpeed));
      if (t >= fastMs_) { turnPhase_ = 1; slowStartAt_ = now; lineFound_.reset(); }
      break;
    case 1: {
      drive((int16_t)(dir * p_.turnSlowSpeed), (int16_t)(-dir * p_.turnSlowSpeed));
      float angle = minDeg + turnRate_ * kS * (now - slowStartAt_) / 1000.0f;
      if (angle > target + p_.turnMaxExtraDeg) {
        hal_.log("quay qua goc ma chua thay line -> tim line");
        lastPos_ = (float)dir * p_.lostPos;
        enter(State::Recover);
        return;
      }
      LineScan sc = scanLine(bits, p_, 0);
      // dải chứa M, rộng <= 3 mắt (ở nút có cung bo, cung dính sát line khi cách tâm 12 cm)
      bool ok = sc.seen && sc.mainLen() <= 3 && sc.mainLo <= 2 && sc.mainHi >= 2;
      if (lineFound_.feed(ok)) {
        float tF = fastMs_ / 1000.0f, tS = (now - slowStartAt_) / 1000.0f;
        float r = clampf(target / (tF + kS * tS), 0.5f * p_.turnRateDegS, 2.0f * p_.turnRateDegS);
        turnRate_ += p_.turnLearnAlpha * (r - turnRate_);
        drive((int16_t)(-dir * p_.brakePwm), (int16_t)(dir * p_.brakePwm));  // phanh ngược
        turnPhase_ = 2;
        phaseAt_ = now;
      }
      break;
    }
    default:
      if (now - phaseAt_ >= p_.brakeMs) { lastPos_ = 0; resumeFollow(now); }
  }
}

void Robot::doRecover(uint32_t now, uint8_t bits) {
  uint32_t t = now - stateAt_;
  if (lineFound_.feed((bits & (kL1 | kM | kR1)) != 0)) {
    lastPos_ = scanLine(bits, p_, 0).pos;
    resumeFollow(now);
    return;
  }
  if (t > p_.recoverTimeoutMs) {
    // không tìm lại được line: coi như nhánh cụt -> quay đầu
    if (nav_ == NavMode::Map) {
      // bản đồ chỉ có 2 nhánh cụt (VÒNG 1 ở B, XUẤT PHÁT ở H) -> định vị lại theo đó
      loc_.deadEnd();
      uint8_t nn = node_; Dir nh = heading_;
      uint8_t from = lastNode_ < kNumNodes ? lastNode_ : node_;
      loc_.nearest(from, applyAct(heading_, Act::UTurn), nn, nh);
      startTurn(now, Act::UTurn, true);
      node_ = nn; heading_ = nh;
      lastNode_ = nn == kB ? kStub : kStart;
      edgeLen_ = 40; dist_ = 0;
      fullReplan_ = true;
      stats_.relocalized++;
      char buf[48];
      snprintf(buf, sizeof buf, "NGO CUT -> quay ve %c", nodeName(nn));
      hal_.log(buf);
    } else {
      startTurn(now, Act::UTurn, true);
    }
    return;
  }
  // quét trái/phải với biên độ tăng dần: 300 ms, 600, 900, ... bắt đầu về phía line mất
  uint32_t acc = 0; uint8_t k = 0;
  while (acc + 300u * (k + 1) <= t) { acc += 300u * (k + 1); ++k; }
  int8_t dir = (k % 2 == 0) ? -1 : 1;
  if (lastPos_ > 0) dir = (int8_t)-dir;
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

// E18 TRÁI kích: quyết định có dừng tiếp cận/đọc không.
bool Robot::startApproach(uint32_t now) {
  if (now - matchStart_ + 15000u > p_.matchMs) return false;  // không còn đủ thời gian
  int8_t e = nav_ == NavMode::Map ? currentEdge() : -1;
  float s = sFromLow();  // vị trí theo tâm xe (E18 coi như ngang tâm)
  Side es = edgeSide(Side::Left, lowToHigh());
  if (e >= 0) {
    int8_t k = tracker_.find((uint8_t)e, es, s, pass_);
    if (k >= 0 && tracker_.at((uint8_t)k).cls != ObjClass::Pending) {
      hal_.log("vat da doc o luot truoc -> bo qua");
      return false;
    }
  }
  apEdge_ = e; apEdgeSide_ = es; apS_ = s;
  drive(0, 0);
  apPhase_ = kApSettle; apSteps_ = 0; apChecks_ = 0; apPresent_ = 0;
  moveN_ = moveIdx_ = 0; movePhase_ = 0;
  samples_ = 0; vote_.reset(); apWasRed_ = false;
  stats_.objectsRead++;
  enter(State::Approach);
  // E18 kích ở mép vật -> tâm vật ở phía trước; TCS3200 có thể lệch dọc so với E18.
  float align = p_.alignCm - p_.tcsAheadOfE18Cm;
  if (absf(align) > 0.3f) { queueMove(false, align); apPhase_ = kApStep; }
  return true;
}

void Robot::queueMove(bool pivot, float amount) {
  if (moveN_ < sizeof(moves_) / sizeof(moves_[0])) moves_[moveN_++] = Move{pivot, amount};
}

// Chạy lần lượt các chuyển động mở trong hàng đợi: chạy -> phanh ngược ngắn -> dừng ổn định.
bool Robot::runMoves(uint32_t now) {
  if (moveIdx_ >= moveN_) { drive(0, 0); return true; }
  const Move& m = moves_[moveIdx_];
  switch (movePhase_) {
    case 0: {
      if (m.pivot) {
        int8_t dir = m.amount > 0 ? -1 : 1;  // + = quay trái
        moveDur_ = (uint32_t)(absf(m.amount) / turnRate_ * 1000.0f);
        drive((int16_t)(dir * p_.turnSpeed), (int16_t)(-dir * p_.turnSpeed));
      } else {
        float v = speedAt(p_.movePwm);
        moveDur_ = v > 1 ? (uint32_t)(absf(m.amount) / v * 1000.0f) : 0;
        int16_t s = m.amount > 0 ? p_.movePwm : (int16_t)-p_.movePwm;
        drive(s, s);
      }
      moveAt_ = now;
      movePhase_ = 1;
      return false;
    }
    case 1:
      if (now - moveAt_ < moveDur_) return false;
      drive(cmdL_ > 0 ? (int16_t)-p_.brakePwm : p_.brakePwm, cmdR_ > 0 ? (int16_t)-p_.brakePwm : p_.brakePwm);
      moveAt_ = now;
      movePhase_ = 2;
      return false;
    case 2:
      if (now - moveAt_ < p_.brakeMs) return false;
      drive(0, 0);
      moveAt_ = now;
      movePhase_ = 3;
      return false;
    default:
      if (now - moveAt_ < p_.moveSettleMs) return false;
      ++moveIdx_;
      movePhase_ = 0;
      return moveIdx_ >= moveN_;
  }
}

// Tiếp cận vật bên trái: kiểm tra TCS3200 "thấy" vật chưa; chưa thì dịch ngang 1 bước rồi kiểm tra
// lại (tối đa sideStepMax bước); đọc màu (đứng yên rồi bò chậm dọc vật); báo nếu đỏ; trở lại line.
void Robot::doApproach(uint32_t now, uint8_t bits, float dt) {
  switch (apPhase_) {
    case kApSettle:
      drive(0, 0);
      if (now - stateAt_ < p_.moveSettleMs) return;
      apPhase_ = kApCheck; apChecks_ = 0; apPresent_ = 0; apNear_ = 0; nextSampleAt_ = now;
      return;
    case kApCheck: {
      drive(0, 0);
      if (now < nextSampleAt_) return;
      RawColor c;
      bool ok = hal_.readColor(c);
      if (ok && colorPresent(c, p_)) ++apPresent_;
      if (ok && colorNear(c, p_)) ++apNear_;
      ++apChecks_;
      nextSampleAt_ = now + p_.colorGapMs;
      if (apNear_ < p_.presenceNeed && apChecks_ < 3) return;
      if (apNear_ >= p_.presenceNeed || apSteps_ >= p_.sideStepMax) {
        apPhase_ = kApRead; samples_ = 0; vote_.reset(); readStartOdo_ = odo_; nextSampleAt_ = now;
        return;
      }
      // một bước dịch ngang sang trái (về phía vật), không tiến/lùi. Đã thấy mờ -> nửa bước.
      float a = p_.sideStepDeg;
      float d = apPresent_ >= p_.presenceNeed ? p_.sideStepCm * 0.5f : p_.sideStepCm;
      if (apSteps_ < sizeof(stepD_) / sizeof(stepD_[0])) stepD_[apSteps_] = d;
      moveN_ = moveIdx_ = 0; movePhase_ = 0;
      queueMove(true, a);
      queueMove(false, d);
      queueMove(true, -a);
      queueMove(false, -d * cosf(a * kDeg));
      ++apSteps_;
      stats_.sideSteps++;
      apPhase_ = kApStep;
      return;
    }
    case kApStep:
      if (runMoves(now)) { apPhase_ = kApCheck; apChecks_ = 0; apPresent_ = 0; apNear_ = 0; nextSampleAt_ = now; }
      return;
    case kApRead: {
      float crept = odo_ - readStartOdo_;
      bool creep = samples_ >= 3 && crept < p_.readCreepCm;
      if (creep) drive(p_.creepPwm, p_.creepPwm);
      else drive(0, 0);
      advance(cmdL_, cmdR_, dt);
      if (now >= nextSampleAt_) {
        RawColor c;
        bool ok = hal_.readColor(c);
        if (!ok) ++stats_.sensorErrors;
        vote_.add(ok && isRedSample(c, p_, cal_));
        ++samples_;
        nextSampleAt_ = now + p_.colorGapMs;
      }
      bool enough = vote_.reds() >= p_.colorNeed && vote_.reds() * 10u >= samples_ * 6u;
      bool done = enough || (samples_ >= p_.colorSamples && (crept >= p_.readCreepCm || samples_ >= 2 * p_.colorSamples));
      if (!done) return;
      drive(0, 0);
      finishRead(now);
      return;
    }
    case kApReturn:
      if (runMoves(now)) {
        if (mergeMaxCm_ > 0) { apPhase_ = kApMerge; mergeStartOdo_ = odo_; lineFound_.reset(); }
        else apPhase_ = kApRejoin;  // tick() kiểm line rồi về Follow / Recover
      }
      return;
    case kApMerge: {
      // chạy chéo 30° về phía line tới khi mắt giữa thấy line
      drive(p_.movePwm, p_.movePwm);
      advance(cmdL_, cmdR_, dt);
      bool center = (bits & (kL1 | kM | kR1)) != 0;
      if (lineFound_.feed(center) || odo_ - mergeStartOdo_ > mergeMaxCm_) {
        drive(0, 0);
        apPhase_ = kApRejoin;
      }
      return;
    }
    default:
      drive(0, 0);
      return;
  }
}

void Robot::finishRead(uint32_t now) {
  char buf[80];
  apWasRed_ = vote_.reds() >= p_.colorNeed && vote_.reds() * 10u >= samples_ * 6u;  // >= 4 mẫu và >= 60%
  snprintf(buf, sizeof buf, "doc mau: %u/%u do, %u buoc tiep can, c%d s=%.0f", vote_.reds(), samples_, apSteps_, apEdge_ + 1, (double)apS_);
  hal_.log(buf);
  bool fresh = true;
  if (apEdge_ >= 0 && nav_ == NavMode::Map) {
    fresh = tracker_.isNew((uint8_t)apEdge_, apEdgeSide_, apS_, pass_);
    tracker_.mark((uint8_t)apEdge_, apEdgeSide_, apS_, apWasRed_ ? ObjClass::Red : ObjClass::Other, pass_);
  }
  float a = p_.sideStepDeg;
  moveN_ = moveIdx_ = 0; movePhase_ = 0;
  mergeMaxCm_ = 0;
  float lateral = 0;
  for (uint8_t k = 0; k < apSteps_; ++k) lateral += stepD_[k] * sinf(a * kDeg);
  bool roomAhead = nav_ != NavMode::Map || edgeLen_ - dist_ > p_.rejoinMinToNodeCm;
  if (apSteps_ > 0 && roomAhead) {
    // về line nhanh: tiến cho vật ra sau đuôi xe, quay phải, chạy chéo bắt line
    queueMove(false, p_.rejoinClearCm);
    dist_ += p_.rejoinClearCm;
    odo_ += p_.rejoinClearCm;
    queueMove(true, -p_.rejoinDeg);
    mergeMaxCm_ = lateral / sinf(p_.rejoinDeg * kDeg) + 8.0f;
  }
  // Trở về line: đảo ngược từng bước dịch ngang (bước nhỏ -> không quét vào vật kế bên trong cụm;
  // mô phỏng: gộp thành 1 bước lớn làm va chạm tăng gần 3 lần).
  for (uint8_t k = (mergeMaxCm_ > 0 ? 0 : apSteps_); k-- > 0;) {
    float d = stepD_[k];
    queueMove(false, d * cosf(a * kDeg));
    queueMove(true, a);
    queueMove(false, -d);
    queueMove(true, -a);
  }
  apPhase_ = kApReturn;
  if (apWasRed_ && fresh) {
    stats_.alerts++;
    alert_.start(now, p_.alertMs);
    hal_.buzzer(true);
    hal_.led(true);
    snprintf(buf, sizeof buf, "DO! cung c%d s=%.0f (tong %u)", apEdge_ + 1, (double)apS_, stats_.alerts);
    hal_.log(buf);
    persist();
    enter(State::AlertOn);
    return;
  }
  if (apWasRed_) hal_.log("do nhung DA BAO -> bo qua");
}

void Robot::doAlert(uint32_t now) {
  drive(0, 0);
  if (alert_.update(now)) return;
  hal_.buzzer(false);
  hal_.led(false);
  if (p_.enableGripper && p_.gripSide == Side::Left) { gripper_.start(now, hal_); enter(State::Grip); }
  else enter(State::Approach);  // tiếp tục pha trở về line
}

void Robot::doGrip(uint32_t now) {
  drive(0, 0);
  if (gripper_.update(now, hal_)) return;
  stats_.grabbed++;
  char buf[40];
  snprintf(buf, sizeof buf, "GAP xong: %u vat tren xe", stats_.grabbed);
  hal_.log(buf);
  enter(State::Approach);
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

// Vòng 2: báo số rác đang chở bằng số tiếng bíp (mỗi tiếng 300 ms), lặp lại mỗi 5 giây.
void Robot::doGoal(uint32_t now) {
  hal_.setMotors(0, 0);
  if (!p_.enableGripper || stats_.grabbed == 0) return;
  uint32_t t = (now - stateAt_) % 5000;
  uint32_t k = t / 600;
  hal_.buzzer(k < stats_.grabbed && (t % 600) < 300);
}

void Robot::persist() {
  uint16_t regs[kBackupRegs];
  bool running = state_ != State::Boot && state_ != State::Ready && state_ != State::Calib;
  uint32_t el = running ? hal_.millis() - matchStart_ : priorElapsedMs_;
  encodeBackup(regs, stats_.covered, tracker_, stats_.restarts, (uint16_t)(el / 1000));
  hal_.saveBackup(regs, kBackupRegs);
}

// ---------------------------------------------------------------- hiệu chỉnh màu
// 4 bước: lớp Đỏ, Vàng, Xanh, Tím - đưa mẫu sát TCS3200 (sườn trái, ~1,5 cm, che cả E18 trái)
// cho tới khi bíp, rồi rút ra. LED nháy (số lớp + 1) lần mỗi chu kỳ.
void Robot::doCalib(uint32_t now) {
  hal_.setMotors(0, 0);
  if (calibStep_ >= kNumClasses) return;
  uint8_t cls = calibStep_;
  uint32_t ph = (now - stateAt_) % 2000;
  if (!calibWaitClear_ && calibCount_ == 0) hal_.led(ph < (uint32_t)(cls + 1) * 300 && (ph % 300) < 150);

  bool obs = hal_.obstacle(Side::Left);
  if (calibWaitClear_) {
    if (now - stateAt_ >= 150) hal_.buzzer(false);
    if (!obs && now - stateAt_ >= 150) { calibWaitClear_ = false; hal_.buzzer(false); }
    return;
  }
  if (!obs) { calibCount_ = 0; return; }
  if (now < nextSampleAt_) return;
  if (calibCount_ == 0) { cal_.count[cls] = 0; cal_.cent[cls][0] = 0; cal_.cent[cls][1] = 0; }
  RawColor c;
  if (hal_.readColor(c) && colorPresent(c, p_)) { calibAdd(cal_, cls, c); ++calibCount_; }
  nextSampleAt_ = now + p_.colorGapMs;
  if (calibCount_ >= p_.calibSamples) {
    hal_.buzzer(true);  // bíp ngắn: xong bước này
    stateAt_ = now;
    calibCount_ = 0;
    calibWaitClear_ = true;
    ++calibStep_;
    char buf[48];
    snprintf(buf, sizeof buf, "CALIB lop %u xong", cls);
    hal_.log(buf);
    if (calibStep_ >= kNumClasses) {
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
