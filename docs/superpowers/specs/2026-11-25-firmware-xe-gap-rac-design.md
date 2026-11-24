# Thiết kế firmware xe dò line – phát hiện & gắp rác đỏ

Ngày: 2026-11-25 · Trạng thái: chủ sở hữu giao việc chạy tự động qua đêm (bỏ vòng hỏi đáp);
mọi điểm đánh dấu **[Claude quyết]** là lựa chọn của người xây dựng, cần chủ sở hữu xem lại.

## 1. Mục đích (intent)
Đội thi Cuộc thi Robot SV 2026 "KMA_Môi trường xanh" cần một bộ firmware nạp được ngay vào xe
STM32F103C8T6 hiện có, để:
- Vòng 1: dò line, đi qua toàn bộ sân 300×300 cm, báo đúng mọi vật đỏ (≥ 2 s, không lặp,
  không báo vật nhiễu), về ĐÍCH trong 5 phút.
- Vòng 2: cùng nền tảng, thêm khung điều khiển cơ cấu gắp (tùy chọn biên dịch).
Kèm tài liệu hướng dẫn + báo cáo chi tiết, viết cho người mới.

## 2. Mục tiêu / không mục tiêu / phản mục tiêu
- Mục tiêu: phủ 12/12 cung; ≥ 95 % phát hiện đỏ; 0 báo nhầm; ≤ 3 phút; chạy được khi mất định vị
  (fallback); kiểm thử được trên PC trước khi nạp.
- Không mục tiêu: thay đổi phần cứng bắt buộc; camera/AI; điều khiển từ xa.
- Phản mục tiêu: firmware "đẹp" nhưng kém tin cậy hơn bản .ino hiện tại trên các hành vi đã được
  tinh chỉnh thật (quay theo cảm biến + phanh ngược, ngưỡng màu đã thử). Các hằng số đã đo thật
  được giữ làm giá trị mặc định.

## 3. Ràng buộc phần cứng (nguồn: 4 file .md + schematic v3.0)
| Khối | Chân | Ghi chú |
|---|---|---|
| L298N | ENA PA11, IN1 PA5, IN2 PA4, ENB PA9, IN3 PA3, IN4 PA2 | PWM TIM1 |
| Line 5 mắt (bán nguyệt) | L2 PB11, L1 PB12, M PB13, R1 PB14, R2 PB15 | BLACK = 1 |
| E18-D80NK | trái PA8, phải PA10 (INPUT_PULLUP) | HIGH = có vật |
| TCS3200 (trái) | S2 PB0, S3 PB1, OUT PB10; S0=5V, S1=GND | đo chu kỳ (pulseIn) |
| TCS34725 (phải) | I2C1 SCL PB6, SDA PB7, 0x29 | LED cam bật sẵn |
| Còi | PA1 → R 1k → NPN S8050 (schematic) | HIGH = kêu |
| LED | PC13 | LOW = sáng |
| Nguồn | 3S 11,1–12,6 V → cầu chì 2 A → LM2596 → 5 V | |

Sai lệch schematic so với tài liệu (đã ghi trong báo cáo): nhãn I2C1_SCL ở PB8 (PB8+PB7 không phải
cặp I2C hợp lệ), chân INT của TCS34725 nối vào I2C1_SDA, IN1 lặp ở PA5/PA6, OBS_L/ENB cùng PA9.
**[Claude quyết]** firmware theo pinout trong tài liệu (đã chạy thật); có cờ `I2C_REMAP_PB8_PB9`.

Cảm biến line xếp bán nguyệt: mắt giữa nhô xa nhất, mắt ngoài lùi gần trục bánh → khi qua vạch ngang
các mắt chạm đen lần lượt M → L1/R1 → L2/R2 chứ không đồng thời. Vì vậy nút giao được phát hiện bằng
**tích lũy trong cửa sổ thời gian**, không đòi hỏi mọi mắt đen cùng lúc.

## 4. Các cách tiếp cận đã cân nhắc
1. **Sửa dần file .ino hiện tại** – rủi ro thấp, nhưng không giải quyết phủ sân/khử lặp, khó kiểm thử.
2. **Lõi logic thuần C++ độc lập phần cứng + lớp HAL Arduino + trình mô phỏng trên PC** – kiểm thử
   được toàn bộ nhiệm vụ trước khi nạp; tách thuật toán khỏi chân. Chi phí: nhiều code hơn.
3. **Viết lại bằng STM32 HAL/CubeMX + FreeRTOS** – mạnh nhưng đội đang dùng Arduino, đường học dốc,
   không kiểm thử được trên PC dễ dàng.

**[Claude quyết] Chọn (2)**: PlatformIO + Arduino (STM32duino) để đội tiếp tục dùng công cụ quen,
lõi `lib/core` không phụ thuộc Arduino, trình mô phỏng `sim/` chạy cùng lõi đó.

