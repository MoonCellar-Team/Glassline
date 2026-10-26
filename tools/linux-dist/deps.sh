#!/usr/bin/env bash
set -euo pipefail
P=/opt/medecomp
S=/tmp/deps
mkdir -p "$S" "$P"
JOBS=$(nproc)
export PKG_CONFIG_PATH=$P/lib/pkgconfig:$P/share/pkgconfig
export CPPFLAGS="-I$P/include" CFLAGS="-O2" LDFLAGS="-L$P/lib"

licenses() {
	mkdir -p "$P/licenses/$2"
	find "$1" -maxdepth 2 -type f \( -iname 'COPYING*' -o -iname 'LICENSE*' -o -iname 'LICENCE*' -o -name 'FTL.TXT' \) \
		-exec cp {} "$P/licenses/$2/" \;
}

fetch() {
	local f=$S/${3:-$(basename "$1")}
	curl -sSfL --retry 5 -o "$f" "$1"
	echo "$2  $f" | sha256sum -c --quiet -
	tar -C "$S" -xf "$f"
}

lib_zlib() {
	fetch https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16
	(cd $S/zlib-1.3.2 && ./configure --prefix=$P && make -j$JOBS && make install)
	licenses $S/zlib-1.3.2 zlib
}

lib_libpng() {
	fetch https://download.sourceforge.net/libpng/libpng-1.6.59.tar.xz d80dd2a38a37f803cb9b6ac7b14bd6e74ddc3b654780a8380bdf93523fdb4389
	(cd $S/libpng-1.6.59 && ./configure --prefix=$P --disable-static && make -j$JOBS && make install)
	licenses $S/libpng-1.6.59 libpng
}

lib_freetype() {
	fetch https://download.savannah.gnu.org/releases/freetype/freetype-2.15.0.tar.xz 93cd47eb719c325107f2d3cf087d06299e944c3b48a40b55537302304dea1dd5
	(cd $S/freetype-2.15.0 && ./configure --prefix=$P --disable-static --with-zlib=yes --with-png=yes \
		--with-harfbuzz=no --with-bzip2=no --with-brotli=no && make -j$JOBS && make install)
	licenses $S/freetype-2.15.0 freetype
}

lib_expat() {
	fetch https://github.com/libexpat/libexpat/releases/download/R_2_9_0/expat-2.9.0.tar.xz 1e6371862cc31999b368c3b89b49994f0677e1bab5f1b2b85ae3741f5d803051
	(cd $S/expat-2.9.0 && ./configure --prefix=$P --disable-static --without-docbook --without-examples --without-tests \
		&& make -j$JOBS && make install)
	licenses $S/expat-2.9.0 expat
}

lib_fontconfig() {
	fetch https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.15.0.tar.xz 63a0658d0e06e0fa886106452b58ef04f21f58202ea02a94c39de0d3335d7c0e
	(cd $S/fontconfig-2.15.0 && ./configure --prefix=$P --sysconfdir=/etc --localstatedir=/var --disable-static \
		--disable-docs --disable-cache-build && make -j$JOBS && make install)
	licenses $S/fontconfig-2.15.0 fontconfig
}

lib_pixman() {
	fetch https://cairographics.org/releases/pixman-0.44.2.tar.gz 6349061ce1a338ab6952b92194d1b0377472244208d47ff25bef86fc71973466
	(cd $S/pixman-0.44.2 && meson setup b --prefix=$P --libdir=lib --buildtype=release -Dtests=disabled -Ddemos=disabled \
		-Dgtk=disabled -Dlibpng=disabled && ninja -C b install)
	licenses $S/pixman-0.44.2 pixman
}

