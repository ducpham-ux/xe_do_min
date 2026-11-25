// params.h - TOÀN BỘ tham số tinh chỉnh của xe nằm ở đây.
// "(đã đo thật)" = lấy từ bản .ino/đợt test đã chạy trên xe; "(test lần 1)" = rút ra từ
// báo cáo test lần 1; các giá trị khác là điểm xuất phát, cần tinh chỉnh trên sân.
#pragma once
#include <stdint.h>
#include "hal.h"

namespace core {

struct Params {
  // ---------- Hình học xe (test lần 1) ----------
  // Xe 26 x 16 cm. 5 mắt TCRT5000 thẳng hàng trên thanh 7 cm ở mũi xe, cao 0,5 cm.
  // Tọa độ ngang của từng mắt so với tâm thanh (cm), thứ tự L2, L1, M, R1, R2. Âm = bên trái.
  float sensorY[5] = {-3.5f, -1.75f, 0.0f, 1.75f, 3.5f};
  float sensorAheadCm = 12.0f;    // thanh mắt nằm trước TÂM QUAY (giữa 2 trục bánh) bao nhiêu cm
  float lineWidthCm = 2.8f;       // bề rộng vạch đen (thể lệ 2,8-3,0 cm)
  float halfWidthCm = 8.0f;       // nửa bề ngang xe (sườn xe cách tâm)

  // ---------- Bám line PID ----------
  float kp = 16.0f;               // PWM / cm
  float ki = 0.0f;                // PWM / (cm*s)
  float kd = 0.9f;                // PWM / (cm/s), khâu D có lọc thông thấp
  float dTauS = 0.025f;           // hằng số lọc khâu D (s) - mắt số nhảy bậc 0,875 cm
  float iLimit = 30.0f;
  uint8_t ctrlMs = 4;             // chu kỳ điều khiển PID (ms)
  float lostPos = 4.5f;           // vị trí giả định khi mất line (cm), giữ dấu lần cuối
  // Tốc độ (test lần 1: xe cũ chạy 140 quá nhanh -> mất line). Giảm và thêm giảm tốc theo sai số.
  int16_t baseSpeed = 105;        // tốc độ nền trên đoạn thẳng
  int16_t approachSpeed = 85;     // tốc độ khi gần nút dự kiến / thấy cung bo
  float approachFrac = 0.80f;     // giảm tốc khi đã đi 80% cung
  float slowOnErr = 0.35f;        // giảm tới 35% tốc độ khi line ở mép thanh (|pos| = 3,5 cm)
  int16_t minFollowPwm = 70;      // không giảm dưới mức này khi bám line
  float accelPwmPerS = 450.0f;    // tăng tốc mềm sau khi dừng/quay (chống trượt bánh, lệch line)
  int16_t maxPwm = 200;
  // Tìm lại line: mất line ở mép -> quay ngay về phía mất (thanh cách tâm quay 12 cm nên
  // chỉ cần quay vài độ là thấy lại), không chờ quét trái-phải.
  float seekMinPos = 1.2f;        // |vị trí cuối| >= 1,2 cm mới coi là "trôi ra mép"
  int16_t seekPwm = 90;
  uint16_t seekMaxMs = 900;       // quá thời gian -> quét 2 phía (Recover)
  float fallbackGateCm = 18.0f;   // chế độ dự phòng: khoảng cách tối thiểu giữa 2 nút
  uint8_t calibSamples = 12;      // số mẫu mỗi lần học màu

