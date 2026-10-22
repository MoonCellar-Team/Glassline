#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
LABEL=${1:-dev}
NAME=$(sed -n 's/^#define MEDECOMP_PORT_NAME "\(.*\)"$/\1/p' port/include/port/brand.h)
[ -n "$NAME" ] || { echo "нет MEDECOMP_PORT_NAME в port/include/port/brand.h" >&2; exit 1; }
EXE=$(echo "$NAME" | tr 'A-Z' 'a-z')
P=/opt/medecomp
B=build/release/linux-build
OUT=build/release/linux
GCC=$(ls -d /opt/rh/gcc-toolset-*/root/usr/lib/gcc/x86_64-redhat-linux/* | sort -V | tail -1)
FLAG="--gcc-install-dir=$GCC"
export PKG_CONFIG_PATH=$P/lib/pkgconfig:$P/share/pkgconfig

cmake -S port -B "$B" -DMEDECOMP_M32=OFF -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
	"-DCMAKE_C_FLAGS=$FLAG" "-DCMAKE_CXX_FLAGS=$FLAG" \
	"-DCMAKE_EXE_LINKER_FLAGS=$FLAG -static-libstdc++ -static-libgcc" "-DCMAKE_SHARED_LINKER_FLAGS=$FLAG" \
	"-DMEDECOMP_GNS_PREFIX=$PWD/build/release/linux-gns" "-DMEDECOMP_VERSION=$LABEL" > /dev/null
cmake --build "$B" -j"$(nproc)" || cmake --build "$B" -j"$(nproc)"
[ -x "$B/medecomp" ] || { echo "нет $B/medecomp — сборка не прошла" >&2; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/lib"
cp "$B/medecomp" "$OUT/$EXE"
[ -d "$B/lib" ] && cp -a "$B/lib/." "$OUT/lib/"
todo=("$OUT/$EXE" "$P/lib/libcurl.so.4" "$P/lib/libqrencode.so.4")
for f in "$OUT"/lib/*.so*; do [ -L "$f" ] || todo+=("$f"); done
while [ ${#todo[@]} -gt 0 ]; do
	f=${todo[0]}
	todo=("${todo[@]:1}")
	case "$f" in "$P"/*) [ -e "$OUT/lib/$(basename "$f")" ] || cp -L "$f" "$OUT/lib/" ;; esac
	for d in $(LD_LIBRARY_PATH=$P/lib:$OUT/lib ldd "$f" | awk -v p="$P/lib/" 'index($3, p) == 1 {print $3}'); do
		[ -e "$OUT/lib/$(basename "$d")" ] || { cp -L "$d" "$OUT/lib/"; todo+=("$d"); }
	done
done
cp -L /usr/lib64/libatomic.so.1 "$OUT/lib/"
for f in "$OUT"/lib/*.so*; do [ -L "$f" ] || patchelf --set-rpath '$ORIGIN' "$f"; done
mkdir -p "$OUT/fonts"
cp /usr/share/fonts/liberation-sans/LiberationSans-*.ttf "$OUT/fonts/"
mkdir -p "$OUT/config" "$OUT/licenses"
cp config/string_overrides.tsv "$OUT/config/"
cp -r config/achievements "$OUT/config/"
cp -a "$B/branding" "$OUT/"
cp -r $P/licenses/. "$OUT/licenses/"
for f in "$OUT"/lib/*-LICENSE.txt; do [ -e "$f" ] && mv "$f" "$OUT/licenses/"; done
mkdir -p "$OUT/licenses/liberation-fonts"
cp /usr/share/licenses/liberation-fonts-common/LICENSE "$OUT/licenses/liberation-fonts/"
cat > "$OUT/fonts/fonts.conf" <<'CONF'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig>
  <dir>/usr/share/fonts</dir>
  <dir>/usr/local/share/fonts</dir>
  <dir prefix="xdg">fonts</dir>
  <cachedir prefix="xdg">fontconfig</cachedir>
</fontconfig>
CONF
patchelf --set-rpath '$ORIGIN/lib' "$OUT/$EXE"

missing=$(ldd "$OUT/$EXE" "$OUT"/lib/*.so* 2>/dev/null | grep "not found" | sort -u || true)
[ -z "$missing" ] || { echo "не нашлись библиотеки:"; echo "$missing"; exit 1; } >&2
glibc=$(for f in "$OUT/$EXE" "$OUT"/lib/*.so*; do objdump -T "$f" 2>/dev/null; done | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1)
[ "$(printf '%s\nGLIBC_2.28\n' "$glibc" | sort -V | tail -1)" = GLIBC_2.28 ] || { echo "нужна $glibc — новее 2.28" >&2; exit 1; }
system=$(ldd "$OUT/$EXE" "$OUT"/lib/*.so* 2>/dev/null | awk '$3 ~ /^\// && $3 !~ /build\/release\/linux\/lib\// {print $1}' | sort -u | tr '\n' ' ')

APPDIR=build/release/linux-appdir
rm -rf "$APPDIR"
cp -a "$OUT" "$APPDIR"
APP_ID=space.mooncellar.rooftops
sed -e "s/@NAME@/$NAME/g" -e "s/@EXEC@/$EXE/g" tools/linux-dist/$APP_ID.desktop > "$APPDIR/$APP_ID.desktop"
cp port/branding/app-icon-256.png "$APPDIR/$APP_ID.png"
cp port/branding/app-icon-256.png "$APPDIR/.DirIcon"
mkdir -p "$APPDIR/usr/share/metainfo" "$APPDIR/usr/share/icons/hicolor/256x256/apps"
sed "s/@NAME@/$NAME/g" tools/linux-dist/$APP_ID.metainfo.xml > "$APPDIR/usr/share/metainfo/$APP_ID.metainfo.xml"
cp port/branding/app-icon-256.png "$APPDIR/usr/share/icons/hicolor/256x256/apps/$APP_ID.png"
cat > "$APPDIR/AppRun" <<RUN
#!/bin/sh
here=\$(dirname "\$(readlink -f "\$0")")
exec "\$here/$EXE" "\$@"
RUN
chmod +x "$APPDIR/AppRun"
rm -f "build/release/$EXE.AppImage"
ARCH=x86_64 appimagetool --runtime-file /opt/appimage/runtime-x86_64 --no-appstream "$APPDIR" "build/release/$EXE.AppImage" > /dev/null
rm -rf "$APPDIR"
echo "готово: $OUT — библиотек $(ls "$OUT/lib" | wc -l), glibc не новее ${glibc#GLIBC_}; от системы: $system"
