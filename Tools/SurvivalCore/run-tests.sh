#!/usr/bin/env bash
set -euo pipefail
base_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  "$base_dir/SurvivalCore.cpp" "$base_dir/Inventory.cpp" \
  "$base_dir/tests.cpp" -o "$build_dir/survival_tests"
"$build_dir/survival_tests"
