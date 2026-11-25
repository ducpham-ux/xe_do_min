# Xe dò line phát hiện & gắp rác đỏ – Firmware STM32F103

Firmware cho xe dự thi **Cuộc thi Robot Sinh viên 2026 “KMA_Môi trường xanh”**.
Vòng 1: dò line, báo vật đỏ. Vòng 2: gắp rác.

## Bắt đầu nhanh

| Việc | Lệnh / file |
|---|---|
| Đọc hướng dẫn | `tai_lieu/Huong_dan_su_dung.docx` |
| Đọc báo cáo kỹ thuật | `tai_lieu/Bao_cao_ky_thuat.docx` |
| Nạp vào xe (ST-Link) | mở `firmware/` bằng VS Code + PlatformIO → `bluepill` → Upload |
| File nạp sẵn | `firmware/ban_nap_san/firmware_vong1.bin`, `firmware_vong2_gap.bin` |
| Chỉnh tham số | `firmware/lib/core/src/params.h` |
| Chỉnh chân nối | `firmware/include/pins.h` |
| Unit test trên PC | `bash tools/run_host_tests.sh` |
| Mô phỏng cả lượt thi | `bash tools/run_sim.sh --seeds 50` |

## Khởi động (nút RESET = START)

- Không che gì: chạy thi đấu sau 1 giây.
- Che E18 **trái**: CALIB học màu TCS3200 (nên làm tại sân).
- Che E18 **phải**: TEST, in giá trị cảm biến ra PB5 (57600 baud).
- Che **cả hai**: xóa bộ nhớ restart.

## Thay đổi sau test lần 1

Xem `docs/thay_doi_sau_test_lan_1.md`: thanh 5 mắt thẳng 7 cm, chạy chậm hơn, bỏ qua cung bo 45°,
bỏ TCS34725, tiếp cận vật bằng dịch ngang cho TCS3200 (1,5 cm), đi ngược cung để đọc vật bên phải,
tự định vị lại trên bản đồ.

## Kết quả mô phỏng (50 lượt, 8 đỏ + 16 nhiễu đặt ngẫu nhiên)

Về đích 50/50 · 0 báo nhầm · bắt 80,8% vật đỏ · thời gian trung vị 271 s.
Không vật thể: 90/90 lượt đi đúng toàn bộ lộ trình với cung bo R = 15/20/25 cm.
Chi tiết ở `docs/ket_qua_mo_phong/`.

## Cấu trúc

```
firmware/   PlatformIO (STM32duino). lib/core = thuật toán độc lập phần cứng
sim/        mô phỏng sân + xe, chạy cùng lõi thuật toán
tools/      script test / mô phỏng / vẽ hình / dựng tài liệu
docs/       spec, plan (quy trình Superpowers), kết quả mô phỏng
tai_lieu/   tài liệu Word + hình
```

## Chưa kiểm trên xe thật

Firmware mới được kiểm bằng unit test, mô phỏng và biên dịch chéo.
Trước khi thi cần đo các tham số trong `docs/thay_doi_sau_test_lan_1.md` mục 7
(`vBaseCmS`, `turnRateDegS`, `sensorAheadCm`, ngưỡng TCS3200, chiều động cơ) và chạy CALIB.
