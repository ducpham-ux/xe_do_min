// minitest.h - bộ test tối giản chạy trên PC (không cần thư viện ngoài)
#pragma once
#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

namespace mt {
struct Case { const char* name; std::function<void()> fn; };
inline std::vector<Case>& cases() { static std::vector<Case> v; return v; }
inline int& failures() { static int f = 0; return f; }
struct Reg { Reg(const char* n, std::function<void()> f) { cases().push_back({n, f}); } };
}  // namespace mt

#define TEST(name) \
  static void name(); \
  static mt::Reg reg_##name(#name, name); \
  static void name()

#define CHECK(cond) do { if (!(cond)) { std::printf("  FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, #cond); ++mt::failures(); } } while (0)
#define CHECK_EQ(a, b) do { auto _a = (a); auto _b = (b); if (!(_a == _b)) { std::printf("  FAIL %s:%d  %s == %s  (%g vs %g)\n", __FILE__, __LINE__, #a, #b, (double)_a, (double)_b); ++mt::failures(); } } while (0)
#define CHECK_NEAR(a, b, eps) do { double _a = (a), _b = (b); if (std::fabs(_a - _b) > (eps)) { std::printf("  FAIL %s:%d  %s ~ %s  (%g vs %g)\n", __FILE__, __LINE__, #a, #b, _a, _b); ++mt::failures(); } } while (0)
