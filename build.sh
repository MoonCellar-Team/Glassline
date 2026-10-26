#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

usage() {
	echo "./build.sh [linux|windows|windows-check|android|mac|web|all] [номер] — например: ./build.sh windows или ./build.sh all 1.0" >&2
	exit 1
}

TARGET=${1:-linux}
NUMBER=${2:-}
case "$TARGET" in linux | windows | windows-check | android | mac | web | all) ;; *) usage ;; esac
[ -z "$NUMBER" ] || [[ "$NUMBER" =~ ^[0-9A-Za-z._-]+$ ]] || { echo "номер — буквы, цифры, «.», «_», «-»: $NUMBER" >&2; exit 1; }
LABEL=${NUMBER:-dev}
SUFFIX=${NUMBER:+-$NUMBER}
OUT=build/release
mkdir -p "$OUT"
NAME=$(sed -n 's/^#define MEDECOMP_PORT_NAME "\(.*\)"$/\1/p' port/include/port/brand.h)
[ -n "$NAME" ] || { echo "нет MEDECOMP_PORT_NAME в port/include/port/brand.h" >&2; exit 1; }
FILE=$(echo "$NAME" | tr 'A-Z' 'a-z')
JOBS=$(nproc 2>/dev/null || echo 4)

build() {
	cmake --build "$1" -j"$JOBS" || { echo "сборка не прошла — ещё раз (загрузки рвутся)" >&2; cmake --build "$1" -j"$JOBS"; }
}

