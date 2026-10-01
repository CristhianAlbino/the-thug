#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
python3 "$root/build_data.py" --check
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  "$root/../CompanionCore/CompanionCore.cpp" \
  "$root/../HunterCore/HunterCore.cpp" "$root/../HunterCore/RunRules.cpp" \
  "$root/../PerceptionCore/PerceptionCore.cpp" \
  "$root/../SurvivalCore/Inventory.cpp" "$root/../SurvivalCore/SurvivalCore.cpp" \
  "$root/../IntegrationCore/IntegrationCore.cpp" \
  "$root/../SaveCore/SaveCore.cpp" "$root/../SaveCore/WorldState.cpp" \
  "$root/../SaveCore/NarrativeState.cpp" "$root/../SaveCore/StorySave.cpp" \
  "$root/../WorldCore/WorldCore.cpp" "$root/CampaignCore.cpp" \
  "$root/CampaignSave.cpp" "$root/tests.cpp" -o "$build_dir/campaign_tests"
"$build_dir/campaign_tests"
