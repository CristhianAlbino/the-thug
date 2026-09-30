#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  "$root/../CompanionCore/CompanionCore.cpp" \
  "$root/../SurvivalCore/Inventory.cpp" \
  "$root/../SurvivalCore/SurvivalCore.cpp" \
  "$root/../IntegrationCore/IntegrationCore.cpp" \
  "$root/SaveCore.cpp" "$root/tests.cpp" \
  -o "${TMPDIR:-/tmp}/the-thug-save-tests"
"${TMPDIR:-/tmp}/the-thug-save-tests"
