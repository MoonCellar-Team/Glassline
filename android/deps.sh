#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT=$PWD
A=$ROOT/build/android
NDK=$A/sdk/ndk/27.3.13750724
API=28
TC=$NDK/toolchains/llvm/prebuilt/linux-x86_64
SRC=$A/src
JOBS=$(nproc)
[ -x "$TC/bin/clang" ] || { echo "нет NDK: python3 -I tools/android_deps.py" >&2; exit 1; }
mkdir -p "$SRC" "$A/bin"
cat > "$A/bin/meson" <<'EOF'
#!/bin/sh
exec uvx --from meson==1.6.1 meson "$@"
EOF
cat > "$A/bin/ninja" <<'EOF'
#!/bin/sh
exec uvx --from ninja==1.11.1.3 ninja "$@"
EOF
chmod +x "$A/bin/meson" "$A/bin/ninja"
export PATH=$A/bin:$PATH

fetch() {
	local f=$SRC/${3:-$(basename "$1")}
	if [ ! -f "$f.ok" ]; then
		curl -sSfL --retry 5 -o "$f" "$1"
		echo "$2  $f" | sha256sum -c --quiet -
		tar -C "$SRC" -xf "$f"
		touch "$f.ok"
	fi
}

licenses() {
	mkdir -p "$P/licenses/$2"
	find "$1" -maxdepth 2 -type f \( -iname 'COPYING*' -o -iname 'LICENSE*' -o -iname 'LICENCE*' -o -name 'FTL.TXT' \) \
		-exec cp {} "$P/licenses/$2/" \;
}

autotools() {
	local name=$1 src=$2
	shift 2
	local b=$A/deps-build/$ABI/$name
	rm -rf "$b" && mkdir -p "$b"
	(cd "$b" && "$src/configure" --host=$HOST --prefix="$P" --disable-shared --enable-static --with-pic "$@" \
		&& make -j"$JOBS" && make install)
}

cross_file() {
	cat > "$A/deps-build/$ABI/meson-cross.ini" <<EOF
[binaries]
c = '$CC'
cpp = '$CXX'
ar = '$AR'
strip = '$STRIP'
# Библиотеки статические — с их зависимостями (Libs.private): иначе программы-утилиты не компонуются
pkg-config = ['pkg-config', '--static']

[built-in options]
c_args = ['-fPIC', '-I$P/include']
cpp_args = ['-fPIC', '-I$P/include']
c_link_args = ['-L$P/lib']

[properties]
pkg_config_libdir = '$P/lib/pkgconfig'

[host_machine]
system = 'android'
cpu_family = '$CPU_FAMILY'
cpu = '$CPU'
endian = 'little'
EOF
}

meson_lib() {
	local name=$1 src=$2
	shift 2
	local b=$A/deps-build/$ABI/$name
	rm -rf "$b"
	meson setup "$b" "$src" --cross-file "$A/deps-build/$ABI/meson-cross.ini" --prefix="$P" --libdir=lib \
		--buildtype=release -Ddefault_library=static "$@"
	ninja -C "$b" install
}

cmake_lib() {
	local name=$1 src=$2
	shift 2
	local b=$A/deps-build/$ABI/$name
	rm -rf "$b"
	cmake -S "$src" -B "$b" -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" -DANDROID_ABI="$ABI" \
		-DANDROID_PLATFORM=android-$API -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$P" -DCMAKE_INSTALL_LIBDIR=lib \
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON "$@"
	cmake --build "$b" -j"$JOBS" && cmake --install "$b"
}

step() {
	local stamp
	stamp="$P/.done-$1-$(declare -f "lib_$1" | sha256sum | cut -c1-12)"
	[ -f "$stamp" ] && return
	echo "== $ABI: $1"
	rm -f "$P/.done-$1" "$P/.done-$1"-*
	"lib_$1"
	touch "$stamp"
}

lib_zlib() {
	mkdir -p "$P/lib/pkgconfig"
	printf 'Name: zlib\nDescription: zlib (Android)\nVersion: 1.2.13\nLibs: -lz\nCflags:\n' > "$P/lib/pkgconfig/zlib.pc"
}

lib_libpng() {
	fetch https://download.sourceforge.net/libpng/libpng-1.6.59.tar.xz d80dd2a38a37f803cb9b6ac7b14bd6e74ddc3b654780a8380bdf93523fdb4389
	autotools libpng "$SRC/libpng-1.6.59" --enable-hardware-optimizations=no
	licenses "$SRC/libpng-1.6.59" libpng
}

lib_freetype() {
	fetch https://download.savannah.gnu.org/releases/freetype/freetype-2.15.0.tar.xz 93cd47eb719c325107f2d3cf087d06299e944c3b48a40b55537302304dea1dd5
	CC_BUILD=cc autotools freetype "$SRC/freetype-2.15.0" --with-zlib=yes --with-png=yes --with-harfbuzz=no \
		--with-bzip2=no --with-brotli=no
	licenses "$SRC/freetype-2.15.0" freetype
}

