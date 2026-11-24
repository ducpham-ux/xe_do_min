"""Dựng tai_lieu/Huong_dan_su_dung.docx. Chạy: python tools/build_huong_dan.py"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from docx_helpers import Doc  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
H = lambda n: os.path.join(ROOT, "tai_lieu", "hinh", n)  # noqa: E731
d = Doc("Hướng dẫn sử dụng firmware xe dò line gắp rác")

d.cover("CUỘC THI ROBOT SINH VIÊN 2026 · “KMA_MÔI TRƯỜNG XANH”", "HƯỚNG DẪN SỬ DỤNG",
        "Firmware xe dò line – phát hiện & gắp rác đỏ\n(STM32F103C8T6 · PlatformIO · Arduino)",
        [["Dành cho", "Thành viên đội, kể cả người mới làm quen vi điều khiển"],
         ["Bạn sẽ làm được", "Nạp firmware, kiểm tra cảm biến, hiệu chỉnh màu, chỉnh tham số, chạy thi đấu, xử lý sự cố"],
         ["Thư mục code", "xe_gap_rac/firmware (nạp vào xe), xe_gap_rac/sim (mô phỏng trên máy tính)"],
         ["Phiên bản", "v1.0 · 25/11/2026"]])
d.h1("Mục lục"); d.toc(); d.page_break()

# ------------------------------------------------------------------ 1
d.h1("1. Bộ firmware này làm gì?")
d.p("Firmware điều khiển xe tự chạy trên sân 300 × 300 cm theo một **lộ trình đi qua đủ 12 cung đường**, dừng lại **kêu còi + sáng LED 2,2 giây** mỗi khi gặp vật **màu đỏ** mới (không báo vật vàng/cam/xanh/tím, không báo lại vật đã báo), rồi **dừng ở ĐÍCH**. Ở Vòng 2 có thể bật thêm phần **gắp rác** bằng 2 servo.")
d.fig(H("lo_trinh_phu.png"), "Hình 1 – Lộ trình xe đi: 16 đoạn, phủ đủ 12 cung, kết thúc ở ĐÍCH (góc A)", 11)
d.h2("1.1. Cấu trúc thư mục")
d.code("""
xe_gap_rac/
├─ firmware/                 <- MỞ THƯ MỤC NÀY BẰNG VS CODE ĐỂ NẠP VÀO XE
│  ├─ platformio.ini         cấu hình build (bluepill / bluepill_gripper)
│  ├─ include/pins.h         SƠ ĐỒ CHÂN + công tắc (đảo chiều động cơ, I2C...)
│  ├─ lib/core/src/          THUẬT TOÁN (không phụ thuộc phần cứng)
│  │   └─ params.h           TẤT CẢ THAM SỐ TINH CHỈNH nằm ở đây
│  ├─ src/main.cpp           chương trình chính, chế độ TEST
│  ├─ src/arduino_hal.cpp    đọc/ghi phần cứng thật
│  └─ test/host/             50 unit test chạy trên máy tính
├─ sim/                      trình mô phỏng cả sân thi đấu
├─ tools/                    script: chạy test, mô phỏng, vẽ hình, dựng tài liệu
├─ docs/                     thiết kế, kế hoạch, kết quả mô phỏng
└─ tai_lieu/                 tài liệu Word (file này + Báo cáo kỹ thuật)""")
d.note("**Quy tắc vàng:** muốn đổi hành vi của xe (tốc độ, ngưỡng màu, thời gian...) thì chỉ sửa `firmware/lib/core/src/params.h`. Muốn đổi chân nối dây thì sửa `firmware/include/pins.h`. Không cần đụng vào các file khác.")

# ------------------------------------------------------------------ 2
d.h1("2. Chuẩn bị")
d.h2("2.1. Phần cứng")
d.table(["Thứ", "Ghi chú"], [
    ["Xe đã lắp ráp đúng bảng chân (mục 3)", "STM32F103C8T6 Blue Pill, L298N, 4 động cơ, 5 TCRT5000, 2 E18-D80NK, TCS3200, TCS34725, còi"],
    ["Mạch nạp ST-Link V2", "Nối 4 dây: SWDIO, SWCLK, GND, 3.3V vào chân SWD ở đầu Blue Pill"],
    ["Mạch USB-UART (CP2102/CH340) – khuyên có", "Để xem log: nối RX của mạch vào PB5, GND chung"],
    ["Pin 3S đã sạc cân bằng", "Mỗi cell 4,05–4,15 V trước khi chạy thật"],
], [6, 10.6])
d.h2("2.2. Phần mềm (cài 1 lần)")
d.steps([
    "Cài **Visual Studio Code** (code.visualstudio.com).",
    "Trong VS Code mở mục Extensions, tìm và cài **PlatformIO IDE**. Chờ cài xong rồi khởi động lại VS Code.",
    "Cài driver **ST-Link** (STSW-LINK009 của ST) nếu Windows chưa nhận mạch nạp.",
    "Chọn File → Open Folder → chọn thư mục `xe_gap_rac/firmware`. Lần đầu PlatformIO sẽ tự tải bộ biên dịch STM32 (~300 MB, cần mạng).",
])

# ------------------------------------------------------------------ 3
d.h1("3. Đấu nối")
d.fig(H("so_do_khoi.png"), "Hình 2 – Sơ đồ khối phần cứng", 16)
d.table(["Thiết bị", "Chân thiết bị", "Chân STM32 / nguồn", "Ghi chú"], [
    ["L298N – bánh trái", "ENA / IN1 / IN2", "PA11 / PA5 / PA4", "ENA là PWM"],
    ["L298N – bánh phải", "ENB / IN3 / IN4", "PA9 / PA3 / PA2", "ENB là PWM"],
    ["TCRT5000 (trái → phải)", "L2 · L1 · M · R1 · R2", "PB11 · PB12 · PB13 · PB14 · PB15", "Đen = HIGH"],
    ["E18-D80NK trái / phải", "OUT (dây đen)", "PA8 / PA10", "HIGH khi có vật"],
    ["TCS3200 (trái)", "S2 / S3 / OUT", "PB0 / PB1 / PB10", "S0 = 5 V, S1 = GND, OE = GND"],
    ["TCS34725 (phải)", "SCL / SDA / VIN", "PB6 / PB7 / 5 V", "Để trống chân LED (LED tự sáng)"],
    ["Còi qua S8050", "R 1 kΩ vào chân B", "PA1", "HIGH = kêu"],
    ["Debug (tùy chọn)", "RX của mạch USB-UART", "PB5", "57 600 baud"],
    ["Servo nâng / kẹp (Vòng 2)", "dây tín hiệu", "PA6 / PA7", "Nguồn servo 5–6 V riêng"],
], [4.2, 3.6, 4.6, 4.2], fs=8.5)
d.note("**Lưu ý về bản vẽ schematic v3.0:** bản vẽ ghi I2C1_SCL ở **PB8**, chân INT của TCS34725 nối vào I2C1_SDA và IN1 xuất hiện ở cả PA5 lẫn PA6. Firmware dùng **PB6/PB7** như trong tài liệu (xe đã chạy thật với cách nối này). Nếu bạn thực sự nối SCL = PB8, SDA = PB9 thì đặt `I2C_REMAP_PB8_PB9 = 1` trong `pins.h`. Không nối chân INT của TCS34725 vào SDA.", fill="FDE9E7")

# ------------------------------------------------------------------ 4
d.h1("4. Nạp firmware vào xe")
d.steps([
    "Tắt công tắc nguồn xe (động cơ không quay khi đang nạp). Nối ST-Link vào chân SWD và cắm ST-Link vào máy tính.",
    "Trong VS Code, bấm biểu tượng PlatformIO (đầu kiến) → **bluepill → Upload**. Hoặc mở Terminal và gõ `pio run -e bluepill -t upload`.",
    "Thấy dòng `SUCCESS` là xong. LED PC13 sẽ nháy khi xe ở trạng thái chờ.",
    "Muốn dùng cơ cấu gắp (Vòng 2): chọn môi trường **bluepill_gripper** thay cho bluepill.",
])
d.p("Không có ST-Link? Có thể nạp file `firmware/ban_nap_san/firmware_vong1.bin` (Vòng 2: `firmware_vong2_gap.bin`) bằng STM32CubeProgrammer qua mạch USB-UART (đặt jumper BOOT0 = 1, nạp xong đặt lại BOOT0 = 0). Tuy nhiên cách này dùng chung chân PA9/PA10 với động cơ và cảm biến, nên cần rút dây ENB/E18 phải khi nạp.")

# ------------------------------------------------------------------ 5
d.h1("5. Các chế độ khởi động")
d.p("Nút **RESET** trên Blue Pill chính là nút **START** (thể lệ cho phép khởi động bằng nút nhấn). Ngay khi bật nguồn hoặc nhấn RESET, firmware đọc 2 cảm biến E18 trong 0,3 giây để chọn chế độ:")
d.table(["Bạn làm gì khi nhấn RESET", "Chế độ", "Xe phản hồi"], [
    ["Không che gì", "RUN – thi đấu", "LED nháy nhanh 1 giây rồi xe chạy"],
    ["Che (đưa tay sát) E18 TRÁI", "CALIB – học màu", "2 tiếng bíp ngắn; LED nháy theo bước (mục 7)"],
    ["Che E18 PHẢI", "TEST – kiểm tra", "3 tiếng bíp; log giá trị cảm biến ra PB5 mỗi 0,25 s"],
    ["Che CẢ HAI", "Xóa bộ nhớ restart, rồi RUN", "1 tiếng bíp dài"],
], [5.5, 4.2, 6.9])
d.note("**Bộ nhớ restart:** khi nhấn RESET (không tắt nguồn), xe **nhớ các vật đỏ đã báo và các cung đã đi**. Sau khi trọng tài cho restart, xe chỉ đi các cung còn thiếu và không báo lại vật cũ. Muốn bắt đầu một lượt mới hoàn toàn: **tắt hẳn nguồn** hoặc che cả hai E18 khi nhấn RESET.")

# ------------------------------------------------------------------ 6
d.h1("6. Kiểm tra lần đầu (chế độ TEST)")
d.steps([
    "Nối mạch USB-UART: RX → PB5, GND → GND. Trong VS Code chọn **bluepill → Monitor** (hoặc PuTTY 57 600 baud).",
    "Che E18 phải, nhấn RESET. Màn hình sẽ in mỗi 0,25 s một dòng như sau:",
])
d.code("LINE ..#.. | E18 00 | T3200 r=880 g=930 b=850 -- | T34725 ok r=20 g=15 b=12 c=47 --")
d.table(["Kiểm tra", "Cách làm", "Kết quả đúng"], [
    ["5 mắt line", "Đặt từng mắt lên vạch đen", "Ký tự tương ứng chuyển từ . sang # (trái → phải: L2 L1 M R1 R2)"],
    ["E18", "Đưa tay sát 2 bên hông", "E18 10 (trái) / 01 (phải). Vặn biến trở để bắt ở 10–12 cm"],
    ["TCS3200", "Đặt vật đỏ cách 2 cm bên trái", "r nhỏ hơn hẳn g và b, cuối dòng hiện DO"],
    ["TCS34725", "Đặt vật đỏ bên phải", "chữ ok; r lớn hơn hẳn g, b; hiện DO. Nếu ghi LOI: kiểm tra PB6/PB7"],
    ["Vật nhiễu", "Đặt vật vàng/xanh/tím", "Không được hiện DO"],
], [3.2, 5.2, 8.2], fs=8.5)
d.h2("6.1. Chiều quay động cơ và đo tham số chuyển động")
d.steps([
    "Trong chế độ TEST, **che E18 trái 3 giây**: xe bíp 1 tiếng, chờ 1 giây, rồi **chạy thẳng 2 giây** ở tốc độ `baseSpeed`. Nếu xe chạy lùi hoặc xoay tròn, đổi `MOTOR_LEFT_INVERT` / `MOTOR_RIGHT_INVERT` trong `pins.h` rồi nạp lại.",
    "Đo quãng đường xe đi được (cm), chia 2 → ghi vào `vBaseCmS` trong `params.h` (ví dụ đi 78 cm → 39).",
    "**Che E18 phải 3 giây**: xe bíp 2 tiếng rồi quay trái trong `turnMinMs`. Góc quay nên khoảng 40–60°. Quay ít hơn thì tăng `turnMinMs`, quay nhiều hơn thì giảm.",
    "Đo khoảng cách từ trục bánh xe tới mắt line ngoài cùng (L2/R2) theo chiều dọc xe → ghi vào `sensorAheadCm`. **Tham số này quan trọng cho việc không báo lặp.**",
])

# ------------------------------------------------------------------ 7
d.h1("7. Hiệu chỉnh màu tại sân (CALIB) – NÊN LÀM")
d.p("Mô phỏng cho thấy: nếu Ban tổ chức dùng vật nhiễu **màu cam**, ngưỡng mặc định của TCS34725 (cảm biến có LED cam) có thể **báo nhầm hàng loạt** (66 lần trong 30 lượt mô phỏng, mỗi lần −5 điểm). Sau khi CALIB thì **không còn báo nhầm**. CALIB mất khoảng 2 phút và chỉ cần làm một lần cho mỗi điều kiện ánh sáng; kết quả được lưu vào Flash.")
d.steps([
    "Che E18 TRÁI rồi nhấn RESET → nghe 2 tiếng bíp. Xe đứng yên.",
    "LED nháy **1 lần** mỗi chu kỳ 2 giây = đang học **màu ĐỎ**. Đặt vật ĐỎ cạnh cảm biến **TRÁI** (cách 2–3 cm, sao cho E18 trái bắt). Giữ yên tới khi nghe **bíp ngắn** (khoảng 0,5 giây), rồi rút vật ra.",
    "Đặt cùng vật ĐỎ ở bên **PHẢI**, chờ bíp, rút ra.",
    "LED nháy **2 lần** = học màu VÀNG/CAM: làm như trên với vật vàng (hoặc cam, tùy màu nhiễu BTC công bố): trái rồi phải.",
    "LED nháy **3 lần** = màu XANH; **4 lần** = màu TÍM. Làm tương tự.",
    "Sau bước cuối, LED sáng liền và log ghi `CALIB: da luu Flash`. Nhấn RESET để chạy bình thường.",
])
d.note("Sau khi CALIB, xe dùng **tâm màu đã học** thay cho ngưỡng mặc định. Muốn quay lại ngưỡng mặc định thì nạp lại firmware và **chạy CALIB lại**. Nếu chưa từng CALIB, xe dùng ngưỡng mặc định đã thử thật của đội.")

# ------------------------------------------------------------------ 8
d.h1("8. Chạy thi đấu")
d.steps([
    "Tắt nguồn rồi bật lại (để xóa bộ nhớ restart của lượt trước).",
    "Đặt xe trên nhánh XUẤT PHÁT, mũi hướng vào sân, mắt giữa M nằm trên vạch.",
    "Khi trọng tài ra hiệu: nhấn RESET (không che E18). Sau 1 giây xe chạy.",
    "Xe tự đi theo lộ trình, báo vật đỏ (còi + LED 2,2 giây, xe đứng yên), rồi dừng ở góc ĐÍCH (LED sáng liền).",
    "**Restart:** khi trọng tài cho phép, đặt xe lại ở XUẤT PHÁT và nhấn RESET (KHÔNG tắt nguồn). Xe sẽ chỉ đi các cung còn thiếu rồi về đích.",
])
d.p("Bảo hiểm thời gian: nếu đã chạy quá **4 phút**, xe bỏ phần còn lại và đi đường ngắn nhất về ĐÍCH. Đến **5 phút** xe dừng hẳn.")
d.note("**Cần hỏi Ban tổ chức:** “về đích” là dừng ở nút góc A hay phải đi hẳn vào ô ĐÍCH? Firmware hiện **dừng ngay tại nút A** (hành động Stop cuối lộ trình trong `Robot::onNode`). Nếu BTC yêu cầu đi vào ô ĐÍCH, cần bổ sung một đoạn bò thẳng sau Stop – chưa được cài đặt.")

# ------------------------------------------------------------------ 9
d.h1("9. Tinh chỉnh – triệu chứng nào chỉnh tham số nào")
d.table(["Triệu chứng", "Tham số (params.h)", "Hướng chỉnh"], [
    ["Xe lắc qua lại khi chạy thẳng", "kp, kd", "Giảm kp 10–20%; tăng nhẹ kd"],
    ["Xe chậm bám lại khi lệch line", "kp", "Tăng kp"],
    ["Ở ngã tư xe rẽ trước khi bánh tới tâm nút", "scanTimeMs (hoặc crawlCm)", "Tăng 135 → 150"],
    ["Xe lao quá tâm nút rồi mới rẽ", "scanTimeMs", "Giảm 135 → 115"],
    ["Quay xong lại bám nhầm cung loa bo / nhánh cũ", "turnMinMs", "Tăng 200 → 250"],
    ["Quay lố qua line mới", "turnSlowSpeed, brakeMs", "Giảm turnSlowSpeed; tăng brakeMs 25 → 35"],
    ["Bỏ sót vật đỏ", "creepMaxCm, colorNeed, vị trí cảm biến", "Tăng creepMaxCm 8 → 10; giảm colorNeed 4 → 3; dời cảm biến màu gần line hơn"],
    ["Báo nhầm vật nhiễu", "CALIB, colorNeed", "Chạy CALIB; tăng colorNeed 4 → 5"],
    ["Báo lặp cùng một vật", "sensorAheadCm, vBaseCmS, dupTolCm", "Đo lại 2 tham số đầu; tăng dupTolCm 16 → 20"],
    ["Xe nhận nút giả giữa cung", "nodeGateFrac, nodeConfirmSamples", "Tăng 0,5 → 0,6; tăng 3 → 4"],
    ["Pin đầy xe chạy nhanh/quay lố", "adaptSpeed", "Để true (mặc định): xe tự học vận tốc qua mỗi cung 150 cm"],
], [5.4, 4.6, 6.6], fs=8.5)

# ------------------------------------------------------------------ 10
d.h1("10. Xử lý sự cố")
d.table(["Hiện tượng", "Nguyên nhân thường gặp", "Cách xử lý"], [
    ["Log báo `CANH BAO: khong thay TCS34725`", "Lỏng dây PB6/PB7, sai địa chỉ, nối nhầm INT vào SDA", "Cắm chặt; tháo dây INT; xe vẫn chạy bằng cảm biến trái"],
    ["TCS3200 luôn đọc r = g = b = 0", "Lỏng dây OUT (PB10) hoặc OE chưa nối GND", "Bóp chặt giắc PB10, kiểm tra OE"],
    ["TCS3200 nhận bàn tay là đỏ", "Lỏng S2 (PB0) làm kẹt bộ lọc", "Bóp chặt giắc PB0, chạy CALIB"],
    ["Còi kêu nhỏ", "Sụt áp", "Firmware đã dừng động cơ khi kêu; kiểm tra pin, S8050"],
    ["Log `LECH BAN DO`, rồi `CHE DO DU PHONG`", "Xe đếm sai nút (nhiễu, loa bo)", "Chỉnh scanTimeMs, turnMinMs; xem mục 9"],
    ["Xe không chạy sau RESET", "Đang che E18 (vào CALIB/TEST)", "Bỏ tay ra, nhấn RESET lại"],
    ["Xe chạy được một lúc thì khởi động lại", "Watchdog: chương trình bị treo > 0,5 s", "Xem log trước lúc reset; báo lỗi kèm log"],
], [5, 5.4, 6.2], fs=8.5)

# ------------------------------------------------------------------ 11
d.h1("11. Kiểm thử trên máy tính (không cần xe)")
d.p("Mọi thuật toán có thể chạy thử trên Windows trước khi nạp vào xe. Mở **Git Bash** trong thư mục `xe_gap_rac` và chạy:")
d.code("""
bash tools/run_host_tests.sh                       # 50 unit test -> "50 tests, 0 failed checks"
bash tools/run_sim.sh --seeds 50                   # mô phỏng 50 lượt thi ngẫu nhiên
bash tools/run_sim.sh --seeds 1 --verbose          # xem log từng sự kiện như trên xe thật
bash tools/run_sim.sh --seeds 30 --orange --calib  # thử vật nhiễu màu cam, có CALIB
bash tools/run_sim.sh --seeds 30 --restart-at 40000  # restart ở giây thứ 40
bash tools/run_sim.sh --seeds 20 --gripper         # thử cơ cấu gắp Vòng 2""")
d.p("Trình biên dịch `zig c++` được cài sẵn trong `xe do duong/.tools/venv` (cài bằng `pip install ziglang`). Kết quả mô phỏng in ra bảng từng lượt và dòng `KET QUA: DAT / CHUA DAT`.")
d.fig(H("quy_dao_mo_phong.png"), "Hình 3 – Một lượt mô phỏng: xe đi đủ 12 cung, báo đúng các vật đỏ nằm trong tầm cảm biến", 12)

