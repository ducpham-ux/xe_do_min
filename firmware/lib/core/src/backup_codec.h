// backup_codec.h - đóng gói trạng thái nhiệm vụ vào 10 thanh ghi Backup 16 bit của STM32F103
// (giữ nguyên khi nhấn RESET, mất khi tắt nguồn) để "restart thông minh".
//  regs[0] = 0xA<<12 | mặt nạ 12 cung đã phủ
//  regs[1] = số lần restart <<8 | số vật đã báo
//  regs[2..9] = dãy bit, mỗi vật 13 bit: cung(4) bên(1) s(8) -> tối đa 9 vật (bit 0..116)
//              bit 117..125 = số giây thi đã trôi qua (đồng hồ trọng tài chạy liên tục qua restart)
#pragma once
#include <stdint.h>
#include "object_tracker.h"

namespace core {
constexpr uint8_t kBackupRegs = 10;
constexpr uint8_t kStartStubEdge = 12;  // nhánh XUẤT PHÁT -> H được coi là "cung" số 12
constexpr uint8_t kBackupMaxObjects = 9;
bool encodeBackup(uint16_t regs[kBackupRegs], uint16_t covered, const ObjectTracker& t, uint8_t restarts, uint16_t elapsedS = 0);
bool decodeBackup(const uint16_t regs[kBackupRegs], uint16_t& covered, ObjectTracker& t, uint8_t& restarts, uint16_t* elapsedS = nullptr);
}  // namespace core