lib_expat() {
	fetch https://github.com/libexpat/libexpat/releases/download/R_2_9_0/expat-2.9.0.tar.xz 1e6371862cc31999b368c3b89b49994f0677e1bab5f1b2b85ae3741f5d803051
	autotools expat "$SRC/expat-2.9.0" --without-docbook --without-examples --without-tests
	licenses "$SRC/expat-2.9.0" expat
}

lib_fontconfig() {
	fetch https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.15.0.tar.xz 63a0658d0e06e0fa886106452b58ef04f21f58202ea02a94c39de0d3335d7c0e
	autotools fontconfig "$SRC/fontconfig-2.15.0" --disable-docs --disable-cache-build --disable-nls
	licenses "$SRC/fontconfig-2.15.0" fontconfig
}

lib_pixman() {
	fetch https://cairographics.org/releases/pixman-0.44.2.tar.gz 6349061ce1a338ab6952b92194d1b0377472244208d47ff25bef86fc71973466
	meson_lib pixman "$SRC/pixman-0.44.2" -Dtests=disabled -Ddemos=disabled -Dgtk=disabled -Dlibpng=disabled \
		-Dopenmp=disabled -Darm-simd=disabled -Dneon=disabled -Da64-neon=disabled
	licenses "$SRC/pixman-0.44.2" pixman
}

lib_cairo() {
	fetch https://cairographics.org/releases/cairo-1.18.6.tar.xz 1c767308174337a74694da0f3ec069c271452163a1ef4540964c50c301f157d4
	meson_lib cairo "$SRC/cairo-1.18.6" -Dxlib=disabled -Dxcb=disabled -Dquartz=disabled -Dglib=disabled \
		-Dtests=disabled -Dspectre=disabled -Dsymbol-lookup=disabled -Dgtk_doc=false \
		-Dfreetype=enabled -Dfontconfig=enabled -Dpng=enabled -Dzlib=enabled
	licenses "$SRC/cairo-1.18.6" cairo
}

lib_sdl2() {
	fetch https://github.com/libsdl-org/SDL/releases/download/release-2.30.10/SDL2-2.30.10.tar.gz f59adf36a0fcf4c94198e7d3d776c1b3824211ab7aeebeb31fe19836661196aa
	cmake_lib sdl2 "$SRC/SDL2-2.30.10" -DSDL_STATIC=OFF -DSDL_SHARED=ON -DSDL_TEST=OFF
	licenses "$SRC/SDL2-2.30.10" sdl2
}

lib_openal() {
	fetch https://openal-soft.org/openal-releases/openal-soft-1.24.1.tar.bz2 0b9883d2e372d4ce66d37b142ab10b606a8a0ed3e873d1e070b1c878b695425a
	cmake_lib openal "$SRC/openal-soft-1.24.1" -DLIBTYPE=SHARED -DALSOFT_EXAMPLES=OFF -DALSOFT_UTILS=OFF \
		-DALSOFT_INSTALL_EXAMPLES=OFF -DALSOFT_INSTALL_UTILS=OFF -DALSOFT_BACKEND_OBOE=OFF
	licenses "$SRC/openal-soft-1.24.1" openal
}

