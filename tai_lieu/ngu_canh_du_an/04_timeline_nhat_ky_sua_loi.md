# TIMELINE HOÀN THÀNH & NHẬT KÝ SỬA LỖI

## 1. Các mốc tiến độ đã hoàn thành

### Giai đoạn 1: Dựng nền tảng phần cứng & Nguồn
* Phục hồi thành công cell pin 18650 bị tụt áp ($1.36\text{ V}$) và hoàn thiện pack nguồn 3S (hiện đạt $11.58\text{ V}$).
* Hoàn tất đấu nối 4 động cơ qua L298N, 5 mắt dò line TCRT5000, 2 cảm biến E18-D80NK, còi `PA1`, TCS3200 (chuyển `OUT` sang `PB10` an toàn $5\text{ V}$) và TCS34725 qua I2C (`PB6`/`PB7`).

### Giai đoạn 2: Xây dựng luồng phát hiện vật cản & Kiểm tra còi
* Triển khai cơ chế dùng 2 mắt E18-D80NK làm cảm biến kích hoạt trước khi đọc màu hai bên sườn.
* Xác nhận phần cứng còi `PA1` và cả 2 cảm biến màu hoạt động ổn định thông qua test độc lập và test trực tiếp với vật mẫu.

### Giai đoạn 3: Khắc phục lỗi bắt màu & Giảm độ trễ
* Sửa lỗi thử vật bằng tay không dừng và lỗi còi kêu ngắt quãng khi xe vừa chạy vừa kêu.
* Khắc phục triệt để lỗi nhận nhầm màu trên **TCS3200** (giơ tay hoặc đưa hộp sữa xanh lá đều kêu còi).
* Tối ưu tốc độ phản hồi và độ nhạy cho **TCS34725** sử dụng LED màu cam.

### Giai đoạn 4: Chuyển đổi chiến lược điều hướng
* Tinh chỉnh thời gian quét nút giao để thoát khỏi hiện tượng kẹt/đi thẳng tại ngã tư có loa bo góc.
* Thử nghiệm cấu trúc kịch bản cố định (lộ trình 5 bước và lộ trình 13 bước quét toàn bản đồ) nhưng phát hiện lỗi xoay lố góc khi pin đầy và tính rủi ro cao nếu trượt 1 nút giao.
* **Quyết định chốt phương án mới:** Chuyển hẳn sang **thuật toán tự dò đường độc lập** (bám phải, phá vòng lặp, tự quay đầu ngõ cụt, phanh ngược khi xoay compa).

---

## 2. Nhật ký các lỗi đã xử lý (Root Cause Analysis)

| Hiện tượng lỗi | Nguyên nhân gốc rễ | Cách đã xử lý trong code / phần cứng |
| :--- | :--- | :--- |
| Đưa vật đỏ vào trước mũi xe khi nhấc lên thì xe vẫn chạy tiếp | 1) E18 và cảm biến màu đặt **hai bên sườn**, không phải trước mũi.<br>2) Bẫy `ignoreObstacleUntil = 1000ms` kích hoạt sớm khi vật mới ở cự ly $7\text{ cm}$ (ngoài tầm đọc màu $1.5\text{--}3\text{ cm}$).<br>3) Tuột dây `OUT` (`PB10`) của TCS3200. | Cắm chặt lại `PB10`; dừng bánh và quét màu liên tiếp nhiều lần trước khi kết luận; rút ngắn thời gian khóa xuống $400\text{--}450\text{ ms}$. |
| Còi lúc chạy kêu nhỏ hoặc không kêu rõ | Khi 4 động cơ quay cùng lúc gây sụt áp nguồn pin cấp cho chân I/O `PA1`. | Phanh dừng hẳn 4 động cơ `drive(0, 0)` trước khi xuất xung bật còi `PA1`. |
| TCS3200 nhận nhầm bàn tay và hộp sữa xanh lá là màu đỏ | 1) Giắc `S2` (`PB0`) lỏng khiến bộ lọc kẹt ở mức `LOW` (đọc nhầm kênh Blue thay cho Green).<br>2) Vỏ hộp bóng phản xạ tia IR lọt vào kênh Red (TCS3200 không có kính lọc IR) và da tay phản xạ đỏ mạnh. | Siết chặt giắc `PB0`; thêm `delayMicroseconds(200)` ổn định bộ lọc; thêm điều kiện $r < 450$ và siết tỷ lệ xuống $r < g \times 0.55$ và $r < b \times 0.55$. |
| TCS34725 mất vài giây mới nhận được vật màu đỏ | 1) Thời gian tích phân mặc định dài ($50\text{ ms}$).<br>2) LED tích hợp trên mạch là **LED cam** làm lọt sáng sang kênh Green, kéo tỷ lệ $R/G$ xuống dưới ngưỡng $1.18$ cũ. | Giảm thời gian tích phân xuống $\approx 24\text{ ms}$ (`0xF6`); tăng Gain lên $60\times$ (`0x03`); đặt lại ngưỡng riêng cho LED cam: $c > 80$, $r > g \times 1.10$, $r > b \times 1.45$. |
| Vào ngã tư đầu tiên xe quay phải quá mạnh làm chệch khỏi line | `delay(220)` xoay mù ở tốc độ $145$ làm xe văng quá $95^\circ\text{--}105^\circ$ trước khi kịp tìm line mới, cộng thêm quán tính khi pin $11.58\text{ V}$. | Bỏ xoay mù thời gian dài; chuyển sang xoay dựa vào trạng thái cảm biến thoát vạch cũ, sau đó giảm tốc tìm vạch mới và phanh ngược $25\text{ ms}$. |
| Xe cứ lao thẳng qua ngã tư | Ưu tiên `if (coThang)` đứng đầu kết hợp `SCAN_TIME_MS = 75ms` quá ngắn làm mắt mũi còn đè lên vạch ngang của ngã tư. | Tăng `SCAN_TIME_MS` lên $135\text{ ms}$, siết điều kiện kích hoạt nút giao và chuyển sang thuật toán tự dò Bám phải + Phá vòng lặp. |