  // ---------- Nút giao (thanh thẳng) ----------
  // Vạch ngang thật làm cả dải đen liền từ M ra tới mắt ngoài; cung bo 45° tạo dải RỜI
  // ở mắt ngoài (mẫu "tách") hoặc dải liền rất ngắn khi cung vừa tách khỏi line.
  float nodeGateFrac = 0.50f;     // chỉ xét nút khi đã đi >= 50% chiều dài cung
  float edgeLenCm = 150.0f;       // chiều dài cung giữa 2 nút
  float startStubCm = 30.0f;      // tâm quay lúc xuất phát -> nút H (ước lượng)
  float minCrossCm = 1.6f;        // vạch ngang phải dài >= 1,6 cm theo hướng đi (vạch thật 2,8)
  float suspectCrossCm = 2.3f;    // nếu ngay trước đó line "phình" 3 mắt (cung bo đang nhập) thì đòi 2,3 cm
  float mergeWideCm = 0.8f;       // phình 3 mắt liên tục >= 0,8 cm = dấu hiệu cung bo
  float straightCheckCm = 2.0f;   // sau vạch ngang bò 2 cm để xét còn lối thẳng
  int16_t scanSpeed = 80;         // tốc độ bò qua/vào tâm nút
  float crawlExtraCm = 0.0f;      // bù thêm khi bò vào tâm nút trước khi quay (đo trên sân)

  // ---------- Quay tại chỗ ----------
  int16_t turnSpeed = 110;        // (đã đo thật)
  int16_t turnSlowSpeed = 85;     // (đã đo thật) tốc độ tìm line mới
  float turnRateDegS = 200.0f;    // tốc độ quay ở turnSpeed (độ/s) - đo bằng TEST, sau đó tự học
  float turnMinDeg = 62.0f;       // rẽ 90°: quay nhanh tối thiểu 62° (bỏ qua cung bo 45°)
  float uturnMinDeg = 140.0f;     // quay đầu: tối thiểu 140°
  float turnMaxExtraDeg = 70.0f;  // quá (góc + 70°) mà chưa thấy line -> Recover
  float turnLearnAlpha = 0.4f;    // tốc độ học turnRate từ mỗi lần rẽ thành công
  uint16_t brakeMs = 25;          // (đã đo thật) phanh ngược
  int16_t brakePwm = 110;

  // ---------- Ước lượng vận tốc (không có encoder) ----------
  float vBaseCmS = 30.0f;         // vận tốc ở baseSpeed (đo bằng chế độ TEST)
  float pwmDeadband = 30.0f;      // PWM dưới mức này bánh không quay (đo bằng chế độ TEST)
  float vLearnAlpha = 0.6f;       // tốc độ học từ thời gian chạy hết cung
  bool adaptSpeed = true;         // tự chỉnh PWM để giữ vận tốc mục tiêu khi pin thay đổi
  float adaptMin = 0.85f, adaptMax = 1.20f;

  // ---------- Vật cản E18 (tầm 7 cm, test lần 1) ----------
  uint8_t obstacleDebounce = 3;   // mẫu (1 ms) liên tiếp
  // Vật khác phải có sườn LOW->HIGH mới của E18 và cách lần kích trước >= 3 cm (cụm vật sát nhau).
  float obstacleRearmCm = 3.0f;
  float rightRearmCm = 3.0f;      // vật bên phải chỉ được GHI NHỚ, không dừng
  float postReadCm = 6.0f;        // đọc xong 1 vật: bỏ qua E18 trái thêm 6 cm (không đọc lại chính nó)

