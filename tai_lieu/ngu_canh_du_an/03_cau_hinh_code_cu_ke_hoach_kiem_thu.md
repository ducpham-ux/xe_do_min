# CẤU HÌNH CODE HIỆN TẠI & KẾ HOẠCH KIỂM THỬ

## 1. Kiến trúc logic phần mềm mới nhất

### Luồng xử lý trong `loop()`
1. **Kiểm tra vật cản (`PA8` / `PA10`):**
   * Khi có vật và `millis() > ignoreObstacleUntil`: dừng xe ngay lập tức (`drive(0, 0)`).
   * Quét tối đa 6 lần, mỗi lần cách nhau $35\text{ ms}$ ($\approx 210\text{ ms}$). Ngắt vòng lặp ngay khi phát hiện màu đỏ.
   * **Nếu thấy Đỏ:** Kêu còi `PA1` + nháy LED `PC13` 3 nhịp ($200\text{ ms}$ bật / $200\text{ ms}$ tắt), sau đó khóa bỏ qua $450\text{ ms}$ (`ignoreObstacleUntil = millis() + 450`) để lướt qua vật vừa báo mà vẫn bắt được trụ màu nằm kế bên.
   * **Nếu lỗi I2C bên phải (`checkRight_TCS34725() == -1`):** Nháy chậm `PC13` chu kỳ $400\text{ ms}$ để cảnh báo tuột dây `PB6`/`PB7`.
   * **Nếu không phải Đỏ:** Khóa bỏ qua $400\text{ ms}$ để đi tiếp.
2. **Xử lý hết vạch đen (Toàn `WHITE`):**
   * Xác nhận lại sau $60\text{ ms}$ để tránh khe hở băng dính.
   * Nếu thời gian chạy $< 45\text{ giây}$ (`MIN_EXPLORE_MS = 45000`): hiểu là đi nhầm ngõ cụt (*VÒNG 1*) hoặc trượt line -> gọi `quayDau()` xoay $180^\circ$.
   * Nếu $\ge 45\text{ giây}$: dừng hẳn tại *ĐÍCH*.
3. **Phát hiện nút giao thật:**
   * Chỉ gọi `xuLyNutGiao()` khi mắt ngoài (`L2` hoặc `R2`) đọc `BLACK` **đồng thời** với cụm mắt giữa (`M`, `L1`, `R1`), kèm chống lặp trong $450\text{ ms}$ (`lastNodeTime`).
   * Nếu chỉ có `L2` hoặc `R2` chạm đen đơn lẻ -> nắn lái gắt (`-65, 125` hoặc `125, -65`) mà không kích hoạt quay compa.
4. **Quy tắc rẽ tự động tại ngã ba / ngã tư:**
   * Góc vuông $90^\circ$ duy nhất -> tự động rẽ theo lối mở.
   * Ngã ba / ngã tư -> ưu tiên **Phải (`1`) -> Thẳng (`0`) -> Trái (`-1`)**.
   * **Phá vòng lặp:** Theo dõi `lastTurnDir` và `sameTurnCount`. Nếu rẽ cùng 1 chiều 2 lần liên tiếp (`sameTurnCount >= 2`), tới nút thứ 3 bắt buộc cấm rẽ hướng đó để ép xe chuyển sang vùng sân khác.

## 2. Bảng thông số đã tinh chỉnh (Cho mức pin ~11.6V)

| Nhóm | Hằng số / Tham số | Giá trị hiện tại | Ý nghĩa & Dải tinh chỉnh |
| :--- | :--- | :--- | :--- |
| **Tốc độ chạy** | `SPEED_BASE` | `115` | Tốc độ bám line đường thẳng |
| **Ôm cua nhẹ** | `SPEED_CURVE_OUT` / `IN` | `128` / `45` | Bù lệch line khi mắt `L1` hoặc `R1` chạm vạch |
| **Quét nút giao** | `SPEED_SCAN` / `SCAN_TIME_MS` | `95` / `135` | Thời gian bò vào tâm nút (chỉnh trong dải $120\text{--}155\text{ ms}$) |
| **Xoay compa** | `SPEED_TURN` / Phanh ngược | `110` / `25 ms` | Tốc độ thoát vạch cũ; xoay tìm line mới ở tốc độ `85` |
| **Thời gian dò** | `MIN_EXPLORE_MS` | `45000` ($45\text{ s}$) | Ngưỡng phân biệt ngõ cụt cần quay đầu với ô ĐÍCH |
| **TCS3200 (Trái)** | `isRedLeft_TCS3200()` | `delay 200us`, `r<450`, tỷ lệ `<0.55` | Lọc đỏ đậm, loại da tay và vật xanh lá |
| **TCS34725 (Phải)** | `0x01=0xF6`, `0x0F=0x03` | Tích phân $24\text{ ms}$, Gain $60\times$ | Điều kiện nhận đỏ: `c>80`, `r>g*1.10`, `r>b*1.45` |

## 3. Checklist việc cần làm tiếp theo trên sân thật
- [ ] **Sạc cân bằng pin 3S:** Đưa 2 cell đang ở mức $3.81\text{ V}$ và $3.79\text{ V}$ lên bằng cell $3.98\text{ V}$ (hoặc đồng đều mức $4.05\text{--}4.15\text{ V}$) trước giờ chạy chính thức.
- [ ] **Chạy thử vòng dò line không vật cản:** Đặt xe chạy qua các ngã tư có loa bo góc để tinh chỉnh `SCAN_TIME_MS` (hiện là `135`; tăng lên `150` nếu bánh dừng non trước vạch ngang, giảm xuống `115` nếu lao quá tâm).
- [ ] **Đo thời gian thực tế hoàn thành vòng sân:** Điều chỉnh lại `MIN_EXPLORE_MS` (hiện $45000\text{ ms}$) khớp với thời gian thực tế xe cần để quét xong bản đồ và về ô *ĐÍCH*.
- [ ] **Chỉnh biến trở sau đuôi 2 mắt E18-D80NK:** Vặn tầm bắt khoảng $10\text{--}12\text{ cm}$ để bắt trụ đặt sát vạch đen nhưng không nhận nhầm thành biên sân.
- [ ] **Làm vành che sáng cho cảm biến màu:** Cắt Fomex hoặc bìa đen làm vành nhô ra $1\text{--}1.5\text{ cm}$ quanh mặt TCS3200 và TCS34725 để chống đèn trần chiếu xiên gây lóa (giữ nguyên LED tích hợp trên cả 2 cảm biến).

## 4. Hướng dẫn nạp lại ngữ cảnh cho phiên chat mới
Khi bắt đầu một cuộc trò chuyện mới, bạn chỉ cần đính kèm 4 file `.md` này (hoặc dán nội dung file `02` và `04`) kèm theo đoạn code `.ino` hiện tại đang dùng là trợ lý AI sẽ nắm toàn bộ cấu hình để hỗ trợ tiếp ngay lập tức.