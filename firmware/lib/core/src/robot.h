// robot.h - "bộ não" của xe: máy trạng thái hành vi.
// Gọi begin() một lần, sau đó gọi tick() liên tục trong loop() (không có delay).
#pragma once
#include <stdint.h>
#include "alert.h"
#include "backup_codec.h"
#include "color.h"
#include "field_map.h"
#include "gripper.h"
#include "hal.h"
#include "line_pos.h"
#include "node_detector.h"
#include "object_tracker.h"
#include "params.h"
#include "pid.h"

namespace core {

enum class State : uint8_t {
  Boot, Calib, Ready, Follow, NodeScan, Turn, ScanObject, AlertOn, Grip, Recover, Goal
};
enum class BootMode : uint8_t { Run, Calib, ClearMemory };
enum class NavMode : uint8_t { Map, Fallback };

struct Stats {
  uint8_t alerts = 0;          // số vật đỏ đã báo (kể cả trước restart)
  uint8_t restarts = 0;
  uint16_t covered = 0;        // mặt nạ cung đã đi
  uint8_t nodesSeen = 0;
  uint8_t mismatches = 0;
  uint16_t sensorErrors = 0;
  uint8_t grabbed = 0;
  bool fallback = false;
  bool wentHome = false;
  uint32_t finishMs = 0;       // thời điểm về đích (tính từ START)
};

class Robot {
 public:
  Robot(Hal& hal, const Params& p);
  void begin(BootMode mode);
  void tick();
  State state() const { return state_; }
  const Stats& stats() const { return stats_; }
  uint8_t nextNode() const { return node_; }
  Dir heading() const { return heading_; }
  NavMode navMode() const { return nav_; }
  float distOnEdge() const { return dist_; }
  const Calib& calib() const { return cal_; }

 private:
  void enter(State s);
  void doReady(uint32_t now);
  void doFollow(uint32_t now, uint8_t bits, float dt);
  void doNodeScan(uint32_t now, uint8_t bits, float dt);
  void doTurn(uint32_t now, uint8_t bits);
  void doScanObject(uint32_t now, float dt);
  void doAlert(uint32_t now);
  void doGrip(uint32_t now);
  void doRecover(uint32_t now, uint8_t bits);
  void doCalib(uint32_t now);
  void doGoal(uint32_t now);
  void onNode(uint32_t now, const NodeShape& shape);
  Act fallbackChoice(const NodeShape& shape);
  void startTurn(uint32_t now, Act a, bool uturnAtDeadEnd);
  void finish(uint32_t now);
  void drive(int16_t l, int16_t r);       // áp hệ số bù tốc độ + kẹp
  void advance(int16_t l, int16_t r, float dt);  // tích phân quãng đường
  void persist();
  int8_t currentEdge() const;
  float sFromLow() const;
  bool obstacleEdge(Side s, uint32_t now);

  Hal& hal_;
  const Params& p_;
  Calib cal_;
  NodeDetector nd_;
  Pid pid_;
  ObjectTracker tracker_;
  Alert alert_;
  Gripper gripper_;
  Stats stats_;

  State state_ = State::Boot;
  uint32_t stateAt_ = 0, matchStart_ = 0, lastCtrl_ = 0, lostSince_ = 0, lastTick_ = 0;
  bool ticked_ = false;
  int16_t cmdL_ = 0, cmdR_ = 0;   // lệnh danh định (trước khi bù tốc độ)
  float odo_ = 0, learnDist_ = 0;
  uint8_t calibStep_ = 0;
  bool lost_ = false;
  float lastPos_ = 0;

  // định vị
  NavMode nav_ = NavMode::Map;
  uint8_t lastNode_ = kStart, node_ = kH;
  Dir heading_ = Dir::N;
  float edgeLen_ = 30, dist_ = 0;
  float vEst_ = 40, pwmScale_ = 1.0f;
  bool learnOk_ = false;
  Act plan_[48];
  uint8_t planLen_ = 0, planIdx_ = 0;
  uint32_t lastNodeAt_ = 0;

  // dự phòng (thuật toán cũ)
  Act lastTurn_ = Act::Straight;
  uint8_t sameTurn_ = 0;

  // quay
  Act turnAct_ = Act::Straight;
  uint8_t turnPhase_ = 0;
  uint32_t phaseAt_ = 0;
  Confirm lineFound_{2};

  // vật cản
  uint8_t obsRun_[2] = {0, 0};
  bool obsArmed_[2] = {true, true};
  float distAtScan_[2] = {-1000, -1000};
  Side scanSide_ = Side::Left;
  uint8_t samples_ = 0;
  ColorVote vote_;
  uint32_t nextSampleAt_ = 0;
  uint16_t sampleErrors_ = 0;
  float scanStartOdo_ = 0;
  uint8_t scanLowRun_ = 0;

  // gắp / calib
  uint8_t calibCount_ = 0;
  bool calibWaitClear_ = false;
};

}  // namespace core
