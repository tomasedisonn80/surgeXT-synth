#!/usr/bin/env bash
# Run native UI tests on a real X11 desktop, not a bare Xvfb server. JUCE looks
# up WM_PROTOCOLS without creating it; Openbox publishes the required WM atoms.
set -euo pipefail

if [[ $# -eq 0 ]]; then
    echo "Usage: $0 COMMAND [ARGUMENT ...]" >&2
    exit 2
fi

if [[ "$1" != "--inside-xvfb" ]]; then
    exec xvfb-run --auto-servernum --server-args='-screen 0 1920x1200x24' \
        bash "$0" --inside-xvfb "$@"
fi
shift
if [[ $# -eq 0 ]]; then
    echo "A native test command is required." >&2
    exit 2
fi

openbox --sm-disable >&2 &
native_wm_pid=$!
cleanup_native_wm() {
    local native_test_status=$?
    trap - EXIT
    # This PID is the child started above, never a process-name/global kill.
    kill "$native_wm_pid" 2>/dev/null || true
    wait "$native_wm_pid" 2>/dev/null || true
    exit "$native_test_status"
}
trap cleanup_native_wm EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

native_wm_ready=false
for ((native_wm_attempt = 0; native_wm_attempt < 100; ++native_wm_attempt)); do
    if ! kill -0 "$native_wm_pid" 2>/dev/null; then
        echo "Openbox exited before the native test desktop became ready." >&2
        exit 1
    fi
    native_wm_property=$(xprop -root _NET_SUPPORTING_WM_CHECK 2>/dev/null || true)
    if [[ "$native_wm_property" =~ window\ id\ \#\ 0x([[:xdigit:]]+) ]] &&
        ((16#${BASH_REMATCH[1]} != 0)); then
        native_wm_ready=true
        break
    fi
    sleep 0.1
done
if [[ "$native_wm_ready" != true ]]; then
    echo "Openbox did not publish its readiness atom within 10 seconds." >&2
    exit 1
fi

echo "Native X11 desktop ready: $native_wm_property"
"$@"