host_gcc_flag() {
	local tmp
	tmp=$(mktemp -d)
	printf '#include <string>\nint main() { return std::string("x").size() - 1; }\n' > "$tmp/t.cpp"
	if c++ "$tmp/t.cpp" -o "$tmp/t" 2>/dev/null; then rm -rf "$tmp"; return; fi
	for d in $(ls -d /usr/lib/gcc/x86_64-linux-gnu/* 2>/dev/null | sort -rV); do
		if [ -e "$d/libstdc++.so" ] && c++ --gcc-install-dir="$d" "$tmp/t.cpp" -o "$tmp/t" 2>/dev/null; then
			echo "--gcc-install-dir=$d"
			break
		fi
	done
	rm -rf "$tmp"
}

host_gperf() {
	[ -x "$A/host/bin/gperf" ] && return
	fetch https://ftp.gnu.org/pub/gnu/gperf/gperf-3.1.tar.gz 588546b945bba4b70b6a3a616e80b4ab466e3f33024a352fc2198112cdbb3ae2
	local flag
	flag=$(host_gcc_flag)
	(cd "$SRC/gperf-3.1" && env -u CC -u CFLAGS -u CPPFLAGS -u LDFLAGS CXX="c++ $flag" CXXFLAGS="-O2 -std=gnu++11" \
		./configure --prefix="$A/host" \
		> /dev/null && make -j"$JOBS" > /dev/null && make install > /dev/null)
}

lib_curl() {
	fetch https://github.com/openssl/openssl/releases/download/openssl-3.5.9/openssl-3.5.9.tar.gz 603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a
	local ssl=$A/deps-build/$ABI/ssl target
	case $ABI in
	arm64-v8a) target=android-arm64 ;;
	armeabi-v7a) target=android-arm ;;
	*) target=android-x86_64 ;;
	esac
	rm -rf "$A/deps-build/$ABI/openssl" && cp -a "$SRC/openssl-3.5.9" "$A/deps-build/$ABI/openssl"
	(cd "$A/deps-build/$ABI/openssl" && env -u CC -u CXX -u AR -u RANLIB -u CFLAGS -u CXXFLAGS -u CPPFLAGS -u LDFLAGS \
		ANDROID_NDK_ROOT="$NDK" PATH="$TC/bin:$PATH" ./Configure $target -D__ANDROID_API__=$API no-shared no-tests \
		-fPIC --prefix="$ssl" --libdir=lib --openssldir=/nonexistent/medecomp-openssl \
		&& env ANDROID_NDK_ROOT="$NDK" PATH="$TC/bin:$PATH" make -j"$JOBS" build_libs \
		&& make install_dev)
	fetch https://curl.se/download/curl-8.23.0.tar.xz 8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5
	cmake_lib curl "$SRC/curl-8.23.0" -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DBUILD_CURL_EXE=OFF \
		-DBUILD_TESTING=OFF -DBUILD_LIBCURL_DOCS=OFF -DBUILD_MISC_DOCS=OFF -DENABLE_CURL_MANUAL=OFF \
		-DCURL_USE_OPENSSL=ON -DOPENSSL_ROOT_DIR="$ssl" -DOPENSSL_USE_STATIC_LIBS=ON -DCMAKE_FIND_ROOT_PATH="$ssl" \
		-DCURL_CA_BUNDLE=none -DCURL_CA_PATH=none -DCURL_USE_LIBPSL=OFF -DCURL_BROTLI=OFF -DCURL_ZSTD=OFF \
		-DUSE_NGHTTP2=OFF -DUSE_LIBIDN2=OFF -DCURL_USE_LIBSSH2=OFF -DCURL_DISABLE_LDAP=ON -DCURL_DISABLE_LDAPS=ON \
		-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--exclude-libs,ALL
	licenses "$SRC/curl-8.23.0" curl
	licenses "$SRC/openssl-3.5.9" openssl
}

lib_qrencode() {
	fetch https://github.com/fukuchi/libqrencode/archive/refs/tags/v4.1.1.tar.gz 5385bc1b8c2f20f3b91d258bf8ccc8cf62023935df2d2676b5b67049f31a049c libqrencode-4.1.1.tar.gz
	cmake_lib qrencode "$SRC/libqrencode-4.1.1" -DWITH_TOOLS=NO -DBUILD_SHARED_LIBS=YES -DCMAKE_POLICY_VERSION_MINIMUM=3.5
	licenses "$SRC/libqrencode-4.1.1" qrencode
}

build_abi() {
	ABI=$1
	case $ABI in
	arm64-v8a) TRIPLE=aarch64-linux-android HOST=aarch64-linux-android CPU_FAMILY=aarch64 CPU=aarch64 ;;
	armeabi-v7a) TRIPLE=armv7a-linux-androideabi HOST=arm-linux-androideabi CPU_FAMILY=arm CPU=armv7 ;;
	x86_64) TRIPLE=x86_64-linux-android HOST=x86_64-linux-android CPU_FAMILY=x86_64 CPU=x86_64 ;;
	*) echo "неизвестная ABI: $ABI" >&2; exit 1 ;;
	esac
	P=$A/sysroot/$ABI
	mkdir -p "$P" "$A/deps-build/$ABI"
	export CC=$TC/bin/$TRIPLE$API-clang CXX=$TC/bin/$TRIPLE$API-clang++ AR=$TC/bin/llvm-ar RANLIB=$TC/bin/llvm-ranlib \
		STRIP=$TC/bin/llvm-strip NM=$TC/bin/llvm-nm
	export CFLAGS="-O2 -fPIC" CXXFLAGS="-O2 -fPIC" CPPFLAGS="-I$P/include" LDFLAGS="-L$P/lib"
	export PKG_CONFIG_LIBDIR=$P/lib/pkgconfig:$P/share/pkgconfig
	unset PKG_CONFIG_PATH
	cross_file
	for lib in zlib libpng freetype expat fontconfig pixman cairo sdl2 openal curl qrencode; do step $lib; done
}

host_gperf
export PATH=$A/host/bin:$PATH
for abi in "${@:-arm64-v8a armeabi-v7a x86_64}"; do
	for a in $abi; do build_abi "$a"; done
done
echo "готово: $A/sysroot"
