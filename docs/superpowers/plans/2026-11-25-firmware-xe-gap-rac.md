# Firmware xe dò line gắp rác – Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Firmware PlatformIO/Arduino cho STM32F103 (Blue Pill) với lõi thuật toán độc lập phần cứng, kiểm thử bằng unit test + mô phỏng toàn sân trên PC.

**Architecture:** `firmware/lib/core` (C++17 thuần, namespace `core`) chứa toàn bộ thuật toán và FSM, chỉ nói chuyện với phần cứng qua lớp trừu tượng `core::Hal`. `firmware/src` cài `ArduinoHal` + `main.cpp`. `sim/` cài `SimHal` trên mô hình động học 2D để chạy nguyên nhiệm vụ.

**Tech Stack:** C++17, PlatformIO (ststm32, bluepill_f103c8, framework arduino), `zig c++` 0.17 làm trình biên dịch host (`.tools/venv`), Python + python-docx cho tài liệu.

**Spec:** `docs/superpowers/specs/2026-11-25-firmware-xe-gap-rac-design.md`

## Global Constraints
- Pinout đúng bảng mục 3 của spec; I2C mặc định PB6/PB7; debug TX PB5; servo PA6/PA7.
- Bit cảm biến line: bit0 = L2, bit1 = L1, bit2 = M, bit3 = R1, bit4 = R2; 1 = đen.
- Lệnh động cơ −255…255; dương = tiến.
- Cảnh báo 2 200 ms; 6 mẫu màu, đỏ khi ≥ 4/6; GO_HOME 240 000 ms; cung 150 cm.
- Ngưỡng mặc định: TCS3200 r<450 && r<0,55g && r<0,55b (chu kỳ µs); TCS34725 c>80 && r>1,10g && r>1,45b.
- Lõi `core` không include `Arduino.h`; không cấp phát động sau khởi tạo.
- Lộ trình mặc định (nút): H,E,B,A,D,E,F,C,B,E,F,I,H,G,D,A; hành động: S,S,L,L,L,S,L,L,L,L,R,R,S,R,S,STOP.

## Review Focus
1. Loa bo cạnh nút vừa đi qua kích hoạt nút giả → cổng quãng đường phải chặn (test trong Task 2 & sim Task 7).
2. Hai vật đỏ cùng bên cách nhau ~15 cm → phải báo cả hai, không bị khử lặp nuốt (Task 5).
3. Đi lại cung c4/c7 theo chiều ngược → không báo lặp (Task 5, sim Task 7).
4. Cảm biến màu phải lỗi I2C → không treo, vẫn chạy bằng cảm biến trái (Task 6/7 sim inject lỗi).
5. Mất định vị (nút sai loại 2 lần) → chuyển fallback bám phải, vẫn về đích theo thời gian (Task 6/7).

---

### Task 1: Hạ tầng test host + vị trí line + PID
**Files:** Create `firmware/lib/core/src/{params.h,hal.h,line_pos.h,line_pos.cpp,pid.h,pid.cpp}`, `firmware/test/host/{minitest.h,test_main.cpp,test_line_pid.cpp}`, `tools/run_host_tests.sh`
**Interfaces:**
- Produces: `struct core::Params` (mọi hằng số, giá trị mặc định trong định nghĩa); `core::Hal` (abstract: `millis, readLine, obstacle(Side), readColor(Side,RawColor&), setMotors, buzzer, led, log, loadBackup/saveBackup, loadCalib/saveCalib, servo`); `float core::linePosition(uint8_t bits, const Params&, bool innerOnly, float last, bool* seen)`; `class core::Pid{ Pid(float kp,float ki,float kd,float iLimit); float update(float err,float dt); void reset(); }`.
- [ ] Test: `linePosition(0b00100)==0`; `0b00110` ≈ −1,25 (trung bình L1,M theo tọa độ −2,5/0); mất line giữ dấu last ×1,0 → ±LOST_POS; innerOnly bỏ L2/R2.
- [ ] Test: PID chỉ P trả Kp·e; tích phân bị kẹp ±iLimit; reset xóa trạng thái.
- [ ] Chạy `tools/run_host_tests.sh` → FAIL (chưa có code) → cài đặt → PASS → commit.

