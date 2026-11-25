# Thay đổi firmware sau báo cáo test lần 1

Ngày: 2026-11-25 · Nhánh: `feat/bao-cao-lan-1`

## 1. Báo cáo test lần 1 → việc đã làm

| Hiện tượng / yêu cầu | Nguyên nhân | Đã sửa |
|---|---|---|
| Xe chạy quá nhanh, mất line, tìm lại lâu | Tốc độ nền 140; khi mất line xe đi thẳng 60 ms rồi quét trái–phải từ từ | Tốc độ nền **105**, gần nút **85**, giảm thêm tới 35% khi line lệch mép thanh, tăng tốc mềm sau mỗi lần dừng/quay. Mất line ở mép → **quay ngay về phía vừa mất** (thanh cách tâm quay 12 cm nên chỉ vài độ là thấy lại). PID có lọc khâu D. |
| Hình học mắt dò sai | Code cũ giả định 5 mắt xếp bán nguyệt | Thanh **thẳng 7 cm** (mắt cách 1,75 cm), cách tâm quay **12 cm**, line **2,8 cm**. |
| Ngã tư có đường bo 45° gây nhiễu, chỉ rẽ ở góc vuông | Cung bo làm dải đen phình/tách ở mép thanh, giống vạch ngang | Xem mục 2. |
| Bỏ TCS34725 | Không phát hiện màu trong môi trường test | Đã bỏ hẳn (HAL, I2C, CALIB). PB6/PB7 để trống. |
| E18 7 cm, TCS3200 trái 1,5 cm: quay tại chỗ tiến lại gần vật rồi đọc màu | TCS3200 chỉ đọc được rất gần | Xem mục 3. |
| Không đi nhầm đường lại | Lệch bản đồ 2 lần là rơi về "bám phải" | Xem mục 4. |

## 2. Dò line và nút giao (thanh thẳng 5 mắt)

- **Dải đen liền**: mỗi mẫu được tách thành các dải đen liền. Line thật (2,8 cm) chỉ phủ 1–2 mắt; dải rộng hơn là line + cung bo hoặc vạch ngang.
- **Vùng cung bo** (gần B, E, H — biết từ bản đồ): chỉ lái khi mẫu "sạch" (1 dải, ≤ 2 mắt). Mẫu phình/tách thì giữ thẳng hướng, đóng băng PID cho tới khi qua vùng. Đây là thay đổi quyết định trong mô phỏng: trước đó xe hay bị cung bo kéo sang nhánh khác.
- **Nhận vạch ngang thật**: dải liền từ M ra tới mắt ngoài, dài ≥ 1,6 cm theo hướng đi (≥ 2,3 cm nếu ngay trước đó line phình 3 mắt). Ở B/E/H còn phải **thấy cung bo tách khỏi line trước** (mẫu `L2 . M . R2`) — cung bo luôn tách trước khi tới vạch ngang thật. Nếu đã đi quá vị trí dự kiến 4 cm mà chưa thấy mẫu tách thì bỏ điều kiện này (an toàn).
- **Rẽ đúng tâm**: sau vạch ngang bò thêm 2 cm xét lối thẳng; nếu rẽ thì bò tiếp cho **tâm quay vào giữa nút** (12 − 1,4 cm) rồi mới quay tại chỗ.
- **Quay**: quay nhanh tối thiểu **62°** (90°) / **140°** (quay đầu) — bỏ qua cung bo 45° — rồi quay chậm tới khi M nằm giữa một dải hẹp, phanh ngược 25 ms. Tốc độ quay (độ/s) **tự học** sau mỗi lần rẽ.

## 3. Đọc màu vật thể (chỉ TCS3200 sườn trái)

**Vật bên TRÁI** (E18 trái kích):
1. Dừng, tiến `alignCm` = 2 cm (E18 bắt mép vật, tâm vật còn ở phía trước).
2. Đọc TCS3200 3 lần: thấy **RÕ** (kênh nhỏ nhất < 450 µs) → đọc màu; thấy **MỜ** (< 600 µs) → dịch nửa bước; không thấy → dịch một bước.
3. Một bước dịch ngang = quay trái 15° → tiến 8 cm → quay phải 15° → lùi 7,7 cm: dịch sang trái ~2,1 cm, không tiến/lùi. Tối đa 4 bước.
4. Đọc 7+ mẫu (đứng yên rồi bò chậm 3 cm dọc vật). Đỏ khi ≥ 4 mẫu đỏ **và** ≥ 60% số mẫu.
5. Đỏ và chưa báo → dừng, còi + LED **2,2 s**. Rồi đảo ngược từng bước dịch ngang để về line.

**Vật bên PHẢI**: không dừng (không có cảm biến màu bên phải). Xe ghi nhớ vị trí vật và đặt nhu cầu **đi ngược cung đó** sau này để vật nằm bên trái. Lộ trình được lập lại tham lam (cung chưa đi + cung cần đi ngược), về đích khi tới 4:05.

