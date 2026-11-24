"""Dựng tai_lieu/Bao_cao_ky_thuat.docx. Chạy: python tools/build_bao_cao.py"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from docx_helpers import Doc  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
H = lambda n: os.path.join(ROOT, "tai_lieu", "hinh", n)  # noqa: E731
d = Doc("Báo cáo kỹ thuật – Xe dò line phát hiện & gắp rác đỏ")

d.cover("CUỘC THI ROBOT SINH VIÊN 2026 · “KMA_MÔI TRƯỜNG XANH”", "BÁO CÁO KỸ THUẬT",
        "Xe tự hành dò line phát hiện và gắp rác màu đỏ\nThiết kế phần cứng · Thuật toán · Triển khai phần mềm · Kiểm thử",
        [["Nền tảng", "STM32F103C8T6 · L298N + 4 động cơ DC · 5 TCRT5000 · 2 E18-D80NK · TCS3200 · TCS34725"],
         ["Phần mềm", "C++17, PlatformIO + Arduino (STM32duino); lõi thuật toán độc lập phần cứng"],
         ["Kiểm chứng", "52 unit test; mô phỏng toàn sân 50 lượt ngẫu nhiên + 6 kịch bản lỗi; biên dịch chéo thành công"],
         ["Quy trình", "Superpowers: brainstorming → spec → plan → TDD → mô phỏng → debug có hệ thống → review"],
         ["Phiên bản", "v1.0 · 25/11/2026"]])
d.h1("Mục lục"); d.toc(); d.page_break()

# ================================================================== 1
d.h1("1. Tóm tắt")
d.p("Báo cáo trình bày toàn bộ thiết kế của xe tự hành dự thi Vòng 1 (tìm và cảnh báo rác đỏ) và khung mở rộng Vòng 2 (gắp rác). Xe dùng phần cứng sẵn có của đội. Phần đóng góp chính là **bộ firmware mới**: xe biết **bản đồ sân**, đi theo **lộ trình phủ đủ 12 cung** đã tối ưu, **nhận màu đỏ** bằng quét trượt và bỏ phiếu, **không báo lặp** nhờ ghi nhớ vị trí vật, cảnh báo đúng thể lệ (≥ 2 s) và **nhớ trạng thái qua restart**.")
d.p("Toàn bộ thuật toán được kiểm chứng trên máy tính bằng **trình mô phỏng sân thi đấu** viết riêng cho dự án. Qua 50 lượt với vị trí vật ngẫu nhiên, xe **về đích 50/50**, **phủ đủ 12 cung 50/50**, **0 báo nhầm, 0 báo lặp**, bắt đúng **96,8% vật đỏ nằm trong tầm cảm biến**, thời gian trung vị **83 giây** (giới hạn 300 giây). Mô phỏng cũng chỉ ra yếu tố quyết định tỉ lệ bắt đỏ của cả hệ thống là **vị trí lắp cảm biến màu** (mục 3.5).")
d.table(["Chỉ tiêu (mô phỏng 50 lượt)", "Kết quả", "Thể lệ / mục tiêu"], [
    ["Về đích trong thời gian", "50/50 lượt", "+30 điểm"],
    ["Phủ đủ 12 cung", "50/50 lượt", "để không bỏ sót vật"],
    ["Bắt đúng vật đỏ trong tầm cảm biến", "96,8% (331/342)", "+10 điểm/vật"],
    ["Bắt đúng trên tổng số vật đỏ (cảm biến lắp ở 4 cm, tầm 4 cm)", "82,8% (331/400)", "phụ thuộc lắp đặt cơ khí"],
    ["Báo nhầm vật nhiễu / báo lặp / báo không có vật", "0 / 0 / 0", "−5 điểm mỗi lần báo nhầm"],
    ["Thời gian hoàn thành (trung vị)", "82,8 s", "≤ 300 s"],
    ["Thời gian cảnh báo", "2,2 s liên tục, xe đứng yên", "≥ 2 s"],
], [7.6, 4.2, 4.8])

# ================================================================== 2
d.h1("2. Yêu cầu bài toán")
d.h2("2.1. Sân và nhiệm vụ")
d.bullets([
    "Sân 300 × 300 cm, nền trắng, line đen rộng 2,8–3,0 cm, chia 4 ô 150 × 150 cm. Ba nút trên trục giữa có **loa bo** (cung cong nối các nhánh).",
    "XUẤT PHÁT ở giữa cạnh đáy; nhánh cụt VÒNG 1 ở giữa cạnh đỉnh; ĐÍCH ở góc trên trái.",
    "Vật hình trụ/hộp cao 8–14 cm, đường kính 4–8 cm, đặt **cách line 5 ± 2 cm**. Tối đa 8 vật đỏ hợp lệ; vật nhiễu màu cam/vàng, xanh, tím, đặt ngẫu nhiên.",
    "Robot tối đa 30 × 30 cm, bánh < 10 cm, nguồn ≤ 24 VDC, khởi động bằng nút nhấn; Vòng 1 tự hành hoàn toàn.",
])
d.h2("2.2. Tính điểm và hệ quả cho thiết kế")
d.table(["Quy định", "Điểm", "Giải pháp trong firmware"], [
    ["Về đích trong 5 phút", "+30", "Lộ trình ngắn (≈ 23 m); quá 4 phút → đường ngắn nhất về đích"],
    ["Phát hiện đúng vật đỏ", "+10/vật", "Lộ trình phủ 12 cung; quét trượt; bỏ phiếu ≥ 4 mẫu"],
    ["Báo nhầm vật nhiễu", "−5/lần", "Ngưỡng đã thử + hiệu chỉnh tâm màu tại sân (CALIB)"],
    ["Không báo lặp cùng vật", "không cộng", "Nhớ vị trí vật đã báo theo (cung, bên, khoảng cách)"],
    ["Tín hiệu ≥ 2 giây", "bắt buộc", "Còi + LED liên tục 2,2 s"],
    ["Restart −5/−10/−15, thời gian tính liên tục", "trừ điểm", "Nhớ cung đã đi và vật đã báo qua nút RESET"],
    ["Vòng 2: gắp tự động +20/vật, đếm đúng +20", "cộng", "Khung điều khiển 2 servo; báo số rác bằng tiếng bíp"],
], [5.2, 2.2, 9.2], fs=8.5)

# ================================================================== 3
d.h1("3. Thiết kế phần cứng")
d.fig(H("schematic_doi.jpg"), "Hình 1 – Bản vẽ nguyên lý (schematic v3.0) của đội", 16.5)
d.fig(H("so_do_khoi.png"), "Hình 2 – Sơ đồ khối phần cứng", 15)
d.h2("3.1. Khối nguồn")
d.p("Pin 3S Li-ion 18650 (11,1 V danh định, 12,6 V khi đầy) đi qua công tắc SW1 và **cầu chì 2 A** (bảo vệ chập mạch) thành đường +12 V cấp cho L298N. Module **LM2596** (mạch hạ áp xung, hiệu suất cao, ít nóng hơn ổn áp tuyến tính) tạo +5 V cho Blue Pill, cảm biến và còi. Các tụ 100–220 µF lọc gợn khi động cơ đóng cắt, tụ 100 nF lọc nhiễu tần số cao.")
d.note("Theo nhật ký của đội, một cell từng tụt sâu 1,36 V và hiện lệch 0,19 V so với hai cell còn lại → **phải sạc cân bằng** trước khi thi. Điện áp pin ảnh hưởng trực tiếp tới tốc độ và quán tính khi quay; firmware có bộ **tự học vận tốc** (mục 4.5) để giảm phụ thuộc vào mức pin.")
d.h2("3.2. Vi điều khiển STM32F103C8T6")
d.p("Lõi ARM Cortex-M3 72 MHz, 64 KB Flash, 20 KB RAM, không có bộ tính số thực (FPU). Firmware dùng khoảng **85% Flash** và **18% RAM**. Các tài nguyên đặc biệt được dùng: Timer 1 (PWM 2 động cơ), I2C1 (TCS34725), **thanh ghi Backup** (nhớ trạng thái qua RESET), **Flash giả lập EEPROM** (lưu kết quả CALIB), **watchdog độc lập IWDG** (tự khởi động lại nếu chương trình treo quá 0,5 s).")
d.h2("3.3. Cảm biến – nguyên lý cho người mới")
d.table(["Cảm biến", "Nguyên lý", "Cách firmware dùng"], [
    ["TCRT5000 × 5", "LED hồng ngoại chiếu xuống, phototransistor nhận ánh sáng phản xạ. Nền trắng phản xạ mạnh, vạch đen hấp thụ. Mạch so sánh trên module cho ra mức số (đen = 1).", "Đọc cả 5 bit trong 1 lệnh từ cổng GPIOB (PB11–PB15 liền nhau) → tính vị trí line và phát hiện nút."],
    ["E18-D80NK × 2", "Cảm biến tiệm cận hồng ngoại có điều chế, chỉnh tầm 3–80 cm bằng biến trở; ra mức số.", "Kích hoạt quét màu khi có vật ở bên hông (tầm chỉnh 10–12 cm)."],
    ["TCS3200 (trái)", "Ma trận photodiode có kính lọc R/G/B/trong; bộ chuyển dòng → tần số. Chọn bộ lọc bằng S2/S3; S0/S1 chia tần số (20%).", "Đo độ rộng xung (pulseIn) cho từng bộ lọc: xung càng ngắn = ánh sáng màu đó càng mạnh."],
    ["TCS34725 (phải)", "4 kênh R/G/B/Clear, ADC 16 bit, giao tiếp I2C, có kính lọc hồng ngoại; module có LED cam.", "Tích phân 24 ms, khuếch đại 60× (giá trị đội đã thử); đọc 8 byte một lần."],
], [3.2, 6.8, 6.6], fs=8.5)
d.fig(H("mat_ban_nguyet.png"), "Hình 3 – Thanh 5 mắt xếp bán nguyệt: các mắt chạm vạch ngang lần lượt, không đồng thời", 14)
d.p("Vì 5 mắt nằm trên một cung tròn, khi xe tiến tới vạch ngang thì **mắt giữa chạm trước, mắt ngoài chạm sau cùng**. Firmware vì vậy không chờ “5 mắt cùng đen”, mà **tích lũy** những gì mắt ngoài thấy trong khoảng thời gian bò qua nút (mục 4.2).")
d.h2("3.4. Cơ cấu chấp hành")
d.bullets([
    "**L298N**: hai mạch cầu H. IN1/IN2 (IN3/IN4) chọn chiều quay, ENA/ENB nhận PWM 1 kHz để điều tốc. Hai động cơ mỗi bên mắc song song nên xe điều khiển kiểu vi sai (trái/phải).",
    "**Còi**: còi chủ động 5 V nối qua transistor **S8050** (PA1 → điện trở 1 kΩ → chân B), có **diode 1N4148** chống điện áp ngược. Còi không lấy dòng trực tiếp từ chân vi điều khiển nên kêu to và ổn định. Firmware vẫn dừng động cơ khi kêu, như kinh nghiệm của đội.",
    "**Servo gắp (Vòng 2)**: PA6 (nâng) và PA7 (kẹp), xung 50 Hz. Cần nguồn servo riêng.",
])
d.h2("3.5. Phát hiện quan trọng: vị trí lắp cảm biến màu")
d.p("Mô phỏng cho thấy vật bị bỏ sót **không phải do thuật toán**, mà do **cảm biến màu không với tới vật**. Vật cách mép line 3–7 cm, trong khi cảm biến màu chỉ đọc tốt trong khoảng vài cm (tài liệu đội ghi 1,5–3 cm). E18 tầm 12 cm vẫn phát hiện được vật, nhưng mặt cảm biến màu cách vật quá xa. Bảng quét dưới đây đo tỉ lệ bắt đỏ theo hai thông số cơ khí:")
d.fig(H("hinh_hoc_cam_bien.png"), "Hình 4 – Tỉ lệ bắt được vật đỏ theo vị trí mặt cảm biến và tầm đọc màu (mô phỏng)", 12)
d.note("**Khuyến nghị cơ khí:** đặt **mặt cảm biến màu và E18 ở mép thân xe, cách tâm xe khoảng 4,5 cm** (vừa đủ không chạm vật gần nhất: mép line 1,5 cm + 3 cm). Chọn cách lắp/độ nghiêng sao cho cảm biến màu đọc được tới **4–5 cm** (giữ LED chiếu sáng, thêm vành che sáng). Khi đó tỉ lệ bắt đỏ của cả hệ thống đạt **89–99%**; nếu cảm biến lùi vào 3,5 cm với tầm 3 cm thì chỉ còn **45%**.", fill="E2EFDA")
d.h2("3.6. Sai lệch giữa schematic và tài liệu đấu nối")
d.table(["Điểm", "Trên schematic v3.0", "Firmware dùng (theo tài liệu đã chạy thật)"], [
    ["I2C TCS34725", "SCL ở PB8, SDA ở PB7 (không phải cặp I2C hợp lệ)", "SCL PB6, SDA PB7; có tùy chọn PB8/PB9 (`I2C_REMAP_PB8_PB9`)"],
    ["Chân INT TCS34725", "nối vào I2C1_SDA", "không dùng – nên để trống"],
    ["IN1", "xuất hiện ở cả PA5 và PA6", "PA5 (PA6 dành cho servo Vòng 2)"],
    ["OBS_L / ENB", "cùng ghi PA9", "OBS_L = PA8, ENB = PA9"],
], [3.4, 6, 7.2], fs=8.5)

# ================================================================== 4
d.h1("4. Thuật toán")
d.h2("4.1. Bám line bằng PID")
d.fig(H("pid.png"), "Hình 5 – Vòng điều khiển PID", 15.5)
d.p("**Vị trí line** x (cm) = trung bình tọa độ ngang của các mắt đang thấy đen (L2 = −4,33; L1 = −2,5; M = 0; R1 = +2,5; R2 = +4,33). Ví dụ L1 và M cùng đen → x = −1,25 cm: line lệch trái, xe phải quẹo trái. Nếu mất line, xe giữ dấu của lần thấy cuối (±6 cm) để quay về đúng phía.")
d.p("**PID** tính lượng lái u = Kp·x + Kd·(tốc độ thay đổi của x) (+ Ki·tích phân, mặc định tắt). Bánh trái chạy V + u, bánh phải V − u. Khâu P kéo xe về giữa, khâu D “phanh” dao động. So với kiểu bật/tắt (bang-bang) của bản cũ, PID cho đường chạy mượt và có thể chạy nhanh hơn. Gần nút giao, xe **chỉ dùng 3 mắt giữa** để lái, vì mắt ngoài dễ chạm cung loa bo và kéo xe lệch.")
d.h2("4.2. Phát hiện và phân loại nút giao")
d.steps([
    "**Cổng quãng đường:** chỉ xét nút khi xe đã đi ≥ 50% chiều dài cung (75 cm). Nhờ vậy cung loa bo ở nút vừa đi qua không bị nhận nhầm thành nút mới.",
    "**Ứng viên:** một mắt ngoài (L2 hoặc R2) **và** một mắt giữa cùng đen liên tục ≥ 3 mẫu (3 ms).",
    "**Bò vào tâm:** xe đi thẳng chậm (PWM 95) trong `scanTimeMs` = 135 ms (giá trị đội đã đo), vừa đi vừa ghi nhận mắt ngoài trái/phải có thấy đen ≥ 2 mẫu liên tiếp không (lọc nhiễu 1 mẫu).",
    "**Kết luận:** có nhánh trái/phải theo bước 3; còn lối thẳng nếu sau khi bò mắt giữa vẫn thấy line.",
    "**Đối chiếu bản đồ:** loại nút đo được phải khớp loại nút dự kiến. Sai 2 lần → chuyển **chế độ dự phòng** (thuật toán bám phải + phá vòng lặp của bản cũ).",
])
d.h2("4.3. Quay tại nút")
d.p("Xe quay tại chỗ, nhanh (PWM 110) trong tối thiểu 200 ms để rời line cũ và vùng giao cắt; sau đó quay chậm (PWM 85) cho tới khi mắt giữa hoặc mắt trong phía quay thấy line **2 mẫu liên tiếp**; cuối cùng **phanh ngược 25 ms** (kỹ thuật đội đã kiểm chứng để tránh quay lố). Quá 1,8 s chưa thấy line → trạng thái tìm line (Recover): quét trái/phải với biên độ tăng dần.")
d.note("Trong quá trình phát triển, mô phỏng đã thử 2 cách khác: “đếm số vạch quét qua” và “chờ mắt giữa rời hẳn line cũ”. Cả hai đều hỏng khi trục bánh dừng lệch tâm nút vài cm, vì lúc đó line mới, cung loa bo và nhánh cũ gộp thành một vùng đen liền. Cách quay theo thời gian tối thiểu cho kết quả tốt nhất (50/50 lượt) và được giữ lại.")
d.h2("4.4. Bản đồ và lộ trình phủ sân")
d.fig(H("ban_do_san.png"), "Hình 6 – Sân thi đấu được mô hình hóa thành đồ thị", 11)
d.p("Sân được mô hình thành **đồ thị** 9 nút (A…I) và 12 cung (c1…c12). Xe biết mình đang ở **nút nào và hướng nào** (Bắc/Đông/Nam/Tây): mỗi lần rẽ, hướng đổi theo; mỗi lần qua nút, vị trí chuyển sang nút kề.")
d.p("**Lộ trình phủ tối ưu.** Bài toán “đi qua mọi cung ít nhất một lần, từ H tới A, không quay đầu giữa đường” là một biến thể của **bài toán người đưa thư Trung Hoa**. Đồ thị có 4 nút bậc lẻ (B, D, F, H), nên phải đi lặp tối thiểu 3 cung. Tìm kiếm vét cạn trên máy tính cho lời giải **16 đoạn ≈ 23 m**, đi lặp cung c4 và c7:")
d.fig(H("lo_trinh_phu.png"), "Hình 7 – Lộ trình phủ 12/12 cung", 11)
d.table(["Bước", "Nút", "Hành động", "Bước", "Nút", "Hành động"], [
    ["1", "H", "thẳng", "9", "B", "trái"],
    ["2", "E", "thẳng", "10", "E", "trái"],
    ["3", "B", "trái", "11", "F", "phải"],
    ["4", "A", "trái (lần 1 qua ĐÍCH – không dừng)", "12", "I", "phải"],
    ["5", "D", "trái", "13", "H", "thẳng"],
    ["6", "E", "thẳng", "14", "G", "phải"],
    ["7", "F", "trái", "15", "D", "thẳng"],
    ["8", "C", "trái", "16", "A", "DỪNG – ĐÍCH"],
], [1.3, 1.3, 5.6, 1.3, 1.3, 5.8], fs=8.5)
d.bullets([
    "**BFS về đích:** quá 4 phút, xe tìm đường ngắn nhất (tìm kiếm theo chiều rộng trên các trạng thái nút + hướng) từ vị trí hiện tại về A.",
    "**Lộ trình tham lam sau restart:** đi tới cung chưa phủ gần nhất, lặp lại tới khi phủ hết, rồi về A.",
])
d.h2("4.5. Ước lượng quãng đường khi không có encoder")
d.p("Xe không có cảm biến đo vòng bánh, nên quãng đường được ước lượng: **s = v̂ × (PWM − vùng chết)/(PWM nền − vùng chết) × thời gian**. Trong đó v̂ là vận tốc ở PWM nền. **v̂ tự học** sau mỗi cung 150 cm, theo tỉ lệ giữa chiều dài thật và quãng đường ước lượng (hệ số học 0,6). Firmware cũng **tự chỉnh PWM** (±15–20%) để giữ vận tốc mục tiêu khi pin đầy hoặc yếu. Quãng đường được dùng cho: cổng phát hiện nút, giảm tốc khi sắp tới nút, và định vị vật để khử lặp.")
d.h2("4.6. Nhận diện màu đỏ")
d.fig(H("quet_mau.png"), "Hình 8 – Quy trình nhận diện vật đỏ", 16)
d.bullets([
    "**Quét trượt:** E18 thường kích ở **mép vật**, lúc đó cảm biến màu chưa nhìn thẳng vào vật (đúng lỗi “E18 bắt ở 7 cm” trong nhật ký của đội). Xe dừng 30 ms rồi **bò chậm qua vật** (tối đa 8 cm), cứ 35 ms đọc một mẫu.",
    "**Bỏ phiếu:** đủ **4 mẫu đỏ** mới kết luận là đỏ. Một mẫu nhiễu không thể gây báo nhầm.",
    "**Ngưỡng mặc định (đội đã thử thật):** TCS3200: r < 450 µs và r < 0,55·g và r < 0,55·b. TCS34725: c > 80 và r > 1,10·g và r > 1,45·b.",
    "**Sau CALIB – phân loại theo tâm màu:** đổi (R, G, B) thành tỉ lệ màu r = R/(R+G+B), g = G/(R+G+B). Tỉ lệ này **không phụ thuộc độ sáng** (vật xa hay gần, đèn mạnh hay yếu). Mỗi màu (đỏ, vàng/cam, xanh, tím) được học thành một điểm tâm. Mẫu chỉ được coi là đỏ khi gần tâm đỏ (< 0,07) **và** gần đỏ hơn hẳn mọi màu khác (cách biệt ≥ 0,02).",
])
d.h2("4.7. Khử báo lặp theo vị trí")
d.p("Mỗi vật đỏ đã báo được nhớ bằng khóa **(cung, bên, khoảng cách s tính từ nút có chỉ số nhỏ)**. “Bên” tính theo **chiều của cung**, không theo bên của xe: cùng một vật nằm bên phải xe khi đi A→D nhưng nằm bên trái xe khi đi D→A. Hai lần báo cách nhau dưới 16 cm trên cùng cung, cùng bên, được coi là một vật. Lộ trình đi lặp c4 và c7, nên tính năng này bắt buộc phải có.")
d.h2("4.8. An toàn, thời gian và restart")
d.fig(H("fsm.png"), "Hình 9 – Máy trạng thái hành vi", 16)
d.bullets([
    "**Cảnh báo 2,2 s** (dư 10% so với 2 s), còi + LED liên tục, động cơ dừng.",
    "**Watchdog 0,5 s**; mọi trạng thái đều có thời hạn; lỗi I2C → khôi phục bus (9 xung SCL) và tiếp tục chạy bằng cảm biến còn lại.",
    "**Restart thông minh:** 10 thanh ghi Backup 16 bit lưu mặt nạ cung đã phủ, số lần restart, tối đa 9 vật đỏ (13 bit/vật) và **số giây thi đã trôi qua** (ghi mỗi 5 s), để mốc 4:00/5:00 vẫn đúng sau restart vì đồng hồ trọng tài chạy liên tục. Giữ nguyên khi nhấn RESET, tự mất khi tắt nguồn (dùng cờ POR để phân biệt).",
    "**Chế độ dự phòng:** nếu định vị sai, xe dùng lại thuật toán bám phải + phá vòng lặp đã chạy được của bản cũ.",
])
d.h2("4.9. Vòng 2 – gắp và đếm rác")
d.p("Sau khi báo một vật đỏ nằm ở phía tay gắp, xe chạy chuỗi **mở kẹp → hạ tay → kẹp → nâng tay** (mỗi bước 450 ms, có thể chỉnh), rồi tăng bộ đếm. Ở ĐÍCH, còi bíp đúng số rác đang chở để trọng tài đếm (+20 điểm khi đếm đúng). Đây là khung phần mềm: góc servo và tầm với cần chỉnh khi đội hoàn thiện cơ khí.")

# ================================================================== 5
d.h1("5. Triển khai phần mềm")
d.fig(H("kien_truc.png"), "Hình 10 – Kiến trúc: một lõi thuật toán, hai lớp phần cứng (xe thật / mô phỏng)", 16)
d.p("Điểm then chốt là **lớp trừu tượng phần cứng** `core::Hal`. Thuật toán không gọi `digitalRead` hay `analogWrite` trực tiếp, mà gọi `hal.readLine()`, `hal.setMotors()`... Trên xe, `ArduinoHal` cài đặt các hàm đó bằng STM32duino; trên máy tính, `SimHal` cài đặt bằng mô hình vật lý. Nhờ vậy **cùng một mã nguồn** vừa nạp được vào xe, vừa chạy thử được hàng trăm lượt thi trên máy tính.")
d.table(["File", "Vai trò"], [
    ["lib/core/src/params.h", "Mọi tham số tinh chỉnh, có chú thích (đã đo thật / cần đo)"],
    ["lib/core/src/robot.cpp", "Máy trạng thái: bám line, nút, quay, quét màu, cảnh báo, gắp, tìm line, CALIB, kết thúc"],
    ["lib/core/src/field_map.cpp", "Đồ thị sân, lộ trình phủ, BFS, lộ trình tham lam"],
    ["lib/core/src/line_pos.cpp, pid.cpp", "Vị trí line, PID"],
    ["lib/core/src/node_detector.h/.cpp", "Phát hiện/phân loại nút, bộ xác nhận nhiều mẫu"],
    ["lib/core/src/color.cpp", "Ngưỡng màu, tỉ lệ màu, tâm màu, bỏ phiếu"],
    ["lib/core/src/object_tracker.cpp, backup_codec.cpp", "Khử lặp theo vị trí; đóng gói thanh ghi Backup"],
    ["lib/core/src/alert.h, gripper.cpp", "Hẹn giờ cảnh báo; chuỗi gắp servo"],
    ["src/arduino_hal.cpp, src/main.cpp", "Phần cứng thật; chọn chế độ, chế độ TEST, watchdog"],
    ["sim/world.cpp, sim/main.cpp", "Mô phỏng sân + xe; chấm điểm; xuất quỹ đạo"],
], [6.2, 10.4], fs=8.5)
d.h2("5.1. Vòng lặp không chặn")
d.p("`loop()` gọi `robot.tick()` liên tục; mỗi mili-giây `tick()` chạy một lần: đọc 5 mắt line, cập nhật trạng thái, và cứ 4 ms tính PID một lần. Không có `delay()` trong lúc chạy, nên xe luôn phản ứng kịp với vạch và vật. Ngoại lệ duy nhất là đọc TCS3200 bằng `pulseIn` (tối đa 3 ms mỗi kênh), và việc này chỉ xảy ra khi xe đang bò chậm qua vật.")
d.h2("5.2. Quy trình phát triển (Superpowers)")
d.fig(H("quy_trinh.png"), "Hình 11 – Quy trình phát triển", 16.5)
d.bullets([
    "**Spec và Plan** được lưu trong `docs/superpowers/` để ai cũng đọc lại được lý do của từng quyết định.",
    "**TDD:** mỗi chức năng có test viết trước, chạy để thấy test FAIL, rồi mới viết code cho tới khi PASS. Tổng cộng **52 unit test**.",
    "**Debug có hệ thống:** mỗi lỗi mô phỏng tìm ra đều đi theo trình tự thu thập bằng chứng → giả thuyết → kiểm chứng → test tái hiện → sửa (bảng mục 6.3).",
    "**Commit nhỏ** theo từng bước trong git (nhánh `feat/firmware`).",
])

# ================================================================== 6
d.h1("6. Kiểm thử và kết quả")
d.h2("6.1. Unit test")
d.table(["Nhóm", "Số test", "Ví dụ kiểm tra"], [
    ["Vị trí line + PID", "7", "trung bình tọa độ, mất line giữ hướng, kẹp tích phân"],
    ["Nút giao + bộ xác nhận", "7", "cần 3 mẫu liên tiếp; bỏ qua nhiễu 1 mẫu; phân loại trái/phải/thẳng"],
    ["Bản đồ + lộ trình", "8", "lộ trình mặc định phủ 12 cung và dừng ở A; BFS; lộ trình tham lam"],
    ["Màu", "6", "ngưỡng với đỏ/vàng/tím/xanh/nền; CALIB loại được màu cam"],
    ["Khử lặp + Backup", "9", "cùng vật đi ngược chiều; 2 vật cách 20 cm; mã hóa/giải mã thanh ghi; lưu giờ thi"],
    ["Robot (máy trạng thái)", "13", "cảnh báo ≥ 2 s; không báo lặp; lỗi I2C không treo; restart giữ vật và giờ thi; CALIB; quét trượt; gắp"],
    ["Cơ cấu gắp", "2", "trình tự và thời gian servo"],
], [4.5, 2, 10.1], fs=8.5)
d.h2("6.2. Mô phỏng toàn sân")
d.p("Mô hình mô phỏng gồm: line 3 cm và loa bo bán kính 15 cm ở B, E, H; xe vi sai (khoảng cách bánh 14 cm, vùng chết PWM 30, quán tính 60 ms, hai động cơ lệch nhau ngẫu nhiên ±6%); 5 mắt bán nguyệt với nhiễu 0,2% mỗi lần đọc; E18 tầm 12 cm; cảm biến màu có nhiễu 4% và tín hiệu yếu dần theo khoảng cách; 8 vật đỏ và 16 vật nhiễu đặt ngẫu nhiên theo thể lệ.")
d.table(["Kịch bản", "Lượt", "Về đích", "Phủ 12 cung", "Bắt đỏ (trong tầm)", "Báo nhầm", "Báo lặp", "t trung vị"], [
    ["Cơ bản", "50", "50/50", "50/50", "96,8%", "0", "0", "82,8 s"],
    ["Nhiễu line ×5 (1%)", "30", "30/30", "30/30", "95,7%", "0", "0", "82,9 s"],
    ["Restart ở giây 40", "30", "30/30", "30/30", "96,0%", "0", "0", "96,2 s"],
    ["TCS34725 hỏng ở giây 30", "30", "30/30", "30/30", "71,1%", "0", "0", "79,2 s"],
    ["Có vật cam, ngưỡng mặc định", "30", "30/30", "30/30", "95,6%", "66", "0", "86,9 s"],
    ["Có vật cam, sau CALIB", "30", "30/30", "30/30", "96,1%", "0", "0", "82,8 s"],
    ["Vòng 2 (gắp bên phải)", "20", "20/20", "20/20", "98,6%", "0", "0", "87,9 s"],
], [4.4, 1.2, 1.6, 1.8, 2.4, 1.6, 1.6, 2.0], fs=8)
d.fig(H("quy_dao_mo_phong.png"), "Hình 12 – Quỹ đạo một lượt mô phỏng", 11.5)
d.h2("6.3. Lỗi tìm ra nhờ mô phỏng")
d.p("Phần giá trị nhất của mô phỏng là phát hiện các lỗi mà unit test không thấy. Mỗi lỗi được xử lý theo quy trình debug có hệ thống:")
d.table(["Hiện tượng", "Nguyên nhân gốc (đã chứng minh)", "Cách sửa"], [
    ["Xe nhận sai loại nút, rơi vào chế độ dự phòng", "1 mẫu nhiễu ở mắt ngoài trong lúc bò qua nút bị cộng dồn thành “có nhánh”. Tắt nhiễu thì hết lỗi.", "Mắt ngoài phải đen ≥ 2 mẫu liên tiếp"],
    ["Quay nhầm hướng ở góc A", "1 mẫu nhiễu khiến bước quay tưởng đã thấy line mới ở 242°", "Bộ xác nhận 2 mẫu khi quay và khi tìm line"],
    ["Báo lặp vật ở c3", "Lần đi xuống vật ở bên phải xe, lần đi lên ở bên trái xe; bộ nhớ so theo bên của xe", "So theo bên của CUNG"],
    ["Báo lặp vật ở c4 (lệch 10–17 cm)", "Ước lượng quãng đường bỏ qua vùng chết PWM; bỏ qua việc khi đi thẳng qua nút trục bánh chưa tới tâm; vận tốc học chậm", "Mô hình vùng chết; bù sensorAheadCm; hệ số học 0,6; dung sai 16 cm"],
    ["Bỏ sót ~15% vật đỏ", "Vật ở xa hơn tầm đọc màu; E18 kích ở mép vật", "Quét trượt + khuyến nghị lắp cảm biến (mục 3.5)"],
    ["66 báo nhầm khi có vật cam", "Ngưỡng R/G = 1,10 của TCS34725 quá sát màu cam (LED cam)", "Chế độ CALIB với tâm màu"],
], [4.4, 6.6, 5.6], fs=8)
d.h2("6.4. Biên dịch cho xe thật")
d.p("`pio run -e bluepill` và `pio run -e bluepill_gripper` đều **SUCCESS**, không cảnh báo: Flash 85,5% / 86,7%, RAM 17,8% / 19,3%. File nạp sẵn nằm trong `firmware/ban_nap_san/`.")

# ================================================================== 7
d.h1("7. Hạn chế và việc cần làm trên xe thật")
d.bullets([
    "Firmware **chưa được chạy trên xe thật**. Kết quả trên là của mô phỏng. Mô hình cảm biến màu và động cơ là **giả định hợp lý**, không phải số đo.",
    "Cần đo trên xe: `vBaseCmS`, `pwmDeadband`, `sensorAheadCm`, `turnMinMs`; chỉnh `kp`/`kd`; chạy CALIB dưới ánh sáng sân.",
    "Kiểm tra trên phần cứng: chiều quay động cơ (`MOTOR_*_INVERT`), mức xung đo TCS3200 (`pulseIn` LOW), thanh ghi Backup, ghi Flash.",
    "Hỏi Ban tổ chức cách xác định “về đích” (dừng ở nút A hay đi vào ô ĐÍCH). Phần đi vào ô chưa được cài đặt.",
    "Hai vật đỏ cùng bên, cùng cung, cách nhau dưới 16 cm sẽ chỉ được báo một lần (đánh đổi để không báo lặp).",
    "Cơ cấu gắp mới là khung phần mềm; chỉ gắp vật ở một bên (`gripSide`).",
])

# ================================================================== 8
d.h1("Phụ lục A – Tham số chính")
d.table(["Tham số", "Mặc định", "Nguồn", "Ý nghĩa"], [
    ["baseSpeed / approachSpeed", "140 / 110", "điểm xuất phát", "PWM nền / gần nút"],
    ["kp / kd / ki", "26 / 0,08 / 0", "mô phỏng", "hệ số PID (x tính bằng cm)"],
    ["scanSpeed / scanTimeMs", "95 / 135 ms", "đã đo thật", "bò vào tâm nút"],
    ["turnSpeed / turnSlowSpeed / brakeMs", "110 / 85 / 25 ms", "đã đo thật", "quay tại nút, phanh ngược"],
    ["turnMinMs / uturnMinMs", "200 / 550 ms", "mô phỏng", "quay nhanh tối thiểu"],
    ["nodeGateFrac / nodeConfirmSamples", "0,5 / 3", "mô phỏng", "cổng quãng đường, số mẫu ứng viên"],
    ["vBaseCmS / pwmDeadband / sensorAheadCm", "40 / 30 / 8,5", "CẦN ĐO", "ước lượng quãng đường"],
    ["creepPwm / creepMaxCm / colorNeed / colorGapMs", "75 / 8 cm / 4 / 35 ms", "mô phỏng + đã đo", "quét trượt và bỏ phiếu"],
    ["t3200MaxR, t3200Ratio", "450, 0,55", "đã đo thật", "ngưỡng TCS3200"],
    ["t34725MinC, RG, RB", "80, 1,10, 1,45", "đã đo thật", "ngưỡng TCS34725"],
    ["calibRadius / calibMargin", "0,07 / 0,02", "mô phỏng", "phân loại tâm màu"],
    ["dupTolCm", "16 cm", "mô phỏng", "dung sai khử lặp"],
    ["alertMs", "2 200 ms", "thể lệ", "thời gian cảnh báo"],
    ["goHomeMs / matchMs", "240 / 300 s", "thể lệ", "về đích sớm / dừng hẳn"],
], [5.6, 3.2, 2.8, 5.0], fs=8.5)
d.h1("Phụ lục B – Lệnh thường dùng")
d.code("""
cd xe_gap_rac
bash tools/run_host_tests.sh                     # unit test
bash tools/run_sim.sh --seeds 50                 # mô phỏng nghiệm thu
cd firmware && pio run -e bluepill -t upload     # nạp vào xe
pio device monitor -b 57600                      # xem log (qua USB-UART nối PB5)
python tools/make_figures.py                     # vẽ lại hình
python tools/build_huong_dan.py && python tools/build_bao_cao.py   # dựng lại tài liệu""")
d.save(os.path.join(ROOT, "tai_lieu", "Bao_cao_ky_thuat.docx"))
print("OK Bao_cao_ky_thuat.docx")
