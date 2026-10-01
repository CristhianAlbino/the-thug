#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
python3 "$root/build_data.py" --check
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror "$root/DirectorCore.cpp" "$root/tests.cpp" -o "$build_dir/director_tests"
"$build_dir/director_tests"
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror "$root/DirectorCore.cpp" "$root/parity_driver.cpp" -o "$build_dir/parity_driver"
python3 "$root/parity.py" --driver "$build_dir/parity_driver"
