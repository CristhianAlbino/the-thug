#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  "$root/../CompanionCore/CompanionCore.cpp" \
  "$root/../HunterCore/HunterCore.cpp" "$root/../HunterCore/RunRules.cpp" \
  "$root/../PerceptionCore/PerceptionCore.cpp" \
  "$root/../SurvivalCore/Inventory.cpp" "$root/../SurvivalCore/SurvivalCore.cpp" \
  "$root/../IntegrationCore/IntegrationCore.cpp" \
  "$root/../SaveCore/SaveCore.cpp" "$root/../SaveCore/WorldState.cpp" \
  "$root/WorldCore.cpp" "$root/tests.cpp" -o "$build_dir/world-tests"
"$build_dir/world-tests"
