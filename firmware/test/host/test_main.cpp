#include "minitest.h"

int main() {
  int before = 0;
  for (auto& c : mt::cases()) {
    before = mt::failures();
    c.fn();
    std::printf("%s %s\n", mt::failures() == before ? "[ OK ]" : "[FAIL]", c.name);
  }
  std::printf("\n%zu tests, %d failed checks\n", mt::cases().size(), mt::failures());
  return mt::failures() == 0 ? 0 : 1;
}
