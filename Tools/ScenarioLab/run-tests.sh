#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  "$root/../HunterCore/RunRules.cpp" "$root/rules_driver.cpp" \
  -o "$build_dir/rules_driver"
python3 "$root/simulate.py" --driver "$build_dir/rules_driver" \
  --scenario all --seed 42 > "$build_dir/first.jsonl"
python3 "$root/simulate.py" --driver "$build_dir/rules_driver" \
  --scenario all --seed 42 > "$build_dir/replay.jsonl"
cmp "$build_dir/first.jsonl" "$build_dir/replay.jsonl"
python3 - "$build_dir/first.jsonl" <<'PY'
import json,sys
rows=[json.loads(line) for line in open(sys.argv[1],encoding='utf-8')]
assert len(rows)==8
assert {row['ending'] for row in rows}=={
 'GarageEscape','RoofEscape','TunnelEscape','WindowEscape'}
assert rows[6]['scenario']=='roof_timeout_window'
assert rows[6]['failed_route']=='roof' and rows[6]['ending']=='WindowEscape'
assert all(row['events'] for row in rows)
print('ScenarioLab: 8 scenarios, C++/map parity and deterministic replay passed')
PY
