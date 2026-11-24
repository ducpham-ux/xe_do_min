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
- Che E18 **trái**: CALIB học màu (nên làm tại sân).
- Che E18 **phải**: TEST, in giá trị cảm biến ra PB5 (57600 baud).
- Che **cả hai**: xóa bộ nhớ restart.

## Kết quả mô phỏng (50 lượt, vật đặt ngẫu nhiên)

Về đích 50/50 · phủ đủ 12 cung 50/50 · 0 báo nhầm · 0 báo lặp ·
bắt đúng 96,8% vật đỏ trong tầm cảm biến · thời gian trung vị 83 s.
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
Trước khi thi cần đo `vBaseCmS`, `pwmDeadband`, `sensorAheadCm`, `turnMinMs`,
kiểm tra chiều động cơ (`MOTOR_*_INVERT`) và chạy CALIB (xem Hướng dẫn mục 6–7).
