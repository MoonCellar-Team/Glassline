#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
A=$PWD/build/android
FILE=$(sed -n 's/^#define MEDECOMP_PORT_NAME "\(.*\)"$/\1/p' port/include/port/brand.h | tr 'A-Z' 'a-z')
API=28
eval "$(python3 -I tools/android_deps.py)"
NUMBER=
if [[ "${1:-}" =~ ^[0-9] ]]; then
	NUMBER=$1
	shift
fi
ABIS=${*:-arm64-v8a armeabi-v7a}
android/deps.sh $ABIS
STRIP=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip
HOST_FLAG=
tmp=$(mktemp -d)
printf '#include <string>\nint main() { return std::string("x").size() - 1; }\n' > "$tmp/t.cpp"
if ! c++ "$tmp/t.cpp" -o "$tmp/t" 2>/dev/null; then
	for d in $(ls -d /usr/lib/gcc/x86_64-linux-gnu/* 2>/dev/null | sort -rV); do
		if [ -e "$d/libstdc++.so" ] && c++ --gcc-install-dir="$d" "$tmp/t.cpp" -o "$tmp/t" 2>/dev/null; then
			HOST_FLAG=--gcc-install-dir=$d
			break
		fi
	done
fi
rm -rf "$tmp"

rm -rf "$A/jniLibs"
for abi in $ABIS; do
	echo "== порт: $abi"
	b=$A/port-$abi
	P=$A/sysroot/$abi
	export PKG_CONFIG_LIBDIR=$P/lib/pkgconfig:$P/share/pkgconfig
	unset PKG_CONFIG_PATH
	cmake -S port -B "$b" -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
		-DANDROID_ABI="$abi" -DANDROID_PLATFORM=android-$API -DANDROID_STL=c++_static -DMEDECOMP_M32=OFF \
		"-DMEDECOMP_VERSION=${NUMBER:-dev}" "-DMEDECOMP_HOST_CXX_FLAGS=$HOST_FLAG" > /dev/null
	cmake --build "$b" -j"$(nproc)"
	j=$A/jniLibs/$abi
	mkdir -p "$j"
	cp "$b/libmain.so" "$P/lib/libSDL2.so" "$P/lib/libopenal.so" "$P/lib/libcurl.so" "$P/lib/libqrencode.so" "$j/"
	for f in "$b"/lib/lib*.so; do [ -e "$f" ] && cp "$f" "$j/"; done
	for f in "$j"/*.so; do "$STRIP" --strip-unneeded "$f"; done
done

assets=$A/assets
rm -rf "$assets"
mkdir -p "$assets/branding" "$assets/config" "$assets/licenses"
cp -r port/branding/. "$assets/branding/"
rm -f "$assets/branding/README.md" "$assets/branding/app-icon.ico"
cp -r config/achievements "$assets/config/"
cp config/string_overrides.tsv "$assets/config/"
first=${ABIS%% *}
cp -r "$A/sysroot/$first/licenses/." "$assets/licenses/"
for f in "$A/port-$first"/lib/*-LICENSE.txt; do [ -e "$f" ] && cp "$f" "$assets/licenses/"; done
stamp="$(git rev-parse --short HEAD)$(git diff --quiet || echo -dirty)-$(date +%s)"
(cd "$assets" && { echo "$stamp"; find . -type f | sed 's#^\./##' | sort; } > "$A/medecomp-assets.txt")
mv "$A/medecomp-assets.txt" "$assets/"

echo "== APK"
code=1
if [[ "$NUMBER" =~ ^([0-9]+)\.([0-9]+)(\.([0-9]+))?$ ]]; then
	code=$((BASH_REMATCH[1] * 10000 + BASH_REMATCH[2] * 100 + ${BASH_REMATCH[4]:-0}))
fi
gradle -p android --no-daemon -q assembleRelease "-PmedecompOut=$A" "-PmedecompVersion=${NUMBER:-dev}" "-PmedecompVersionCode=$code"
apk=$A/$FILE-android${NUMBER:+-$NUMBER}.apk
cp "$A/gradle-out/app/outputs/apk/release/app-release.apk" "$apk"
echo "-> $apk ($(du -h "$apk" | cut -f1); $ABIS)"