# ------------------------------------------------------------------ 12
d.h1("12. Vòng 2 – bật cơ cấu gắp")
d.steps([
    "Lắp 2 servo: servo **nâng tay** vào PA6, servo **kẹp** vào PA7. Cấp nguồn servo bằng mạch 5–6 V riêng (không lấy từ Blue Pill), GND chung.",
    "Trong `params.h` chỉnh góc: `armUpDeg`, `armDownDeg`, `gripOpenDeg`, `gripCloseDeg`, thời gian mỗi bước `gripStepMs`, và bên lắp tay gắp `gripSide`.",
    "Nạp môi trường **bluepill_gripper**.",
    "Khi gặp vật đỏ ở đúng bên tay gắp: xe báo 2,2 s → mở kẹp → hạ tay → kẹp → nâng tay → đếm +1.",
    "Về tới ĐÍCH, còi **bíp N lần** (N = số rác đang chở), lặp lại mỗi 5 giây để trọng tài đếm.",
])
d.note("Cơ cấu gắp hiện là **khung điều khiển**: đội cần thiết kế cơ khí (tầm với 10–15 cm, nâng < 200 g, cao 10–12 cm theo thể lệ) và chỉnh lại góc/thời gian cho khớp.")

# ------------------------------------------------------------------ 13
d.h1("13. Checklist ngày thi")
d.bullets([
    "Pin sạc cân bằng, mỗi cell 4,05–4,15 V; mang pin dự phòng.",
    "Nạp đúng bản firmware đã thử; mang ST-Link + laptop có sẵn PlatformIO.",
    "Chạy TEST: 5 mắt line, 2 E18, 2 cảm biến màu đều đúng.",
    "Chạy CALIB dưới ánh sáng sân thi (nếu BTC cho phép thời gian).",
    "Kiểm tra giắc PB0, PB10, PB6, PB7; vành che sáng cảm biến màu; vít bánh xe.",
    "Tắt/bật nguồn trước mỗi lượt mới; khi restart thì CHỈ nhấn RESET.",
    "Phân công: 1 người đặt xe + RESET, 1 người bấm giờ và đếm số lần báo.",
])
d.save(os.path.join(ROOT, "tai_lieu", "Huong_dan_su_dung.docx"))
print("OK Huong_dan_su_dung.docx")
