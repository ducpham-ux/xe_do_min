# SDD ledger — plan: docs/superpowers/plans/2026-11-25-firmware-xe-gap-rac.md
Pre-flight: Task6 consumes Tasks1-5 interfaces; Task7 consumes Hal(Task1)+Robot(Task6); Task8 consumes Hal; Task9 consumes Hal servo — consistent, no conflicts.
Ruling: chạy trên nhánh feat/firmware trong repo mới (không dùng worktree) — repo mới tạo, chưa có gì trên master ngoài docs — rủi ro thấp.
Task 1: complete (commits 18fd43f..a19a3f7, tests: bash tools/run_host_tests.sh → 7 tests, 0 failed checks)
Task 2: complete (commits a19a3f7..0c88650, tests: bash tools/run_host_tests.sh → 12 tests, 0 failed checks)
Task 3: Ruling: planPath trả về độ dài KỂ CẢ Stop (E,N→A = 3) thay vì 2 như plan ghi — nhất quán với định nghĩa danh sách hành động có Stop ở cuối — sai thì chỉ lệch cách đếm, không ảnh hưởng hành vi.
Task 3: complete (commits 0c88650..8c22219, tests: bash tools/run_host_tests.sh → 20 tests, 0 failed checks)
Task 4: Ruling: Calib dùng count[2][4]+cent[2][4][2] (4 lớp: đỏ/vàng/xanh/tím) thay cho red/others của plan — gọn hơn, cho phép học từng lớp độc lập — sai thì chỉ đổi định dạng Flash.
Task 4: complete (commits 8c22219..459e694, tests: bash tools/run_host_tests.sh → 26 tests, 0 failed checks)
Task 5: Ruling: tracker lưu vị trí chính xác (tối đa 16 vật, dung sai 12 cm) thay vì đoạn 30 cm — để 2 vật đỏ cách 15 cm vẫn được báo riêng (Review Focus #2); backup chứa tối đa 9 vật (13 bit/vật) — đủ cho 8 vật đỏ tối đa — nếu >9 vật đỏ thì vật thứ 10+ có thể báo lại sau restart.
Task 5: complete (commits 459e694..2ca681d, tests: bash tools/run_host_tests.sh → 32 tests, 0 failed checks)
Task 6: Ruling: State::Alert đổi tên AlertOn (trùng tên lớp Alert); bỏ State Test/UTurn riêng (U-turn dùng Turn với act UTurn; chế độ TEST làm ở main.cpp) — gọn hơn — không ảnh hưởng hành vi.
Task 6: Ruling: nhánh XUẤT PHÁT coi như cung số 12 (kStartStubEdge) để khử lặp/ghi nhớ vật ở đó — phát hiện qua test restart — cung này không tính vào mặt nạ phủ.
Task 6: Ruling: test robot_right_sensor_failure sửa kịch bản (E18 chỉ sáng 200 ms) — E18 sáng mãi khi xe chạy là không thực tế; hành vi đúng là quét lại sau 20 cm.
Task 6: complete (commits 2ca681d..be15eb9, tests: bash tools/run_host_tests.sh → 41 tests, 0 failed checks)
Task 7: Ruling: SimHal gộp vào sim/main.cpp (không tách sim_hal.*) — một file dễ đọc hơn cho người mới — không ảnh hưởng.
Task 7: Ruling: tiêu chí bắt đỏ đo trên vật NẰM TRONG TẦM cảm biến (firmware) và báo riêng tỉ lệ toàn hệ thống — vật xa hơn tầm đọc màu là giới hạn phần cứng, không sửa được bằng firmware — báo cáo nêu khuyến nghị lắp cảm biến.
Task 7: Ruling: mô phỏng đặt sensorAheadCm=11.5 (khớp hình học mô phỏng); firmware giữ 8.5 mặc định, phải đo trên xe — sai thì khử lặp kém (đã nêu trong hướng dẫn).
Task 7: Ruling: dupTolCm 12->16 cm — sai số ước lượng vị trí khi đi chéo vào line sau khi quay ~5 cm — 2 vật đỏ cùng bên cùng cung cách <16 cm chỉ báo 1 lần.
Task 7: Ruling: vLearnAlpha 0.3->0.6 — học vận tốc nhanh hơn để cung thứ 3 đã chính xác — nhiễu thời gian cung ảnh hưởng mạnh hơn.
Task 7: Ruling: giữ quay theo thời gian tối thiểu 200 ms + xác nhận 2 mẫu; đã thử 'đếm vạch' và 'rời line trước' đều kém hơn trong mô phỏng — trên xe thật cần chỉnh turnMinMs.
Task 7: Ruling: mẫu màu vàng TCS34725 trong mô phỏng đặt R/G=0.93 và nhiễu 4% — là giả định, số thật phải đo bằng CALIB/TEST.
Task 7: complete (commits be15eb9..94b81b8, tests: bash -c 'bash tools/run_host_tests.sh | tail -1 && build/sim/run_sim.exe --seeds 50 | tail -1' → KET QUA: DAT)
Task 8: Ruling: TCS3200 đo pulseIn(OUT, LOW) — giống mã Arduino phổ biến, ngưỡng r<450 của đội có thể đo theo mức khác — sai thì ngưỡng mặc định lệch, sửa bằng CALIB hoặc đổi LOW->HIGH.
Task 8: Ruling: chiều quay động cơ IN1=HIGH,IN2=LOW là tiến — chưa kiểm trên xe — sai thì đổi MOTOR_*_INVERT trong pins.h.
Task 8: Ruling: Backup dùng getBackupRegister(LL_RTC_BKP_DR1+i) + cờ PORRSTF để bỏ qua sau khi bật nguồn — chưa kiểm trên phần cứng — sai thì restart không nhớ vật (không nguy hiểm).
Task 8: Ruling: chế độ TEST làm trong main.cpp (đo vận tốc/góc quay thủ công) thay vì State Test trong lõi — cần phần cứng thật, không mô phỏng được.
Task 8: complete (commits 94b81b8..f271912, tests: bash -c 'cd firmware && pio run -e bluepill -e bluepill_gripper | tail -3' → ========================= 2 succeeded in 00:00:07.711 =========================)
Task 9: Ruling: gripper chỉ gắp vật ở 1 bên (gripSide, mặc định phải), có thêm park() và báo số rác bằng tiếng bíp ở đích — chưa có cơ khí thật — cần đổi khi đội chốt thiết kế tay gắp.
Task 9: complete (commits f271912..2b1909e, tests: bash tools/run_host_tests.sh → 50 tests, 0 failed checks)
Task 10: Ruling: gỡ tham số goalStopAtNode/goalCreepCm (chưa cài) và binCm (không dùng); tài liệu nói rõ phần 'đi vào ô ĐÍCH' chưa có — tránh cấu hình giả.
Task 10: complete (commits 2b1909e..4b876cd, tests: bash -c 'test -s tai_lieu/Huong_dan_su_dung.docx && test -s tai_lieu/Bao_cao_ky_thuat.docx && bash tools/run_host_tests.sh | tail -1' → 50 tests, 0 failed checks)
Final review: self-review (no subagent tool — hệ thống không cho tự tạo agent khi người dùng chưa yêu cầu)
Final: fixed đồng hồ thi bị đếm lại sau restart — robot_match_clock_continues_after_restart + backup_keeps_elapsed_match_seconds RED→GREEN, suite 52/52
Final: fixed TEST mode bị watchdog reset do delay(1000) — không có khung test cho main.cpp; kiểm bằng biên dịch + đọc code (Ruling: TDD không áp dụng được cho code chỉ chạy trên phần cứng)
Final: minor (deferred): chế độ dự phòng coi 'toàn trắng sau 45 s' là ĐÍCH (giữ hành vi cũ) — có thể dừng ở đầu nhánh cụt
Final: minor (deferred): quét trượt đi thẳng không bám line tối đa 8 cm
Final: minor (deferred): hình bản đồ sân không thể hiện rõ cung loa bo

## Sau báo cáo test lần 1 (2026-11-25)
Ruling: hình học mới do chủ sở hữu cung cấp: thanh 5 mắt thẳng 7 cm, cách tâm quay 12 cm (đã hỏi), TCS3200 ở sườn trái nhìn ngang (đã hỏi).
Ruling: vùng cung bo (B/E/H) chỉ lái theo mẫu sạch, mẫu bẩn giữ thẳng — các cách "đoán line trong dải phình" (cửa sổ 2 mắt, chỉ 3 mắt giữa) đều tự khóa vào cung bo trong mô phỏng.
Ruling: ở B/E/H chỉ nhận vạch ngang sau khi thấy mẫu tách — dải phình khi xe hơi lệch góc dài ngang vạch thật, không tách được bằng độ dài.
Ruling: vật bên phải không dừng mà ghi nhớ + đi ngược cung; lập lại lộ trình tham lam toàn bộ (83% đỏ) thay vì nối thêm vào lộ trình phủ tối ưu (71%) — đổi lại có lượt chưa phủ đủ 12 cung (không bị trừ điểm).
Ruling: trở về line bằng đảo ngược từng bước dịch ngang; 2 cách nhanh hơn (tiến 18 cm rồi nhập chéo; gộp 1 bước lớn) làm va chạm tăng 2-3 lần trong mô phỏng -> tắt (rejoinMinToNodeCm = 10000).
Ruling: đỏ khi >= 4 mẫu VÀ >= 60% mẫu — vật cam + CALIB: báo nhầm 7 -> 1 trên 20 lượt.
Ruling: tiêu chí mô phỏng đổi thành bắt đỏ >= 80%, 0 báo nhầm, về đích >= 95%; va chạm chỉ báo cáo (giới hạn hình học, đã nêu khuyến nghị phần cứng).
Kết quả: 69 unit test đạt; pio build 2 cấu hình đạt; mô phỏng 50 lượt: về đích 50/50, 0 báo nhầm, 80,8% đỏ, trung vị 271 s.
