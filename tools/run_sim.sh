#!/usr/bin/env bash
# Biên dịch và chạy trình mô phỏng toàn sân. Tham số được chuyển tiếp cho chương trình.
# Ví dụ: bash tools/run_sim.sh --seeds 50 --svg build/quy_dao.svg
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ZIGPY="${ZIGPY:-$ROOT/../.tools/venv/Scripts/python}"
OUT="$ROOT/build/sim"
mkdir -p "$OUT"
"$ZIGPY" -m ziglang c++ -std=c++17 -O2 -Wall -Wextra -Werror \
  -I "$ROOT/firmware/lib/core/src" -I "$ROOT/sim" \
  "$ROOT"/firmware/lib/core/src/*.cpp "$ROOT"/sim/*.cpp \
  -o "$OUT/run_sim.exe"
"$OUT/run_sim.exe" "$@"
