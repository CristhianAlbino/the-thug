#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  "$root/../CompanionCore/CompanionCore.cpp" \
  "$root/../HunterCore/HunterCore.cpp" \
  "$root/../HunterCore/RunRules.cpp" \
  "$root/../PerceptionCore/PerceptionCore.cpp" \
  "$root/../SurvivalCore/Inventory.cpp" \
  "$root/../SurvivalCore/SurvivalCore.cpp" \
  "$root/../IntegrationCore/IntegrationCore.cpp" \
  "$root/SaveCore.cpp" "$root/WorldState.cpp" "$root/tests.cpp" \
  -o "${TMPDIR:-/tmp}/the-thug-save-tests"
"${TMPDIR:-/tmp}/the-thug-save-tests"
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  "$root/../CompanionCore/CompanionCore.cpp" \
  "$root/../HunterCore/HunterCore.cpp" \
  "$root/../HunterCore/RunRules.cpp" \
  "$root/../PerceptionCore/PerceptionCore.cpp" \
  "$root/../SurvivalCore/Inventory.cpp" \
  "$root/../SurvivalCore/SurvivalCore.cpp" \
  "$root/../IntegrationCore/IntegrationCore.cpp" \
  "$root/SaveCore.cpp" "$root/WorldState.cpp" "$root/campaign_tests.cpp" \
  -o "${TMPDIR:-/tmp}/the-thug-campaign-tests"
"${TMPDIR:-/tmp}/the-thug-campaign-tests"
