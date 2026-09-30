#!/usr/bin/env bash
set -euo pipefail
base_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  "$base_dir/HunterCore.cpp" "$base_dir/tests.cpp" \
  -o "$build_dir/hunter_core_tests"
"$build_dir/hunter_core_tests"
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  "$base_dir/RunRules.cpp" "$base_dir/run_rules_tests.cpp" \
  -o "$build_dir/run_rules_tests"
"$build_dir/run_rules_tests"
