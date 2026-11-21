#!/usr/bin/env bash
set -euo pipefail
base_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
python3 "$base_dir/emit_cpp.py" --check
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  "$base_dir/NarrativeCore.cpp" "$base_dir/narrative_tests.cpp" \
  -o "$build_dir/narrative_tests"
"$build_dir/narrative_tests"