## 5. Thiết kế đã chốt
- **Kiến trúc**: `core::Robot` (FSM) gọi giao diện `core::Hal` (đọc cảm biến, đặt động cơ, thời gian,
  còi/LED, log, lưu trữ). `ArduinoHal` cho xe thật, `SimHal` cho mô phỏng. Vòng lặp non-blocking,
  tick 1–2 ms; riêng lúc xe đứng yên quét màu được phép chặn ngắn (pulseIn ≤ 3 ms/kênh).
- **Bám line**: PID trên vị trí ngang (cm) tính từ tọa độ thật của 5 mắt; gần nút chỉ dùng 3 mắt giữa
  để lái (chống loa bo kéo lệch); giữ dấu khi mất line.
- **Nút giao**: cổng quãng đường (chỉ xét khi đã đi ≥ 50 % cung), ứng viên khi mắt ngoài đen ≥ 3 mẫu
  liên tiếp → bò thẳng `SCAN_TIME` tích lũy trái/phải → xét còn lối thẳng → loại nút.
- **Quay**: theo sự kiện (rời line cũ → tìm line mới ở tốc độ thấp) + thời gian quay tối thiểu (bỏ
  qua cung loa bo) + phanh ngược 25 ms + timeout.
- **Bản đồ & lộ trình**: đồ thị 9 nút/12 cung; lộ trình phủ tối ưu 16 đoạn tính sẵn
  (H→E→B→A→D→E→F→C→B→E→F→I→H→G→D→A); đối chiếu loại nút với bản đồ; lệch ≥ 2 lần → chế độ
  dự phòng "bám phải + phá vòng lặp" như bản cũ.
- **Thời gian**: quá `GO_HOME_MS` (240 s) → BFS đường ngắn nhất về A.
- **Màu**: 6 mẫu, bỏ phiếu ≥ 4/6. Mặc định dùng ngưỡng đã thử thật (TCS3200: r<450, r<0,55g,
  r<0,55b; TCS34725: c>80, r>1,10g, r>1,45b). Sau khi CALIB, dùng phân loại tâm màu chromaticity
  (lưu EEPROM giả lập Flash).
- **Khử lặp**: khóa (cung, bên, đoạn 30 cm, dung sai ±12 cm); quãng đường ước lượng bằng
  v̂·thời gian chạy (v̂ tự học từ các cung 150 cm).
- **Cảnh báo**: còi + LED liên tục 2 200 ms, xe đứng yên.
- **Restart thông minh**: lưu cung đã phủ + vật đã báo vào thanh ghi Backup (giữ qua nút RESET,
  mất khi tắt nguồn); sau restart lập lộ trình tham lam chỉ đi cung chưa phủ rồi về A.
- **Khởi động**: [Claude quyết] nút RESET của Blue Pill = nút START (thể lệ cho phép nút nhấn);
  xe chờ 1 s rồi chạy. Chọn chế độ khi bật bằng cách che E18: trái = CALIB, phải = TEST,
  cả hai = xóa bộ nhớ restart.
- **Debug**: [Claude quyết] UART mềm chỉ-TX trên PB5 (không đụng PB8 vì schematic gán I2C ở đó).
- **Vòng 2 (gắp)**: [Claude quyết] tắt mặc định (`ENABLE_GRIPPER 0`); 2 servo (nâng PA6, kẹp PA7),
  chuỗi: dừng → mở kẹp → hạ → kẹp → nâng → đếm; số rác báo bằng số tiếng bíp khi về đích.
  Cơ cấu cơ khí chưa có nên góc servo là tham số cần hiệu chỉnh.

## 6. Kiểm thử
- Unit test lõi (PID, vị trí line, nút giao, bản đồ/lộ trình/BFS, phân loại màu, khử lặp, cảnh báo,
  backup) chạy trên PC bằng `zig c++` (không cần cài MinGW).
- Mô phỏng động học 2D toàn sân: đường line + loa bo, xe vi sai có quán tính, 5 mắt bán nguyệt,
  E18 và cảm biến màu, vật ngẫu nhiên (8 đỏ + nhiễu). Chạy nhiều seed, đo: phủ cung, đỏ đúng,
  báo nhầm, báo lặp, về đích, thời gian. Xuất SVG quỹ đạo.
- Biên dịch chéo cho `bluepill_f103c8` bằng PlatformIO để chắc chắn nạp được.

## 7. Để lại cho người xây dựng / đội
Giá trị PID, vận tốc ước lượng, góc servo, khoảng cách cảm biến thật cần đo trên xe (có chế độ
TEST hỗ trợ). Cách xác định "về đích" cần hỏi BTC (cờ `GOAL_STOP_AT_NODE`).
