#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
python3 "$root/build_data.py" --check
python3 "$root/build_companion_data.py" --check
python3 "$root/../DirectorCore/build_data.py" --check
sources=( \
  "$root/../CompanionCore/CompanionCore.cpp" \
  "$root/../HunterCore/HunterCore.cpp" "$root/../HunterCore/RunRules.cpp" \
  "$root/../PerceptionCore/PerceptionCore.cpp" \
  "$root/../SurvivalCore/Inventory.cpp" "$root/../SurvivalCore/SurvivalCore.cpp" \
  "$root/../IntegrationCore/IntegrationCore.cpp" \
  "$root/../SaveCore/SaveCore.cpp" "$root/../SaveCore/WorldState.cpp" \
  "$root/../SaveCore/NarrativeState.cpp" "$root/../SaveCore/StorySave.cpp" \
  "$root/../WorldCore/WorldCore.cpp" "$root/CampaignCore.cpp" \
  "$root/CampaignSave.cpp" "$root/CampaignNarrative.cpp" "$root/../DirectorCore/DirectorCore.cpp" \
  "$root/CheckpointFile.cpp" "$root/CampaignCompanions.cpp" "$root/CampaignCompanionSave.cpp" "$root/CampaignHunter.cpp" "$root/CampaignHunterSave.cpp" )
objects=()
for source in "${sources[@]}"; do
  object="$build_dir/$(basename "$source" .cpp).o"
  c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -c "$source" -o "$object"
  objects+=("$object")
done
for suite in tests hunter_tests companion_tests narrative_tests checkpoint_tests checkpoint_driver; do
  c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror "$root/$suite.cpp" "${objects[@]}" -o "$build_dir/$suite"
  if [[ "$suite" != checkpoint_driver ]]; then "$build_dir/$suite"; fi
done
python3 "$root/checkpoint_crash_tests.py" "$build_dir/checkpoint_driver"