  // ---------- Tiếp cận vật + đọc màu TCS3200 (sườn trái, tầm 1,5 cm, test lần 1) ----------
  // E18 kích ở MÉP vật nên tâm vật còn ở phía trước: tiến alignCm cho TCS ngang tâm vật.
  // Rồi dịch ngang về phía vật bằng các bước: quay trái a° -> tiến d -> quay phải a° -> lùi d.cos(a)
  // (mỗi bước dịch ngang d.sin(a) mà không tiến/lùi). Góc nhỏ để mũi xe quét ra ít (không va vật).
  float alignCm = 2.0f;           // + tcsAheadOfE18Cm được tính thêm
  float sideStepDeg = 15.0f;
  float sideStepCm = 8.0f;        // -> dịch ngang ~2,1 cm mỗi bước
  uint8_t sideStepMax = 4;        // tối đa 4 bước (bước đủ ~2,1 cm, bước nửa ~1 cm)
  int16_t movePwm = 95;           // PWM khi tiến/lùi trong lúc tiếp cận
  uint16_t moveSettleMs = 40;     // dừng ổn định giữa các bước
  // Hai mức "thấy vật" theo chu kỳ kênh nhỏ nhất của TCS3200 (đo bằng TEST, đặt vật ở 1,5 và 2,5 cm):
  uint16_t presenceMaxUs = 600;   // thấy MỜ (vật ~2,5 cm): chỉ dịch nửa bước nữa
  uint16_t nearMaxUs = 450;       // thấy RÕ (vật <= ~2 cm): đọc màu
  uint8_t presenceNeed = 2;       // cần 2/3 mẫu
  uint8_t colorSamples = 7;       // số mẫu đọc màu
  uint8_t colorNeed = 4;          // cần >= 4 mẫu đỏ
  uint16_t colorGapMs = 25;       // khoảng giữa 2 mẫu (1 lần đọc 3 kênh ~2-3 ms)
  int16_t creepPwm = 72;          // bò chậm dọc vật khi đọc (vật có thể lệch dọc vài cm)
  float readCreepCm = 3.0f;       // bò tối đa 3 cm trong lúc đọc
  float tcsAheadOfE18Cm = 0.0f;   // TCS3200 nằm trước E18 trái bao nhiêu cm (âm = sau)
  // Về lại line nhanh (khi còn cách nút > rejoinMinToNodeCm): tiến thẳng cho vật ra sau đuôi xe,
  // quay phải rejoinDeg rồi chạy chéo tới khi thanh mắt bắt lại line (như nhập line bình thường).
  float rejoinClearCm = 18.0f;
  float rejoinDeg = 30.0f;
  float rejoinMinToNodeCm = 10000.0f;  // TẮT mặc định: mô phỏng cho thấy dễ va vật kế tiếp trong cụm
  // Ngưỡng mặc định TCS3200 (đã đo thật)
  uint16_t t3200MaxR = 450;       // chu kỳ kênh đỏ (us) phải nhỏ hơn
  float t3200Ratio = 0.55f;       // r < g*ratio && r < b*ratio
  // Khi đã hiệu chỉnh (CALIB): bán kính và biên tin cậy trong không gian chromaticity
  float calibRadius = 0.07f, calibMargin = 0.02f;

  // ---------- Khử lặp ----------
  // Coi là cùng vật (của lượt đi khác) nếu lệch < 10 cm. Báo lặp không bị trừ điểm, bỏ sót đỏ
  // mất 10 điểm -> để nhỏ.
  float dupTolCm = 10.0f;

  // ---------- Cảnh báo ----------
  uint16_t alertMs = 2200;        // thể lệ: >= 2 giây

  // ---------- Nhiệm vụ ----------
  uint16_t startDelayMs = 1000;   // sau khi nhấn RESET/START
  uint32_t goHomeMs = 245000;     // quá 4:05 -> về đích đường ngắn nhất
  uint32_t matchMs = 300000;      // 5 phút: dừng hẳn
  bool readRightObjects = true;   // vật bên phải: đi ngược cung đó để đọc bằng sườn trái
  uint8_t relocMaxNodes = 4;      // mơ hồ vị trí quá 4 nút -> chế độ dự phòng (bám phải)
  uint32_t fallbackMinExploreMs = 45000;  // (đã đo thật) chế độ dự phòng: trắng sau 45 s = ĐÍCH
  uint16_t lostConfirmMs = 40;    // xác nhận toàn trắng
  uint16_t recoverTimeoutMs = 3000;

  // ---------- Vòng 2: gắp ----------
  bool enableGripper = false;
  Side gripSide = Side::Left;     // chỉ sườn trái có cảm biến màu
  uint8_t gripOpenDeg = 90, gripCloseDeg = 20;   // servo kẹp (PA7)
  uint8_t armUpDeg = 150, armDownDeg = 40;       // servo nâng (PA6)
  uint16_t gripStepMs = 450;
};

}  // namespace core
