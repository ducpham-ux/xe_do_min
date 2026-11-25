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
  Boot, Calib, Ready, Follow, Seek, NodeScan, Turn, Approach, AlertOn, Grip, Recover, Goal
};
enum class BootMode : uint8_t { Run, Calib, ClearMemory };
enum class NavMode : uint8_t { Map, Fallback };

struct Stats {
  uint8_t alerts = 0;          // số vật đỏ đã báo (kể cả trước restart)
  uint8_t restarts = 0;
  uint16_t covered = 0;        // mặt nạ cung đã đi
  uint8_t nodesSeen = 0;
  uint8_t mismatches = 0;
  uint8_t relocalized = 0;     // số lần tự định vị lại nhờ bản đồ
  uint8_t objectsRead = 0;     // số lần dừng đọc màu
  uint8_t pendingSeen = 0;     // số vật bên phải được ghi nhớ để đọc sau
  uint8_t sideSteps = 0;       // tổng số bước dịch ngang tiếp cận vật
  uint16_t lineLost = 0;       // số lần mất line khi đang bám
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
  float turnRate() const { return turnRate_; }
  uint32_t pendingDirMask() const { return pendingDir_; }
  const Calib& calib() const { return cal_; }
  const ObjectTracker& tracker() const { return tracker_; }

 private:
  // một chuyển động mở (không có encoder): quay tại chỗ theo góc hoặc đi thẳng theo quãng
  struct Move { bool pivot; float amount; };  // pivot: độ (+ = trái), thẳng: cm (+ = tiến)

  void enter(State s);
  void doReady(uint32_t now);
  void doFollow(uint32_t now, uint8_t bits, float dt);
  void doSeek(uint32_t now, uint8_t bits);
  void doNodeScan(uint32_t now, uint8_t bits, float dt);
  void doTurn(uint32_t now, uint8_t bits);
  void doApproach(uint32_t now, uint8_t bits, float dt);
  void doAlert(uint32_t now);
  void doGrip(uint32_t now);
  void doRecover(uint32_t now, uint8_t bits);
  void doCalib(uint32_t now);
  void doGoal(uint32_t now);
  Act onNode(uint32_t now, const NodeShape& shape);
  void localize(const NodeShape& shape);
  void replan(bool full);
  Act fallbackChoice(const NodeShape& shape);
  void startTurn(uint32_t now, Act a, bool uturnAtDeadEnd);
  void finish(uint32_t now);
  void drive(int16_t l, int16_t r);       // áp hệ số bù tốc độ + kẹp
  void advance(int16_t l, int16_t r, float dt);  // tích phân quãng đường
  float speedAt(int16_t pwm) const;       // cm/s ở PWM cho trước (ước lượng)
  void persist();
  int8_t currentEdge() const;
  float sFromLow() const;
  bool lowToHigh() const { return lastNode_ < node_ || lastNode_ == kStart; }
  bool obstacleEdge(Side s);
  void onRightObject();
  bool startApproach(uint32_t now);
  void queueMove(bool pivot, float amount);
  bool runMoves(uint32_t now);            // true khi đã chạy hết hàng đợi chuyển động
  void finishRead(uint32_t now);
  void resumeFollow(uint32_t now);

  Hal& hal_;
  const Params& p_;
  Calib cal_;
  NodeDetector nd_;
  Pid pid_;
  ObjectTracker tracker_;
  Alert alert_;
  Gripper gripper_;
  Localizer loc_;
  Stats stats_;

  State state_ = State::Boot;
  uint32_t stateAt_ = 0, matchStart_ = 0, lastCtrl_ = 0, lostSince_ = 0, lastTick_ = 0;
  bool ticked_ = false;
  int16_t cmdL_ = 0, cmdR_ = 0;   // lệnh danh định (trước khi bù tốc độ)
  float vCmd_ = 0;                // tốc độ nền đang dùng (tăng mềm)
  float odo_ = 0, learnDist_ = 0;
  uint32_t priorElapsedMs_ = 0, lastPersist_ = 0;  // thời gian thi trước restart
  uint8_t calibStep_ = 0;
  bool lost_ = false;
  float lastPos_ = 0;

  // định vị
  NavMode nav_ = NavMode::Map;
  uint8_t lastNode_ = kStart, node_ = kH;
  Dir heading_ = Dir::N;
  float edgeLen_ = 30, dist_ = 0, lastNodeOdo_ = 0;
  float vEst_ = 30, pwmScale_ = 1.0f, turnRate_ = 200;
  bool learnOk_ = false;
  Act plan_[64];
  uint8_t planLen_ = 0, planIdx_ = 0;
  uint32_t lastNodeAt_ = 0;
  uint8_t noisyObs_ = 0, ambNodes_ = 0;
  bool replan_ = false, fullReplan_ = false;
  uint32_t pendingDir_ = 0;       // cung có hướng cần đi để đọc vật bên phải
  uint8_t pass_ = 0;              // số hiệu lượt đi trên cung hiện tại

  // nút
  NodeShape nodeShape_;
  uint8_t nodePhase_ = 0;         // 0: xét lối thẳng, 1: bò vào tâm nút
  float nodeOdo_ = 0, crawlTarget_ = 0;
  uint8_t centerRun_ = 0;
  Act pendingAct_ = Act::Straight;

  // dự phòng (thuật toán cũ)
  Act lastTurn_ = Act::Straight;
  uint8_t sameTurn_ = 0;

  // quay
  Act turnAct_ = Act::Straight;
  uint8_t turnPhase_ = 0;
  uint32_t phaseAt_ = 0, fastMs_ = 0, slowStartAt_ = 0;
  Confirm lineFound_{2};
  int8_t seekDir_ = 1;

  // vật cản
  uint8_t obsRun_[2] = {0, 0};
  bool obsArmed_[2] = {true, true};
  float distAtTrig_[2] = {-1000, -1000};

  // tiếp cận + đọc màu
  uint8_t apPhase_ = 0, apSteps_ = 0, apChecks_ = 0, apPresent_ = 0, apNear_ = 0;
  float stepD_[8] = {};           // quãng tiến của từng bước dịch ngang (để đi ngược lại)
  float postReadOdo_ = -1000;     // vị trí lúc đọc xong vật gần nhất
  float mergeMaxCm_ = 0, mergeStartOdo_ = 0;
  Move moves_[20];
  uint8_t moveN_ = 0, moveIdx_ = 0, movePhase_ = 0;
  uint32_t moveAt_ = 0, moveDur_ = 0;
  uint32_t nextSampleAt_ = 0;
  uint8_t samples_ = 0;
  ColorVote vote_;
  float readStartOdo_ = 0;
  int8_t apEdge_ = -1;
  Side apEdgeSide_ = Side::Left;
  float apS_ = 0;
  bool apWasRed_ = false;

  // calib
  uint8_t calibCount_ = 0;
  bool calibWaitClear_ = false;
};

}  // namespace core
