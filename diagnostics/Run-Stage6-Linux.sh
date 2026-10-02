#!/usr/bin/env bash
# Each run owns its configuration/save data; original resources are read-only links.
set -eu
base=${1:?stage-6 directory}
previous=${2:?stage-4 dependencies/assets directory}
arch=${3:?x64 or arm64}
display=${4:?X11 DISPLAY}
run="$base/runs/${5:-$arch-ui-01}"
test ! -e "$run"
mkdir -p "$run/game" "$run/evidence" "$run/user-data/save/default"
tar -xf "$previous/assets-final.tar" -C "$run/game"
ln -s /tmp/s2-matrix-links.zCkO0O "$run/game/res"
ln -s /tmp/s2-matrix-links.zCkO0O/game.db "$run/game/game.db"
ln -s /tmp/s2-matrix-links.zCkO0O/lua-scripts "$run/game/scripts"
cp -a "$previous/runs/x64-vulkan-05/user-data/save/default/linux_vulkan_roundtrip" "$run/user-data/save/default/"
printf 'mainmenu\n' > "$run/game/cfg/lab-no-intro.cfg"
cp "$base/build-$arch/Game" "$run/game/Game"
cp -a "$base/src/assets/fonts" "$run/game/fonts"
export DISPLAY="$display" XDG_RUNTIME_DIR=/run/user/1000 S2_USER_DATA_DIR="$run/user-data" LP_NUM_THREADS=4
if [ "$arch" = x64 ]; then
  export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json
  if [ -d "$base/sdl-x64" ]; then export LD_LIBRARY_PATH="$base/sdl-x64/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"; fi
  command=(./Game)
else
  sysroot="$previous/arm64/sysroot"
  export VK_ICD_FILENAMES="$sysroot/usr/share/vulkan/icd.d/lvp_icd.aarch64.json"
  command=(qemu-aarch64-static -L "$sysroot" -E LD_LIBRARY_PATH=/opt/s2/sdl/lib:/opt/s2/ffmpeg/lib:/usr/lib/aarch64-linux-gnu ./Game)
fi
cd "$run/game"
nohup "${command[@]}" -windowed -1024 -harness -harness-active -cfg ./cfg/lab-no-intro.cfg > "$run/evidence/game.log" 2>&1 < /dev/null &
echo $! > "$run/evidence/game.pid"
sha256sum Game > "$run/evidence/game.sha256"
