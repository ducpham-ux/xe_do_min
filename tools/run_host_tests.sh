#!/usr/bin/env bash
# Biên dịch lõi firmware + unit test bằng zig c++ và chạy trên PC.
# Dùng: bash tools/run_host_tests.sh
set -euo pipefail
shopt -s nullglob
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ZIGPY="${ZIGPY:-$ROOT/../.tools/venv/Scripts/python}"
OUT="$ROOT/build/host"
mkdir -p "$OUT"
"$ZIGPY" -m ziglang c++ -std=c++17 -O1 -Wall -Wextra -Werror \
  -I "$ROOT/firmware/lib/core/src" \
  "$ROOT"/firmware/lib/core/src/*.cpp "$ROOT"/firmware/test/host/*.cpp \
  -o "$OUT/core_tests.exe"
"$OUT/core_tests.exe"