### Task 2: Bộ phát hiện nút giao
**Files:** `core/src/node_detector.{h,cpp}`, `test/host/test_node.cpp`
**Interfaces:** `struct core::NodeShape{bool left,right,straight;}`; `class core::NodeDetector{ void reset(); bool feedCandidate(uint8_t bits); void beginScan(); void accumulate(uint8_t bits); NodeShape finish(uint8_t bitsAfter) const; }` — ứng viên khi (L2||R2) && (L1||M||R1) đúng `NODE_CONFIRM_SAMPLES`(3) mẫu liên tiếp.
- [ ] Test: 2 mẫu rồi đứt → không ứng viên; 3 mẫu → ứng viên; chỉ L2 không có mắt giữa → không.
- [ ] Test: tích lũy L2 trong quét, sau đó M đen → {left, straight}; R2 và sau đó trắng → {right, !straight}.
- [ ] Commit.

### Task 3: Bản đồ, lộ trình, BFS, lập lộ trình phủ tham lam
**Files:** `core/src/field_map.{h,cpp}`, `test/host/test_map.cpp`
**Interfaces:** `enum core::Dir{N,E,S,W}`; `enum class core::Act{Straight,Left,Right,UTurn,Stop}`; node index A=0…I=8, `START=9`, `STUB=10`; `int8_t neighbor(uint8_t node, Dir)`; `int8_t edgeOf(uint8_t a,uint8_t b)` (0..11, thứ tự c1..c12 = AB,BC,AD,BE,CF,DE,EF,DG,EH,FI,GH,HI); `Dir applyAct(Dir,Act)`; `NodeShape expectedShape(uint8_t node, Dir heading)`; `Act actToward(Dir heading, Dir want)`; `extern const Act kDefaultRoute[16]`; `uint8_t planPath(uint8_t node, Dir heading, uint8_t target, Act* out, uint8_t max)` (BFS, không quay đầu); `uint8_t planCoverage(uint8_t node, Dir heading, uint16_t covered, uint8_t target, Act* out, uint8_t max)`.
- [ ] Test: mô phỏng kDefaultRoute từ (H, N) phủ đủ 12 cung và kết thúc ở A; expectedShape(B, N) = {left,right,straight=STUB}; planPath(E,N→A) dài 2; planCoverage với covered = tất cả trừ c11 → đi qua c11 rồi về A.
- [ ] Commit.

### Task 4: Phân loại màu
**Files:** `core/src/color.{h,cpp}`, `test/host/test_color.cpp`
**Interfaces:** `struct core::RawColor{uint16_t r,g,b,c; }`; `enum class core::Sensor{Tcs3200,Tcs34725}`; `struct core::Calib{bool valid; float red[2][3]; float others[2][3][3]; }` (theo sensor, chromaticity r,g); `bool core::isRedSample(Sensor, const RawColor&, const Params&, const Calib&)`; `class core::ColorVote{ void reset(); void add(bool red); uint8_t reds() const; uint8_t total() const; bool decide(uint8_t need) const; }`; `void chroma(Sensor,const RawColor&, float out[3])`.
- [ ] Test ngưỡng mặc định với mẫu đỏ/vàng/tím/xanh/nền (đúng/sai như spec); test calib: tâm đỏ gần → đỏ, cam gần tâm khác → không; vote 4/6.
- [ ] Commit.

### Task 5: Khử lặp + đóng gói backup
**Files:** `core/src/object_tracker.{h,cpp}`, `core/src/backup_codec.{h,cpp}`, `test/host/test_tracker.cpp`
**Interfaces:** `class core::ObjectTracker{ void clear(); bool isNew(uint8_t edge, Side, float sFromLow) const; void mark(uint8_t edge, Side, float sFromLow); uint8_t count() const; const uint8_t* raw() const; void setRaw(const uint8_t*); }` (5 đoạn × 30 cm, kiểm tra s và s±12 cm); `bool encodeBackup(uint16_t regs[10], uint16_t covered, const ObjectTracker&, uint8_t restarts)`, `bool decodeBackup(const uint16_t regs[10], uint16_t& covered, ObjectTracker&, uint8_t& restarts)` (magic 0xA ở 4 bit cao regs[0]).
- [ ] Test: mark (c4, L, 40) → isNew(c4,L,45)=false, (c4,L,95)=true, (c4,R,40)=true; s ở biên đoạn 59/61 coi là trùng; encode→decode khứ hồi bằng nhau; regs sai magic → false.
- [ ] Commit.

