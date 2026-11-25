# TỔNG QUAN ĐỀ TÀI & MỤC TIÊU ĐẦU RA

## 1. Thông tin chung
* **Tên dự án:** Robot tự hành dò line và nhận diện vật thể màu đỏ (Vòng 1).
* **Vi điều khiển trung tâm:** STM32F103C8T6 (Blue Pill), lập trình trên nền tảng Arduino framework.
* **Phương thức điều hướng hiện tại:** Tự dò đường độc lập (không phụ thuộc mảng kịch bản cố định), kết hợp thuật toán phá vòng lặp và quay đầu khi gặp ngõ cụt.

## 2. Đặc điểm sa hình thi đấu (Vòng 1)
* **Kích thước sân:** Lưới vuông tổng thể $300 \times 300\text{ cm}$, chia thành 4 ô vuông nhỏ kích thước $150 \times 150\text{ cm}$ (vạch đen nền trắng).
* **Hình học nút giao:** Các ngã ba, ngã tư trên trục dọc giữa đều có phần **loa bo góc (dạng phễu cong)** khiến cảm biến biên dễ kích hoạt sớm trước khi trục bánh tiến vào tâm nút.
* **Các điểm mốc đặc biệt:**
  * **XUẤT PHÁT:** Cạnh đáy giữa.
  * **VÒNG 1:** Nhánh cụt nhỏ nhô ra ở cạnh đỉnh giữa.
  * **ĐÍCH:** Ô đích nằm tại góc trên cùng bên trái sân.
* **Bố trí vật thể màu:** Các trụ màu (đỏ, xanh lá, vàng, tím) đặt **ngẫu nhiên** dọc hai bên đường line (vị trí trên sơ đồ thi đấu chỉ mang tính minh họa).

## 3. Mục tiêu đầu ra của dự án
1. **Dò line ổn định:** Bám line mượt trên toàn bộ sân $300 \times 300\text{ cm}$, vượt qua các ngã tư có loa bo góc mà không bị nhận nhầm lệch lái thành ngã rẽ và không bị quay lố góc $90^\circ$.
2. **Tự động khám phá bản đồ:** Robot tự động tìm đường đi qua các nhánh để quét vật thể, tự thoát khỏi vòng lặp chạy quanh ô $150 \times 150\text{ cm}$, tự quay đầu $180^\circ$ nếu đi nhầm vào nhánh cụt *VÒNG 1*, và dừng hoàn toàn khi tới *ĐÍCH*.
3. **Phát hiện vật thể chính xác theo thời gian thực:**
   * Quét đồng thời cả hai bên sườn bằng cảm biến tiệm cận trước khi kích hoạt đọc màu.
   * Phân biệt rõ trụ **màu Đỏ** với các vật thể nhiễu (màu Xanh lá, Vàng, Tím, da tay) với thời gian phản hồi nhanh ($\approx 210\text{ ms}$ quét tối đa).
4. **Cảnh báo rõ ràng:** Khi xác nhận vật màu đỏ, xe phanh đứng yên để đảm bảo điện áp, kích hoạt còi kêu to 3 nhịp ($3 \times 200\text{ ms}$) đồng bộ chớp LED `PC13`, sau đó tự động đi tiếp quét các khu vực còn lại mà không bỏ sót trụ đặt sát nhau.