#!/usr/bin/env python3
"""Библиотеки для браузерной сборки (W1): то, чего нет среди портов Emscripten, — из исходников."""

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PACKAGES = [
    ("pixman", "0.46.0", "https://cairographics.org/releases/pixman-0.46.0.tar.gz",
     "02d9ff7b8458ef61731c3d355f854bbf461fd0a4d3563c51f1c1c7b00638050d",
     ["-Dtests=disabled", "-Ddemos=disabled", "-Dgtk=disabled", "-Dlibpng=disabled", "-Dopenmp=disabled",
      "-Dloongson-mmi=disabled", "-Dmmx=disabled", "-Dsse2=disabled", "-Dssse3=disabled", "-Dvmx=disabled",
      "-Darm-simd=disabled", "-Dneon=disabled", "-Da64-neon=disabled", "-Dmips-dspr2=disabled",
      "-Drvv=disabled", "-Dgnu-inline-asm=disabled", "-Dtimers=false"]),
    ("cairo", "1.18.4", "https://cairographics.org/releases/cairo-1.18.4.tar.xz",
     "445ed8208a6e4823de1226a74ca319d3600e83f6369f99b14265006599c32ccb",
     ["-Dfreetype=enabled", "-Dfontconfig=disabled", "-Dpng=enabled", "-Dzlib=disabled", "-Dtests=disabled",
      "-Dxlib=disabled", "-Dxcb=disabled", "-Dquartz=disabled", "-Ddwrite=disabled", "-Dglib=disabled",
      "-Dspectre=disabled", "-Dsymbol-lookup=disabled", "-Dgtk2-utils=disabled", "-Dtee=disabled"]),
]

CMAKE_PACKAGES = [
    ("openal-soft", "1.24.1", "https://openal-soft.org/openal-releases/openal-soft-1.24.1.tar.bz2",
     "0b9883d2e372d4ce66d37b142ab10b606a8a0ed3e873d1e070b1c878b695425a",
     ["-DLIBTYPE=STATIC", "-DALSOFT_UTILS=OFF", "-DALSOFT_EXAMPLES=OFF", "-DALSOFT_TESTS=OFF",
      "-DALSOFT_INSTALL_EXAMPLES=OFF", "-DALSOFT_INSTALL_UTILS=OFF", "-DALSOFT_INSTALL_CONFIG=OFF",
      "-DALSOFT_INSTALL_HRTF_DATA=OFF", "-DALSOFT_INSTALL_AMBDEC_PRESETS=OFF", "-DALSOFT_BACKEND_SDL2=ON",
      "-DALSOFT_REQUIRE_SDL2=ON", "-DALSOFT_BACKEND_WAVE=OFF", "-DALSOFT_BACKEND_PIPEWIRE=OFF",
      "-DALSOFT_BACKEND_PULSEAUDIO=OFF", "-DALSOFT_BACKEND_ALSA=OFF", "-DALSOFT_BACKEND_OSS=OFF",
      "-DALSOFT_BACKEND_JACK=OFF", "-DALSOFT_BACKEND_PORTAUDIO=OFF", "-DALSOFT_BACKEND_SNDIO=OFF",
      "-DALSOFT_BACKEND_SOLARIS=OFF", "-DALSOFT_BACKEND_OPENSL=OFF", "-DALSOFT_BACKEND_OBOE=OFF",
      "-DCMAKE_C_FLAGS=-pthread -DalBufferDataStatic=alsoft_alBufferDataStatic",
      "-DCMAKE_CXX_FLAGS=-pthread -fexceptions -DalBufferDataStatic=alsoft_alBufferDataStatic"],
     ("openal.pc", "-lSDL2", "-sUSE_SDL=2")),
]

FONTS = ("liberation-fonts-ttf-2.1.5",
         "https://github.com/liberationfonts/liberation-fonts/files/7261482/liberation-fonts-ttf-2.1.5.tar.gz",
         "7191c669bf38899f73a2094ed00f7b800553364f90e2637010a69c0e268f25d0")
LIBPNG_PC = """Name: libpng
Description: libpng port from emscripten
Version: 1.6.59
Requires: zlib
Libs: -sUSE_LIBPNG
Cflags: -sUSE_LIBPNG
"""

def fetch(url, tries=4):
    for attempt in range(tries):
        try:
            with urllib.request.urlopen(url, timeout=300) as r:
                return r.read()
        except (urllib.error.URLError, OSError) as e:
            if attempt == tries - 1:
                raise
            print(f"{url}: {e} — ещё раз", file=sys.stderr)
            time.sleep(5 * (attempt + 1))

def tool(name):
    path = shutil.which(name)
    if not path:
        sys.exit(f"нет {name}: " + ("source ~/emsdk/emsdk_env.sh" if name.startswith("em") else "apt install meson ninja-build"))
    return path

