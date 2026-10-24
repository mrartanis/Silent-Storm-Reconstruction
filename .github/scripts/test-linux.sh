#!/usr/bin/env bash
set -euo pipefail
build=${1:?build directory}
shift
# Let individual tests select an invalid driver through SDL hints, too.
unset SDL_VIDEODRIVER SDL_VIDEO_DRIVER
if [[ -z ${S2_TEST_DISPLAY_RUNNING:-} ]]; then
  exec xvfb-run -a -s '-screen 0 1280x1024x24' \
    env S2_TEST_DISPLAY_RUNNING=1 bash "$0" "$build" "$@"
fi
openbox --sm-disable >"${XDG_RUNTIME_DIR:?}/openbox.log" 2>&1 &
window_manager=$!
trap 'kill "$window_manager" 2>/dev/null || true; wait "$window_manager" 2>/dev/null || true' EXIT
ready=false
for attempt in {1..100}; do
  if xprop -root _NET_SUPPORTING_WM_CHECK | grep -q 'window id'; then ready=true; break; fi
  sleep 0.1
done
if [[ "$ready" != true ]]; then cat "$XDG_RUNTIME_DIR/openbox.log"; exit 1; fi
ctest --test-dir "$build" -C Release --output-on-failure --no-tests=error --timeout 120 --parallel 1 "$@"
