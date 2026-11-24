#include "backup_codec.h"

namespace core {

namespace {
void putBits(uint16_t* words, uint16_t& pos, uint16_t value, uint8_t bits) {
  for (uint8_t i = 0; i < bits; ++i, ++pos) {
    if (value & (1u << i)) words[pos / 16] |= (uint16_t)(1u << (pos % 16));
  }
}
uint16_t getBits(const uint16_t* words, uint16_t& pos, uint8_t bits) {
  uint16_t v = 0;
  for (uint8_t i = 0; i < bits; ++i, ++pos)
    if (words[pos / 16] & (1u << (pos % 16))) v |= (uint16_t)(1u << i);
  return v;
}
}  // namespace

bool encodeBackup(uint16_t regs[kBackupRegs], uint16_t covered, const ObjectTracker& t, uint8_t restarts) {
  for (uint8_t i = 0; i < kBackupRegs; ++i) regs[i] = 0;
  uint8_t n = t.count() > kBackupMaxObjects ? kBackupMaxObjects : t.count();
  regs[0] = (uint16_t)(0xA000u | (covered & 0x0FFFu));
  regs[1] = (uint16_t)(((restarts & 0x0Fu) << 8) | n);
  uint16_t pos = 0;
  for (uint8_t i = 0; i < n; ++i) {
    const auto& e = t.at(i);
    putBits(regs + 2, pos, e.edge, 4);
    putBits(regs + 2, pos, (uint16_t)e.side, 1);
    putBits(regs + 2, pos, e.s, 8);
  }
  return true;
}

bool decodeBackup(const uint16_t regs[kBackupRegs], uint16_t& covered, ObjectTracker& t, uint8_t& restarts) {
  if ((regs[0] & 0xF000u) != 0xA000u) return false;
  uint8_t n = regs[1] & 0xFFu;
  if (n > kBackupMaxObjects) return false;
  covered = regs[0] & 0x0FFFu;
  restarts = (uint8_t)((regs[1] >> 8) & 0x0Fu);
  t.clear();
  uint16_t pos = 0;
  for (uint8_t i = 0; i < n; ++i) {
    uint8_t edge = (uint8_t)getBits(regs + 2, pos, 4);
    Side side = getBits(regs + 2, pos, 1) ? Side::Right : Side::Left;
    uint8_t s = (uint8_t)getBits(regs + 2, pos, 8);
    if (edge > kStartStubEdge) return false;
    t.mark(edge, side, s);
  }
  return true;
}

}  // namespace core