lib_cairo() {
	fetch https://cairographics.org/releases/cairo-1.18.6.tar.xz 1c767308174337a74694da0f3ec069c271452163a1ef4540964c50c301f157d4
	(cd $S/cairo-1.18.6 && meson setup b --prefix=$P --libdir=lib --buildtype=release -Dxlib=disabled -Dxcb=disabled \
		-Dglib=disabled -Dtests=disabled -Dspectre=disabled -Dsymbol-lookup=disabled -Dgtk_doc=false \
		-Dfreetype=enabled -Dfontconfig=enabled -Dpng=enabled -Dzlib=enabled && ninja -C b install)
	licenses $S/cairo-1.18.6 cairo
}

lib_sdl2() {
	fetch https://github.com/libsdl-org/SDL/releases/download/release-2.30.10/SDL2-2.30.10.tar.gz f59adf36a0fcf4c94198e7d3d776c1b3824211ab7aeebeb31fe19836661196aa
	cmake -S $S/SDL2-2.30.10 -B $S/sdl-build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DCMAKE_INSTALL_LIBDIR=lib \
		-DSDL_STATIC=OFF -DSDL_TEST=OFF
	cmake --build $S/sdl-build -j$JOBS && cmake --install $S/sdl-build
	licenses $S/SDL2-2.30.10 sdl2
}

lib_openal() {
	fetch https://openal-soft.org/openal-releases/openal-soft-1.24.1.tar.bz2 0b9883d2e372d4ce66d37b142ab10b606a8a0ed3e873d1e070b1c878b695425a
	cmake -S $S/openal-soft-1.24.1 -B $S/al-build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DCMAKE_INSTALL_LIBDIR=lib \
		-DALSOFT_EXAMPLES=OFF -DALSOFT_UTILS=OFF -DALSOFT_INSTALL_EXAMPLES=OFF -DALSOFT_INSTALL_UTILS=OFF
	cmake --build $S/al-build -j$JOBS && cmake --install $S/al-build
	licenses $S/openal-soft-1.24.1 openal
}

lib_curl() {
	fetch https://github.com/openssl/openssl/releases/download/openssl-3.5.9/openssl-3.5.9.tar.gz 603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a
	(cd $S/openssl-3.5.9 && ./Configure linux-x86_64 no-shared no-tests --prefix=$S/ssl --libdir=lib \
		--openssldir=/etc/ssl && make -j$JOBS && make install_sw)
	fetch https://curl.se/download/curl-8.23.0.tar.xz 8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5
	(cd $S/curl-8.23.0 && LDFLAGS="$LDFLAGS -Wl,--exclude-libs,ALL" ./configure --prefix=$P --disable-static \
		--with-openssl=$S/ssl --with-zlib=$P --without-ca-bundle --without-ca-path --without-libpsl --without-brotli \
		--without-zstd --without-nghttp2 --without-libidn2 --without-libssh2 --without-librtmp --disable-ldap \
		--disable-ldaps --disable-manual --disable-docs && make -j$JOBS && make install)
	licenses $S/curl-8.23.0 curl
	licenses $S/openssl-3.5.9 openssl
}

lib_qrencode() {
	fetch https://github.com/fukuchi/libqrencode/archive/refs/tags/v4.1.1.tar.gz 5385bc1b8c2f20f3b91d258bf8ccc8cf62023935df2d2676b5b67049f31a049c libqrencode-4.1.1.tar.gz
	cmake -S $S/libqrencode-4.1.1 -B $S/qr-build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DCMAKE_INSTALL_LIBDIR=lib \
		-DWITH_TOOLS=NO -DBUILD_SHARED_LIBS=YES -DCMAKE_POLICY_VERSION_MINIMUM=3.5
	cmake --build $S/qr-build -j$JOBS && cmake --install $S/qr-build
	licenses $S/libqrencode-4.1.1 qrencode
}

lib_rpath() {
	for f in $P/lib/*.so*; do [ -L "$f" ] || patchelf --set-rpath '$ORIGIN' "$f"; done
	echo "библиотеки готовы: $(ls $P/lib/*.so.* | wc -l) файлов"
}

"lib_$1"