def cross_file(path, sysroot):
    em_sysroot = Path(subprocess.run([tool("em-config"), "CACHE"], capture_output=True, text=True,
                                     check=True).stdout.strip()) / "sysroot"
    path.write_text(f"""[binaries]
c = '{tool("emcc")}'
cpp = '{tool("em++")}'
ar = '{tool("emar")}'
ranlib = '{tool("emranlib")}'
strip = '{tool("emstrip")}'
pkg-config = '{tool("pkg-config")}'

[built-in options]
c_args = ['-pthread', '-O2']
cpp_args = ['-pthread', '-O2']
c_link_args = ['-pthread']
cpp_link_args = ['-pthread']

[properties]
needs_exe_wrapper = true
pkg_config_libdir = ['{sysroot}/lib/pkgconfig', '{em_sysroot}/lib/pkgconfig']

[host_machine]
system = 'emscripten'
cpu_family = 'wasm32'
cpu = 'wasm32'
endian = 'little'
""")

def cmake_package(name, version, url, sha, options, pc_fix, cache, src, sysroot):
    stamp = sysroot / f".{name}-{version}"
    if stamp.exists():
        return
    archive = cached(url, sha, cache)
    tree = src / f"{name}-{version}"
    shutil.rmtree(tree, ignore_errors=True)
    with tarfile.open(archive) as tar:
        tar.extractall(src, filter="data" if hasattr(tarfile, "data_filter") else None)
    build = tree / "build-web"
    subprocess.run([tool("emcmake"), "cmake", "-S", str(tree), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release",
                    f"-DCMAKE_INSTALL_PREFIX={sysroot}", "-DCMAKE_INSTALL_LIBDIR=lib",
                    "-DCMAKE_C_FLAGS=-pthread", "-DCMAKE_CXX_FLAGS=-pthread", *options], check=True)
    subprocess.run(["cmake", "--build", str(build), "-j", str(os.cpu_count() or 4)], check=True)
    subprocess.run(["cmake", "--install", str(build)], check=True)
    pc = sysroot / "lib" / "pkgconfig" / pc_fix[0]
    pc.write_text(pc.read_text().replace(pc_fix[1], pc_fix[2]))
    stamp.touch()
    print(f"{name} {version} -> {sysroot}")

def cached(url, sha, cache):
    archive = cache / url.rsplit("/", 1)[1]
    if not archive.exists():
        data = fetch(url)
        if hashlib.sha256(data).hexdigest() != sha:
            sys.exit(f"{url}: SHA-256 не сходится")
        archive.write_bytes(data)
    elif hashlib.sha256(archive.read_bytes()).hexdigest() != sha:
        sys.exit(f"{archive}: SHA-256 не сходится")
    return archive

def fonts(cache, sysroot):
    name, url, sha = FONTS
    out = sysroot / "share" / "fonts"
    if (out / "LiberationSans-Regular.ttf").exists():
        return
    out.mkdir(parents=True, exist_ok=True)
    with tarfile.open(cached(url, sha, cache)) as tar:
        for m in tar.getmembers():
            base = m.name.rsplit("/", 1)[-1]
            if base.startswith("LiberationSans-") or base == "LICENSE":
                (out / base).write_bytes(tar.extractfile(m).read())
    print(f"{name} -> {out}")

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--root", default=str(ROOT / "build" / "web" / "sysroot"))
    a = ap.parse_args()
    sysroot = Path(a.root).resolve()
    work = ROOT / "build" / "web"
    cache, src = work / "cache", work / "src"
    for d in (cache, src, sysroot):
        d.mkdir(parents=True, exist_ok=True)
    tool("meson"), tool("ninja")
    subprocess.run([tool("embuilder"), "build", "sysroot", "sdl2", "sdl2-mt", "libpng", "libpng-mt",
                    "zlib", "freetype"], check=True)
    cross = work / "emscripten.cross"
    cross_file(cross, sysroot)
    (sysroot / "lib" / "pkgconfig").mkdir(parents=True, exist_ok=True)
    (sysroot / "lib" / "pkgconfig" / "libpng.pc").write_text(LIBPNG_PC)
    for name, version, url, sha, options in PACKAGES:
        stamp = sysroot / f".{name}-{version}"
        if stamp.exists():
            continue
        archive = cached(url, sha, cache)
        tree = src / f"{name}-{version}"
        shutil.rmtree(tree, ignore_errors=True)
        with tarfile.open(archive) as tar:
            tar.extractall(src, filter="data" if hasattr(tarfile, "data_filter") else None)
        build = tree / "build-web"
        env = dict(os.environ, PKG_CONFIG_PATH="")
        subprocess.run([tool("meson"), "setup", str(build), str(tree), "--cross-file", str(cross),
                        f"--prefix={sysroot}", "--libdir=lib", "-Ddefault_library=static", "--buildtype=release",
                        "--wrap-mode=nofallback",
                        *options], check=True, env=env)
        subprocess.run([tool("ninja"), "-C", str(build), "install"], check=True, env=env)
        stamp.touch()
        print(f"{name} {version} -> {sysroot}")
    for name, version, url, sha, options, pc_fix in CMAKE_PACKAGES:
        cmake_package(name, version, url, sha, options, pc_fix, cache, src, sysroot)
    fonts(cache, sysroot)
    print(f"web_deps: {', '.join(p[0] for p in PACKAGES + CMAKE_PACKAGES)}, Liberation Sans -> {sysroot}")

if __name__ == "__main__":
    main()
