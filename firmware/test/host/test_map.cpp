#include "minitest.h"
#include "field_map.h"

using namespace core;

// Chạy một danh sách hành động trên bản đồ, trả về nút cuối và mặt nạ cung đã đi
static uint8_t simulate(uint8_t node, Dir h, const Act* acts, uint8_t n, uint16_t* covered, bool* stopped) {
  *stopped = false;
  for (uint8_t i = 0; i < n; ++i) {
    if (acts[i] == Act::Stop) { *stopped = true; return node; }
    h = applyAct(h, acts[i]);
    int8_t nb = neighbor(node, h);
    if (nb < 0 || nb >= kNumNodes) return 255;
    *covered |= (uint16_t)(1u << edgeOf(node, (uint8_t)nb));
    node = (uint8_t)nb;
  }
  return node;
}

TEST(map_neighbors_and_edges) {
  CHECK_EQ(neighbor(kE, Dir::N), kB);
  CHECK_EQ(neighbor(kB, Dir::N), kStub);
  CHECK_EQ(neighbor(kH, Dir::S), kStart);
  CHECK_EQ(neighbor(kA, Dir::W), -1);
  CHECK_EQ(edgeOf(kA, kB), 0);
  CHECK_EQ(edgeOf(kB, kE), 3);
  CHECK_EQ(edgeOf(kI, kH), 11);
  CHECK_EQ(edgeOf(kA, kE), -1);
}

TEST(map_turn_arithmetic) {
  CHECK(applyAct(Dir::N, Act::Left) == Dir::W);
  CHECK(applyAct(Dir::W, Act::Right) == Dir::N);
  CHECK(applyAct(Dir::E, Act::UTurn) == Dir::W);
  CHECK(actToward(Dir::N, Dir::E) == Act::Right);
  CHECK(actToward(Dir::S, Dir::E) == Act::Left);
}

TEST(map_expected_shapes) {
  NodeShape b = expectedShape(kB, Dir::N);  // đi lên tới B: trái A, phải C, thẳng nhánh VÒNG 1
  CHECK(b.left); CHECK(b.right); CHECK(b.straight);
  NodeShape a = expectedShape(kA, Dir::W);  // góc A đi sang trái: chỉ rẽ trái xuống D
  CHECK(a.left); CHECK(!a.right); CHECK(!a.straight);
  NodeShape d = expectedShape(kD, Dir::S);  // ngã ba D đi xuống: trái E, thẳng G
  CHECK(d.left); CHECK(!d.right); CHECK(d.straight);
}

TEST(map_default_route_covers_all_edges_and_ends_at_A) {
  uint16_t cov = 0; bool stopped = false;
  uint8_t end = simulate(kH, Dir::N, kDefaultRoute, kDefaultRouteLen, &cov, &stopped);
  CHECK_EQ(end, kA);
  CHECK(stopped);
  CHECK_EQ(cov, kAllEdges);
}

TEST(map_bfs_shortest_path_to_goal) {
  Act out[20];
  uint8_t n = planPath(kE, Dir::N, kA, out, 20);
  CHECK_EQ(n, 3);
  CHECK(out[0] == Act::Straight); CHECK(out[1] == Act::Left); CHECK(out[2] == Act::Stop);
  CHECK_EQ(planPath(kA, Dir::N, kA, out, 20), 1);
  CHECK(out[0] == Act::Stop);
}

TEST(map_bfs_never_uturns_or_enters_stubs) {
  Act out[20];
  uint8_t n = planPath(kH, Dir::N, kG, out, 20);  // từ H hướng Bắc tới G: rẽ trái là xong
  CHECK_EQ(n, 2);
  for (uint8_t i = 0; i < n; ++i) CHECK(out[i] != Act::UTurn);
}

TEST(map_greedy_coverage_visits_missing_edge_then_goal) {
  Act out[40];
  uint16_t covered = kAllEdges & ~(1u << 10);  // thiếu c11 (G-H)
  uint8_t n = planCoverage(kH, Dir::N, covered, kA, out, 40);
  CHECK(n > 0);
  uint16_t cov = covered; bool stopped = false;
  uint8_t end = simulate(kH, Dir::N, out, n, &cov, &stopped);
  CHECK_EQ(cov, kAllEdges); CHECK(stopped); CHECK_EQ(end, kA);
}

TEST(map_greedy_coverage_from_scratch_covers_everything) {
  Act out[60];
  uint8_t n = planCoverage(kH, Dir::N, 0, kA, out, 60);
  uint16_t cov = 0; bool stopped = false;
  uint8_t end = simulate(kH, Dir::N, out, n, &cov, &stopped);
  CHECK_EQ(cov, kAllEdges); CHECK(stopped); CHECK_EQ(end, kA);
}
