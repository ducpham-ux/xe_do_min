// params.h - TOÀN BỘ tham số tinh chỉnh của xe nằm ở đây.
// Giá trị có ghi "(đã đo thật)" lấy từ bản .ino đã chạy trên xe; các giá trị khác là
// điểm xuất phát, cần tinh chỉnh trên sân (xem Hướng dẫn, mục Tinh chỉnh).
#pragma once
#include <stdint.h>

namespace core {

struct Params {
  // ---------- Thanh dò line (5 mắt xếp bán nguyệt) ----------
  // Tọa độ ngang của từng mắt so với tâm xe (cm), thứ tự L2, L1, M, R1, R2.
  // Âm = bên trái. Mắt nằm trên cung tròn bán kính 5 cm, góc -60/-30/0/30/60 độ.
  float sensorY[5] = {-4.33f, -2.5f, 0.0f, 2.5f, 4.33f};
  float lostPos = 6.0f;           // vị trí giả định khi mất line (cm), giữ dấu lần cuối

  // ---------- Bám line PID ----------
  float kp = 26.0f;               // PWM / cm
  float ki = 0.0f;                // PWM / (cm*s)
  float kd = 1.6f;                // PWM / (cm/s)
  float iLimit = 40.0f;           // kẹp thành phần tích phân (PWM)
  int16_t baseSpeed = 140;        // tốc độ nền trên đoạn thẳng
  int16_t approachSpeed = 110;    // tốc độ khi gần nút dự kiến
  float approachFrac = 0.80f;     // giảm tốc khi đã đi 80% cung
  int16_t maxPwm = 230;

  // ---------- Nút giao ----------
  uint8_t nodeConfirmSamples = 3; // số mẫu liên tiếp để thành ứng viên nút
  float nodeGateFrac = 0.50f;     // chỉ xét nút khi đã đi >= 50% chiều dài cung
  float edgeLenCm = 150.0f;       // chiều dài cung giữa 2 nút
  float startStubCm = 30.0f;      // nhánh XUẤT PHÁT -> nút H (ước lượng)
  int16_t scanSpeed = 95;         // (đã đo thật)
  uint16_t scanTimeMs = 135;      // (đã đo thật) bò vào tâm nút, chỉnh 120-155

  // ---------- Quay ----------
  int16_t turnSpeed = 110;        // (đã đo thật)
  int16_t turnSlowSpeed = 85;     // (đã đo thật) tốc độ tìm line mới
  uint16_t turnMinMs = 200;       // bỏ qua line trong thời gian này (tránh cung loa bo)
  uint16_t uturnMinMs = 550;      // quay đầu: thời gian tối thiểu trước khi nhận line
  uint16_t turnTimeoutMs = 1800;
  uint16_t brakeMs = 25;          // (đã đo thật) phanh ngược
  int16_t brakePwm = 110;

  // ---------- Ước lượng vận tốc (không có encoder) ----------
  float vBaseCmS = 40.0f;         // vận tốc ước lượng ở baseSpeed (đo bằng chế độ TEST)
  float vLearnAlpha = 0.3f;       // tốc độ học từ thời gian chạy hết cung
  bool adaptSpeed = true;         // tự chỉnh PWM để giữ vận tốc mục tiêu khi pin thay đổi
  float adaptMin = 0.85f, adaptMax = 1.20f;

  // ---------- Vật cản E18 + màu ----------
  uint8_t obstacleDebounce = 3;   // mẫu (1 ms) liên tiếp
  float obstacleRearmCm = 8.0f;   // sau khi xử lý 1 vật, bỏ qua bên đó thêm 8 cm
  uint8_t colorSamples = 6;       // (đã đo thật) 6 mẫu x 35 ms
  uint8_t colorNeed = 4;          // cần >= 4/6 mẫu đỏ
  uint16_t colorGapMs = 35;       // (đã đo thật)
  uint16_t settleMs = 30;         // chờ xe đứng yên trước khi đọc màu
  // Ngưỡng mặc định (đã đo thật)
  uint16_t t3200MaxR = 450;       // chu kỳ kênh đỏ (us) phải nhỏ hơn
  float t3200Ratio = 0.55f;       // r < g*ratio && r < b*ratio
  uint16_t t34725MinC = 80;
  float t34725RG = 1.10f, t34725RB = 1.45f;
  // Khi đã hiệu chỉnh (CALIB): bán kính và biên tin cậy trong không gian chromaticity
  float calibRadius = 0.07f, calibMargin = 0.02f;

  // ---------- Khử lặp ----------
  float binCm = 30.0f;            // đoạn 30 cm
  float dupTolCm = 12.0f;         // coi là cùng vật nếu lệch < 12 cm

  // ---------- Cảnh báo ----------
  uint16_t alertMs = 2200;        // thể lệ: >= 2 giây

  // ---------- Nhiệm vụ ----------
  uint16_t startDelayMs = 1000;   // sau khi nhấn RESET/START
  uint32_t goHomeMs = 240000;     // quá 4:00 -> về đích đường ngắn nhất
  uint32_t matchMs = 300000;      // 5 phút: dừng hẳn
  uint8_t mismatchToFallback = 2; // nút đo sai bản đồ bao nhiêu lần thì sang chế độ dự phòng
  uint32_t fallbackMinExploreMs = 45000;  // (đã đo thật) chế độ dự phòng: trắng sau 45 s = ĐÍCH
  uint16_t lostConfirmMs = 60;    // (đã đo thật) xác nhận toàn trắng
  uint16_t recoverTimeoutMs = 3000;
  bool goalStopAtNode = true;     // ĐÍCH = dừng ở nút góc A (hỏi BTC)
  float goalCreepCm = 0.0f;       // nếu phải đi vào ô ĐÍCH: tiến thêm bao nhiêu cm

  // ---------- Vòng 2: gắp ----------
  bool enableGripper = false;
  uint8_t gripOpenDeg = 90, gripCloseDeg = 20;   // servo kẹp (PA7)
  uint8_t armUpDeg = 150, armDownDeg = 40;       // servo nâng (PA6)
  uint16_t gripStepMs = 450;
};

}  // namespace core
