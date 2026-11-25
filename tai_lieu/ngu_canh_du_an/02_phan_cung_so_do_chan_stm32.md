# CHI TIẾT PHẦN CỨNG & SƠ ĐỒ CHÂN STM32

## 1. Danh sách linh kiện chính
* **MCU:** STM32F103C8T6 (Blue Pill), LED trạng thái `PC13` (tích cực mức `LOW`).
* **Hệ thống truyền động:** 4 động cơ DC kết hợp mạch cầu H L298N.
* **Cảm biến dò line:** Thanh 5 mắt hồng ngoại TCRT5000 (`BLACK = 1`, `WHITE = 0`).
* **Cảm biến vật cản 2 bên sườn:** 2x E18-D80NK (loại thường đóng NC, kích hoạt mức `HIGH` khi có vật).
* **Cảm biến màu bên trái:** Module TCS3200 (tích hợp 4 LED trắng, không có kính lọc hồng ngoại IR).
* **Cảm biến màu bên phải:** Module TCS34725 giao tiếp I2C (địa chỉ `0x29`, bản mạch tích hợp sẵn **LED màu cam** bước sóng $\approx 600\text{--}610\text{ nm}$).
* **Thiết bị báo hiệu:** Còi chip chủ động (loại 5V như TMB12A05).
* **Nguồn nuôi:** Pack pin 3S Li-ion 18650 (điện áp danh định $11.1\text{ V}$, sạc đầy $4.2\text{ V/cell}$).

## 2. Bảng sơ đồ nối chân (Pinout)

### Động cơ & Tín hiệu báo động
| Chức năng | Chân thiết bị | Chân STM32 / Nguồn | Ghi chú |
| :--- | :--- | :--- | :--- |
| Điều khiển tốc độ trái | `ENA` (L298N) | `PA11` | PWM |
| Chiều quay động cơ trái | `IN1` / `IN2` | `PA5` / `PA4` | Digital Output |
| Chiều quay động cơ phải | `IN3` / `IN4` | `PA3` / `PA2` | Digital Output |
| Điều khiển tốc độ phải | `ENB` (L298N) | `PA9` | PWM |
| Còi báo động chủ động | Chân `+` / Chân `-` | `PA1` / `GND` | Chỉ kích khi xe đã dừng để tránh sụt áp |

*(Phương án dự phòng cho còi nếu dùng pin yếu: chuyển chân `-` còi sang `PB9` chịu áp $5\text{ V}$ và nối chân `+` lên thẳng nguồn $5\text{ V}$, kích mức `LOW`).*

### Cảm biến dò line & Vật cản
| Chức năng | Vị trí | Chân STM32 | Chế độ chân |
| :--- | :--- | :--- | :--- |
| Dò line ngoài trái (`L2`) | Mũi xe (trái ngoài) | `PB11` | `INPUT` |
| Dò line trong trái (`L1`) | Mũi xe (trái trong) | `PB12` | `INPUT` |
| Dò line giữa (`M`) | Mũi xe (tâm) | `PB13` | `INPUT` |
| Dò line trong phải (`R1`) | Mũi xe (phải trong) | `PB14` | `INPUT` |
| Dò line ngoài phải (`R2`) | Mũi xe (phải ngoài) | `PB15` | `INPUT` |
| Vật cản trái (E18-D80NK) | Sườn trái | `PA8` | `INPUT_PULLUP` |
| Vật cản phải (E18-D80NK) | Sườn phải | `PA10` | `INPUT_PULLUP` |

### Hai cảm biến màu hai bên sườn
| Module | Chân cảm biến | Nối vào | Ghi chú kỹ thuật |
| :--- | :--- | :--- | :--- |
| **TCS3200 (Trái)** | `S2` / `S3` | `PB0` / `PB1` | Chọn bộ lọc màu |
| **TCS3200 (Trái)** | `OUT` | `PB10` | Dùng chân chịu xung $5\text{ V}$ (đã chuyển từ `PA6` sang) |
| **TCS3200 (Trái)** | `VCC` / `S0` | Nguồn `5V` | Kết hợp `S1 = GND` để chia tần số còn $20\%$ |
| **TCS3200 (Trái)** | `GND` / `S1` / `OE` | `GND` | Mở cổng xuất xung và nối mass chung |
| **TCS34725 (Phải)** | `SCL` / `SDA` | `PB6` / `PB7` | Giao tiếp I2C1 |
| **TCS34725 (Phải)** | `VIN` / `GND` | `5V` / `GND` | Để trống chân `LED` để LED cam tự sáng |

## 3. Hiện trạng nguồn & Lưu ý cơ khí
* **Tình trạng Pack 3S 18650:** 1 cell từng bị tụt sâu xuống $1.36\text{ V}$ đã được kích phục hồi và hoạt động ổn định. Điện áp đo gần nhất của 3 cell lần lượt là $3.98\text{ V}$, $3.81\text{ V}$, $3.79\text{ V}$ (tổng $11.58\text{ V}$, tương đương $\approx 60\text{--}75\%$ dung lượng).
* **Cảnh báo lệch áp cell:** Cell 1 ($3.98\text{ V}$) đang cao hơn 2 cell còn lại khoảng $0.19\text{ V}$, cần sạc cân bằng trước khi chạy thi đấu dài hơi.
* **Gia cố cơ khí:** Đầu giắc cái tại `PB0` (`S2`) và `PB10` (`OUT`) dễ lỏng chốt do rung động từ 4 bánh xe, cần bóp khít lõi kim loại và bó dây cố định.