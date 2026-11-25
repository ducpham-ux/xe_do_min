#include "field_map.h"

namespace core {

namespace {
// Lưới 3x3: cột 0..2 (Tây -> Đông), hàng 0..2 (Nam -> Bắc). A ở góc Tây-Bắc.
inline uint8_t col(uint8_t n) { return n % 3; }
inline uint8_t row(uint8_t n) { return 2 - n / 3; }
inline uint8_t at(int c, int r) { return (uint8_t)((2 - r) * 3 + c); }

const uint8_t kEdges[kNumEdges][2] = {
    {kA, kB}, {kB, kC}, {kA, kD}, {kB, kE}, {kC, kF}, {kD, kE},
    {kE, kF}, {kD, kG}, {kE, kH}, {kF, kI}, {kG, kH}, {kH, kI}};

struct State { uint8_t node; Dir h; };
inline uint8_t sidx(uint8_t node, Dir h) { return (uint8_t)(node * 4 + (uint8_t)h); }
constexpr uint8_t kStates = kNumNodes * 4;

// BFS tổng quát trên trạng thái (nút, hướng). Dừng khi goal(from, dir, to) đúng cho một bước đi.
// Trả về độ dài đường (số bước), ghi chuỗi hành động vào out (không có Stop), cập nhật end state.
template <typename Goal>
int bfs(uint8_t node, Dir h, Goal goal, Act* out, uint8_t max, State* end) {
  int8_t parent[kStates];
  Act via[kStates];
  bool seen[kStates] = {};
  uint8_t queue[kStates];
  uint8_t qh = 0, qt = 0;
  uint8_t s0 = sidx(node, h);
  seen[s0] = true; parent[s0] = -1;
  queue[qt++] = s0;
  while (qh < qt) {
    uint8_t s = queue[qh++];
    uint8_t n = s / 4; Dir hd = (Dir)(s % 4);
    const Act acts[3] = {Act::Straight, Act::Left, Act::Right};
    for (Act a : acts) {
      Dir nd = applyAct(hd, a);
      int8_t nb = neighbor(n, nd);
      if (nb < 0 || nb >= kNumNodes) continue;  // không vào nhánh cụt
      uint8_t ns = sidx((uint8_t)nb, nd);
      bool hit = goal(n, (uint8_t)nb);
      if (!seen[ns] || hit) {
        if (hit) {
          // dựng lại đường: s0 -> ... -> s, rồi thêm a
          Act rev[kStates]; int len = 0;
          rev[len++] = a;
          for (int8_t c = (int8_t)s; parent[(uint8_t)c] >= 0; c = parent[(uint8_t)c]) rev[len++] = via[(uint8_t)c];
          if (len > max) return -1;
          for (int i = 0; i < len; ++i) out[i] = rev[len - 1 - i];
          end->node = (uint8_t)nb; end->h = nd;
          return len;
        }
        seen[ns] = true; parent[ns] = (int8_t)s; via[ns] = a;
        queue[qt++] = ns;
      }
    }
  }
  return -1;
}
}  // namespace

int8_t neighbor(uint8_t node, Dir d) {
  if (node >= kNumNodes) return -1;
  int c = col(node), r = row(node);
  if (node == kB && d == Dir::N) return kStub;
  if (node == kH && d == Dir::S) return kStart;
  switch (d) {
    case Dir::N: r++; break;
    case Dir::S: r--; break;
    case Dir::E: c++; break;
    case Dir::W: c--; break;
  }
  if (c < 0 || c > 2 || r < 0 || r > 2) return -1;
  return (int8_t)at(c, r);
}

int8_t edgeOf(uint8_t a, uint8_t b) {
  for (uint8_t i = 0; i < kNumEdges; ++i)
    if ((kEdges[i][0] == a && kEdges[i][1] == b) || (kEdges[i][0] == b && kEdges[i][1] == a)) return (int8_t)i;
  return -1;
}

Dir applyAct(Dir h, Act a) {
  uint8_t v = (uint8_t)h;
  switch (a) {
    case Act::Left: v = (uint8_t)((v + 3) & 3); break;
    case Act::Right: v = (uint8_t)((v + 1) & 3); break;
    case Act::UTurn: v = (uint8_t)((v + 2) & 3); break;
    default: break;
  }
  return (Dir)v;
}

Act actToward(Dir h, Dir want) {
  switch (((uint8_t)want - (uint8_t)h) & 3) {
    case 0: return Act::Straight;
    case 1: return Act::Right;
    case 2: return Act::UTurn;
    default: return Act::Left;
  }
}

NodeShape expectedShape(uint8_t node, Dir heading) {
  NodeShape s;
  s.left = neighbor(node, applyAct(heading, Act::Left)) >= 0;
  s.right = neighbor(node, applyAct(heading, Act::Right)) >= 0;
  s.straight = neighbor(node, heading) >= 0;
  return s;
}

char nodeName(uint8_t node) {
  if (node < kNumNodes) return (char)('A' + node);
  return node == kStart ? 'S' : 'V';
}

const Act kDefaultRoute[] = {
    Act::Straight, Act::Straight, Act::Left,  Act::Left,     Act::Left,  Act::Straight,
    Act::Left,     Act::Left,     Act::Left,  Act::Left,     Act::Right, Act::Right,
    Act::Straight, Act::Right,    Act::Straight, Act::Stop};
const uint8_t kDefaultRouteLen = sizeof(kDefaultRoute) / sizeof(kDefaultRoute[0]);

uint8_t planPath(uint8_t node, Dir heading, uint8_t target, Act* out, uint8_t max) {
  if (max == 0) return 0;
  if (node == target) { out[0] = Act::Stop; return 1; }
  State end{};
  int len = bfs(node, heading, [target](uint8_t, uint8_t to) { return to == target; }, out, (uint8_t)(max - 1), &end);
  if (len < 0) return 0;
  out[len] = Act::Stop;
  return (uint8_t)(len + 1);
}

uint8_t dirIndex(uint8_t from, uint8_t to) {
  int8_t e = edgeOf(from, to);
  if (e < 0) return 255;
  return (uint8_t)(e * 2 + (from < to ? 0 : 1));
}

uint8_t planGreedy(uint8_t node, Dir heading, uint16_t needEdges, uint32_t needDir, uint8_t target, Act* out, uint8_t max) {
  uint8_t n = 0;
  State cur{node, heading};
  needEdges &= kAllEdges;
  while (needEdges || needDir) {
    uint16_t ne = needEdges; uint32_t nd = needDir;
    State end{};
    int len = bfs(cur.node, cur.h,
                  [ne, nd](uint8_t from, uint8_t to) {
                    int8_t e = edgeOf(from, to);
                    if (e < 0) return false;
                    return (ne & (1u << e)) != 0 || (nd & (1ul << dirIndex(from, to))) != 0;
                  },
                  out + n, (uint8_t)(max - n), &end);
    if (len < 0) return 0;
    // đánh dấu mọi nhu cầu được thỏa trên đoạn vừa lập
    uint8_t nod = cur.node; Dir hd = cur.h;
    for (int i = 0; i < len; ++i) {
      hd = applyAct(hd, out[n + i]);
      int8_t nb = neighbor(nod, hd);
      needEdges &= (uint16_t)~(1u << edgeOf(nod, (uint8_t)nb));
      needDir &= ~(1ul << dirIndex(nod, (uint8_t)nb));
      nod = (uint8_t)nb;
    }
    n = (uint8_t)(n + len);
    cur = end;
  }
  if (n >= max) return 0;
  uint8_t tail = planPath(cur.node, cur.h, target, out + n, (uint8_t)(max - n));
  if (tail == 0) return 0;
  return (uint8_t)(n + tail);
}

uint8_t planCoverage(uint8_t node, Dir heading, uint16_t covered, uint8_t target, Act* out, uint8_t max) {
  return planGreedy(node, heading, (uint16_t)(~covered & kAllEdges), 0, target, out, max);
}

bool planTraverses(uint8_t node, Dir heading, const Act* acts, uint8_t n, uint8_t from, uint8_t to) {
  for (uint8_t i = 0; i < n; ++i) {
    if (acts[i] == Act::Stop) return false;
    heading = applyAct(heading, acts[i]);
    int8_t nb = neighbor(node, heading);
    if (nb < 0 || nb >= kNumNodes) return false;
    if (node == from && nb == to) return true;
    node = (uint8_t)nb;
  }
  return false;
}

// ------------------------------------------------------------------- định vị đa giả thuyết
void Localizer::resetAll(const NodeShape& s) {
  bits_ = 0;
  for (uint8_t n = 0; n < kNumNodes; ++n)
    for (uint8_t h = 0; h < 4; ++h) {
      NodeShape e = expectedShape(n, (Dir)h);
      // chỉ những trạng thái có đường đi tới (đến nút n theo hướng h phải có nút phía sau)
      if (neighbor(n, applyAct((Dir)h, Act::UTurn)) < 0) continue;
      if (e.left == s.left && e.right == s.right) bits_ |= stateBit(n, (Dir)h);
    }
}

uint8_t Localizer::observe(const NodeShape& s) {
  uint64_t keep = 0;
  for (uint8_t i = 0; i < kNumNodes * 4; ++i) {
    if (!((bits_ >> i) & 1u)) continue;
    NodeShape e = expectedShape(i / 4, (Dir)(i % 4));
    if (e.left == s.left && e.right == s.right) keep |= 1ull << i;
  }
  if (!keep) return 0;
  bits_ = keep;
  return count();
}

void Localizer::act(Act a) {
  uint64_t next = 0;
  for (uint8_t i = 0; i < kNumNodes * 4; ++i) {
    if (!((bits_ >> i) & 1u)) continue;
    uint8_t n = i / 4;
    Dir h = applyAct((Dir)(i % 4), a);
    int8_t nb = neighbor(n, h);
    if (nb >= 0 && nb < kNumNodes) next |= stateBit((uint8_t)nb, h);
    else if (nb == kStart || nb == kStub) next |= stateBit(n, applyAct(h, Act::UTurn));  // vào nhánh cụt rồi quay về
  }
  bits_ = next;
}

void Localizer::deadEnd() { bits_ = stateBit(kB, Dir::S) | stateBit(kH, Dir::N); }

uint8_t Localizer::count() const {
  uint8_t c = 0;
  for (uint64_t b = bits_; b; b &= b - 1) ++c;
  return c;
}

bool Localizer::nearest(uint8_t node, Dir h, uint8_t& outNode, Dir& outH) const {
  int best = 1000;
  for (uint8_t i = 0; i < kNumNodes * 4; ++i) {
    if (!((bits_ >> i) & 1u)) continue;
    uint8_t n = i / 4;
    int dc = (int)col(n) - (int)col(node < kNumNodes ? node : kH);
    int dr = (int)row(n) - (int)row(node < kNumNodes ? node : kH);
    int d = (dc < 0 ? -dc : dc) + (dr < 0 ? -dr : dr);
    d = d * 4 + ((Dir)(i % 4) == h ? 0 : 1);
    if (d < best) { best = d; outNode = n; outH = (Dir)(i % 4); }
  }
  return best < 1000;
}

}  // namespace core
