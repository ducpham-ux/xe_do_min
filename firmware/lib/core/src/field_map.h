// field_map.h - mô hình đồ thị sân Vòng 1 và lập lộ trình.
//
//   A ---c1--- B ---c2--- C        (B có nhánh cụt VÒNG 1 phía Bắc)
//   |c3        |c4        |c5
//   D ---c6--- E ---c7--- F
//   |c8        |c9        |c10
//   G ---c11-- H ---c12-- I        (H có nhánh XUẤT PHÁT phía Nam)
//
// Mỗi cung dài 150 cm. Trạng thái lập lộ trình = (nút sắp tới/đang đứng, hướng khi tới nút).
// Danh sách hành động: phần tử thứ k là việc làm ở nút thứ k gặp; Stop = dừng (ĐÍCH).
#pragma once
#include <stdint.h>
#include "node_detector.h"

namespace core {

enum class Dir : uint8_t { N = 0, E = 1, S = 2, W = 3 };
enum class Act : uint8_t { Straight, Left, Right, UTurn, Stop };

enum : uint8_t { kA = 0, kB, kC, kD, kE, kF, kG, kH, kI, kStart = 9, kStub = 10 };
constexpr uint8_t kNumNodes = 9;
constexpr uint8_t kNumEdges = 12;
constexpr uint16_t kAllEdges = (1u << kNumEdges) - 1;

int8_t neighbor(uint8_t node, Dir d);         // -1 nếu không có đường
int8_t edgeOf(uint8_t a, uint8_t b);          // 0..11 hoặc -1
Dir applyAct(Dir h, Act a);
Act actToward(Dir h, Dir want);
NodeShape expectedShape(uint8_t node, Dir heading);
char nodeName(uint8_t node);

extern const Act kDefaultRoute[];             // lộ trình phủ tối ưu, bắt đầu tại H hướng Bắc
extern const uint8_t kDefaultRouteLen;

// BFS đường ngắn nhất (không quay đầu, không vào nhánh cụt). Trả về số hành động (kể cả Stop), 0 nếu không có.
uint8_t planPath(uint8_t node, Dir heading, uint8_t target, Act* out, uint8_t max);
// Lộ trình tham lam: đi các cung chưa phủ gần nhất, rồi về target.
uint8_t planCoverage(uint8_t node, Dir heading, uint16_t covered, uint8_t target, Act* out, uint8_t max);

}  // namespace core