### Task 6: Robot FSM (tích hợp lõi)
**Files:** `core/src/{alert.h,alert.cpp,robot.h,robot.cpp}`, `test/host/test_robot.cpp`
**Interfaces:** `class core::Alert{ void start(uint32_t now,uint32_t ms); bool update(uint32_t now); bool active() const; }`; `enum class core::State{Boot,Calib,Test,Ready,Follow,NodeScan,Turn,ScanObject,Alert,Grip,UTurn,Recover,Goal}`; `enum class core::BootMode{Run,Calib,Test,ClearMemory}`; `class core::Robot{ Robot(Hal&,const Params&); void begin(BootMode); void tick(); State state() const; const Stats& stats() const; }`; `struct core::Stats{uint8_t alerts; uint8_t node; Dir heading; uint16_t covered; bool fallback; uint32_t finishMs; uint8_t grabbed;}`.
- [ ] Test (FakeHal kịch bản): sau begin(Run) + 1 s → Follow; obstacle trái + màu đỏ 6 mẫu → state Alert, buzzer bật ≥ 2 200 ms rồi tắt; cùng vị trí lần 2 → không báo; lỗi readColor phải → không treo, vẫn Follow.
- [ ] Commit.

### Task 7: Trình mô phỏng toàn sân + kiểm thử chấp nhận
**Files:** `sim/{world.h,world.cpp,sim_hal.h,sim_hal.cpp,main.cpp}`, `tools/run_sim.sh`
**Interfaces:** `sim::World{ build(seed); lineAt(x,y); objects; step(dt, pwmL, pwmR) }`; `sim::SimHal : core::Hal`; `main --seeds N --svg out.svg` in bảng tổng hợp.
- [ ] Mô hình: line 3 cm + loa bo cung tròn R=15 cm ở B/E/H; xe vi sai (trục 14 cm, 0,35 cm/s/PWM, vùng chết 30, quán tính τ=60 ms); 5 mắt bán nguyệt (R=5 cm, tâm 6 cm trước trục, góc 0/±30/±60°); E18 tầm 12 cm vuông góc sườn; màu đọc được khi bề mặt vật ≤ 4 cm; nhiễu.
- [ ] Chấp nhận trên 50 seed: phủ 12/12 ≥ 98 %, đỏ đúng ≥ 95 %, báo nhầm = 0, báo lặp = 0, về A ≥ 98 %, thời gian trung vị < 180 s; seed có lỗi I2C phải không treo.
- [ ] Commit.

### Task 8: HAL Arduino + main + chế độ CALIB/TEST + biên dịch chéo
**Files:** `firmware/include/pins.h`, `firmware/src/{arduino_hal.h,arduino_hal.cpp,main.cpp}`
**Interfaces:** `ArduinoHal : core::Hal` (TCS3200 pulseIn timeout 3 ms/kênh, TCS34725 đọc thanh ghi 0x14..0x1B, EEPROM cho Calib, backup.h cho BKP, SoftwareSerial TX PB5, Servo PA6/PA7 khi ENABLE_GRIPPER).
- [ ] `pio run -e bluepill` → SUCCESS, in RAM/Flash.
- [ ] Commit.

### Task 9: Khung gắp Vòng 2
**Files:** `core/src/gripper.{h,cpp}`, `test/host/test_gripper.cpp`; nối vào Robot khi `p.enableGripper`.
**Interfaces:** `class core::Gripper{ void start(uint32_t now); bool update(uint32_t now, Hal&); }` chuỗi OPEN→LOWER→CLOSE→LIFT với thời gian `GRIP_STEP_MS`.
- [ ] Test chuỗi góc servo theo thời gian; sim chạy với enableGripper đếm grabbed = số đỏ.
- [ ] Commit.

### Task 10: Tài liệu
**Files:** `README.md`, `tai_lieu/Huong_dan_su_dung.docx`, `tai_lieu/Bao_cao_ky_thuat.docx`, hình trong `tai_lieu/hinh/`
- [ ] Hướng dẫn: cài đặt, đấu nối, nạp, chế độ khởi động, CALIB, TEST, tinh chỉnh, xử lý sự cố.
- [ ] Báo cáo: phần cứng (schematic + sai lệch), thuật toán, phần mềm, kiểm thử (số liệu sim thật), Vòng 2.
- [ ] Mở bằng Word, render kiểm tra; commit.