**Bản đồ vật thể** trong MCU: mỗi vật lưu (cung, bên của cung, vị trí dọc cung, loại Đỏ / Khác / Chờ đọc, lượt đi). Vật đã đọc ở lượt trước thì bỏ qua (không dừng lại); vật sát nhau trong cùng lượt luôn tách riêng (cụm vàng–đỏ–xanh như sơ đồ).

## 4. Tự định vị trên bản đồ

Xe giữ tập mọi vị trí (nút, hướng) còn khớp với chuỗi hình dạng nút đã đo. Khi nút đo được không khớp vị trí đang tin:
- không khớp giả thuyết nào → coi là đo nhiễu (lần 1), dựng lại tập giả thuyết (lần 2);
- còn giả thuyết khớp → chọn giả thuyết gần vị trí ước lượng nhất, **lập lại lộ trình** từ đó;
- gặp ngõ cụt → chỉ có thể là nhánh VÒNG 1 (B) hoặc XUẤT PHÁT (H).

Chế độ "bám phải" cũ chỉ còn dùng khi mất định vị quá 4 lần.

## 5. Kết quả kiểm tra (chưa chạy trên xe thật)

- Unit test: **69/69** đạt (`bash tools/run_host_tests.sh`).
- Biên dịch chéo `bluepill` và `bluepill_gripper`: đạt (Flash ~90%).
- Mô phỏng (xe 26×16, thanh 12 cm, line 2,8, cung bo R 15/20/25 cm):

| Kịch bản | Về đích | Bắt đỏ | Báo nhầm | Ghi chú |
|---|---|---|---|---|
| Không vật, 30 lượt × 3 bán kính cung bo | 90/90 | – | – | phủ đủ 12 cung 90/90, ~100 s/lượt |
| 50 lượt, 8 đỏ + 16 nhiễu, khe 1–7 cm | 50/50 | **80,8%** | 0 | 1 báo lặp, trung vị 271 s, `docs/ket_qua_mo_phong/test_lan1_50_luot.txt` |
| + vật cam, nhiễu line ×3, có CALIB (20 lượt) | 18/20 | 78,8% | 1 | |

## 6. Giới hạn còn lại (cần đội biết)

1. **Thời gian sát 5 phút.** Mỗi vật bên trái tốn ~3–6 s (dịch ngang ra – vào); vật bên phải phải đi ngược cả cung. Trung vị 271 s, xe về đích bằng lệnh "gần hết giờ" ở 4:05 nên ~20% vật đỏ bị bỏ lỡ vì hết giờ.
2. **Va chạm với vật kế bên trong cụm** (~1,7 lần/lượt trong mô phỏng): cảm biến màu ở giữa sườn, mũi xe dài thêm 13 cm; khi dịch ngang để đọc vật này, mũi xe sát vào vật kế tiếp nếu vật đó gần line hơn.
3. Theo thể lệ vật đặt cách line 5 ± 2 cm, nhưng nửa bề ngang xe là 8 cm — nếu sân thật đặt đúng như vậy, thân xe sẽ chạm vật. Nên đo lại khoảng cách thật trên sân.

**Khuyến nghị phần cứng** (hiệu quả hơn mọi tinh chỉnh firmware):
- Lắp **thêm 1 TCS3200 bên phải** → bỏ được toàn bộ các lượt đi ngược.
- Đưa TCS3200 ra **ngoài sườn** (giá đỡ nhô ra 3–5 cm, hoặc thêm ống che sáng/thấu kính) để đọc được ở 4–6 cm → hầu như không phải dịch ngang, hết va chạm.

## 7. Phải đo trên xe trước khi chạy

| Tham số (`params.h`) | Cách đo |
|---|---|
| `MOTOR_*_INVERT` (`pins.h`) | TEST: bánh nào quay ngược thì đổi |
| `vBaseCmS` | TEST: che E18 trái 3 s → xe chạy 2 s, đo quãng đường / 2 |
| `turnRateDegS` | TEST: che E18 phải 3 s → xe quay trái 1 s, đo góc quay (độ) |
| `sensorAheadCm` (12) | Thước: thanh mắt → điểm giữa trục trước và trục sau |
| `presenceMaxUs`, `nearMaxUs` | TEST: đặt vật ở 2,5 cm và 1,5 cm trước TCS3200, đọc kênh nhỏ nhất; cột `thay=` phải ra `MO` / `RO` |
| `t3200MaxR`, `t3200Ratio` hoặc CALIB | TEST với vật đỏ/cam/vàng thật; hoặc chạy CALIB (che E18 trái khi bật: 4 lớp Đỏ, Vàng, Xanh, Tím, đưa mẫu sát TCS3200 tới khi bíp) |
| `sideStepDeg`, `sideStepCm` | Quan sát 1 bước dịch ngang: xe phải dịch sang trái ~2 cm và thẳng hướng lại |
| `minCrossCm` | Nếu xe bỏ sót nút: giảm 0,2; nếu rẽ ở cung bo: tăng 0,2 |

Tài liệu Word (`tai_lieu/*.docx`) vẫn mô tả bản trước test lần 1 — dùng file này làm bản cập nhật.