gcc_flag() {
	local tmp
	tmp=$(mktemp -d)
	printf '#include <string>\nint main() { return std::string("x").size() - 1; }\n' > "$tmp/t.cpp"
	if clang++ "$tmp/t.cpp" -o "$tmp/t" 2>/dev/null; then rm -rf "$tmp"; return; fi
	for d in $(ls -d /usr/lib/gcc/x86_64-linux-gnu/* 2>/dev/null | sort -rV); do
		if [ -e "$d/libstdc++.so" ] && clang++ --gcc-install-dir="$d" "$tmp/t.cpp" -o "$tmp/t" 2>/dev/null; then
			echo "--gcc-install-dir=$d"
			break
		fi
	done
	rm -rf "$tmp"
}
FLAG=$(gcc_flag)

linux() {
	echo "== Linux ($LABEL)"
	local engine image user=() opts=()
	engine=$(command -v podman || command -v docker) ||
		{ echo "нужен podman или docker: сборка для игроков идёт в контейнере (glibc 2.28)" >&2; exit 1; }
	if [ "$(basename "$engine")" = podman ] && [ ! -S "${XDG_RUNTIME_DIR:-/nonexistent}/bus" ]; then
		opts=(--cgroup-manager=cgroupfs --events-backend=file)
	fi
	image=medecomp-linux-build:$(cat tools/linux-dist/Containerfile tools/linux-dist/deps.sh | sha256sum | cut -c1-12)
	if ! "$engine" "${opts[@]}" image inspect "$image" > /dev/null 2>&1; then
		echo "образ сборки $image — один раз, около 20 минут"
		"$engine" "${opts[@]}" build -t "$image" -f tools/linux-dist/Containerfile tools/linux-dist
	fi
	[ "$(basename "$engine")" = docker ] && user=(-u "$(id -u):$(id -g)" -e HOME=/tmp)
	"$engine" "${opts[@]}" run --rm "${user[@]}" -v "$PWD:/src:z" -w /src "$image" bash tools/linux-dist/dist.sh "$LABEL"
	rm -f "$OUT"/*-linux-x64*.tar.gz "$OUT"/*-linux-x64*.AppImage
	tar -C "$OUT" -czf "$OUT/$FILE-linux-x64$SUFFIX.tar.gz" linux
	mv "$OUT/$FILE.AppImage" "$OUT/$FILE-linux-x64$SUFFIX.AppImage"
	echo "-> $OUT/linux, $OUT/$FILE-linux-x64$SUFFIX.tar.gz, $OUT/$FILE-linux-x64$SUFFIX.AppImage"
}

windows() {
	local dir=$OUT/win-build w=build/win
	echo "== Windows ($LABEL)"
	python3 -I tools/win_deps.py --root "$w/sysroot" --mingw "$w"
	cmake -S port -B "$dir" "-DCMAKE_TOOLCHAIN_FILE=$PWD/port/cmake/mingw-w64.cmake" -DMEDECOMP_M32=OFF \
		"-DMEDECOMP_MINGW=$(ls -d "$PWD/$w"/llvm-mingw-* | tail -1)" "-DMEDECOMP_WIN_SYSROOT=$PWD/$w/sysroot" \
		"-DMEDECOMP_HOST_CXX_FLAGS=$FLAG" "-DMEDECOMP_VERSION=$LABEL" > /dev/null
	build "$dir"
	[ -f "$dir/medecomp.exe" ] || { echo "нет $dir/medecomp.exe — сборка под Windows не прошла" >&2; exit 1; }
	python3 -I tools/win_dist.py --build "$dir" --sysroot "$w/sysroot"
	rm -rf "$OUT/windows"
	cp -r "$dir/dist/$FILE-windows-x64" "$OUT/windows"
	rm -f "$OUT"/*-windows-x64*.zip
	cp "$dir/$FILE-windows-x64.zip" "$OUT/$FILE-windows-x64$SUFFIX.zip"
	echo "-> $OUT/windows, $OUT/$FILE-windows-x64$SUFFIX.zip"
}

android() {
	echo "== Android ($LABEL)"
	android/build.sh $NUMBER
	rm -f "$OUT"/*-android*.apk
	cp "build/android/$FILE-android$SUFFIX.apk" "$OUT/"
	echo "-> $OUT/$FILE-android$SUFFIX.apk"
}

mac() {
	local arch dir
	for arch in arm64 x86_64; do
		dir=$OUT/mac-$arch-build
		echo "== macOS $arch ($LABEL)"
		python3 -I tools/mac_deps.py --arch "$arch"
		cmake -S port -B "$dir" "-DCMAKE_TOOLCHAIN_FILE=$PWD/port/cmake/macos.cmake" "-DMEDECOMP_MAC_ARCH=$arch" \
			-DMEDECOMP_FFMPEG_BUNDLE=OFF "-DMEDECOMP_VERSION=$LABEL" > /dev/null
		build "$dir"
		[ -f "$dir/medecomp" ] || { echo "нет $dir/medecomp — сборка под macOS не прошла" >&2; exit 1; }
		python3 -I tools/mac_dist.py --arch "$arch" --build "$dir"
		rm -f "$OUT"/*-macos-$arch*.zip
		cp "$dir/$FILE-macos-$arch.zip" "$OUT/$FILE-macos-$arch$SUFFIX.zip"
		echo "-> $OUT/$FILE-macos-$arch$SUFFIX.zip"
	done
}

web() {
	local dir=$OUT/web-build
	echo "== Браузер ($LABEL)"
	command -v emcc > /dev/null || { echo "нужен emsdk: source ~/emsdk/emsdk_env.sh (README, шаг 6)" >&2; exit 1; }
	python3 -I tools/web_deps.py
	cmake -S port -B "$dir" "-DCMAKE_TOOLCHAIN_FILE=$PWD/port/cmake/web.cmake" "-DMEDECOMP_VERSION=$LABEL" > /dev/null
	build "$dir"
	[ -f "$dir/medecomp.wasm" ] || { echo "нет $dir/medecomp.wasm — сборка для браузера не прошла" >&2; exit 1; }
	rm -rf "$OUT/web"
	mkdir -p "$OUT/web"
	cp "$dir/medecomp.js" "$dir/medecomp.wasm" "$dir/medecomp.data" "$dir/manifest.webmanifest" "$OUT/web/"
	cp "$dir/medecomp.html" "$OUT/web/index.html"
	cp -r "$dir/icons" "$OUT/web/"
	rm -f "$OUT"/*-web*.zip
	(cd "$OUT/web" && zip -qr "../$FILE-web$SUFFIX.zip" .)
	echo "-> $OUT/web, $OUT/$FILE-web$SUFFIX.zip"
}

case "$TARGET" in
linux) linux ;;
windows) windows ;;
windows-check) windows && tools/win_check.sh ;;
android) android ;;
mac) mac ;;
web) web ;;
all) linux && windows && android ;;
esac

[ -f tools/release-notes.md ] || exit 0
sed -e "s/{version}/$LABEL/g" -e "s/{suffix}/$SUFFIX/g" -e "s/{name}/$NAME/g" -e "s/{file}/$FILE/g" tools/release-notes.md \
	> "$OUT/release-notes$SUFFIX.md"
echo "-> $OUT/release-notes$SUFFIX.md"
