#!/usr/bin/env bash
# Run normal/ASan+UBSan/TSan tests against the production header without JUCE.
set -euo pipefail
tap_script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
tap_test_dir=$(mktemp -d)
trap 'rm -f -- "$tap_test_dir/audio-tap-test"; rmdir -- "$tap_test_dir"' EXIT
tap_flags=(-std=c++20 -O2 -g -pthread -Wall -Wextra -Werror)
case "${1:-normal}" in
    normal) ;;
    address) tap_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer) ;;
    thread) tap_flags+=(-fsanitize=thread -fno-omit-frame-pointer) ;;
    *) echo "Usage: $0 [normal|address|thread]" >&2; exit 2 ;;
esac
"${CXX:-c++}" "${tap_flags[@]}" "$tap_script_dir/audio_tap_test.cpp" -o "$tap_test_dir/audio-tap-test"
"$tap_test_dir/audio-tap-test"
