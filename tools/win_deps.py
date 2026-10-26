#!/usr/bin/env python3
"""Библиотеки для сборки под Windows (J1): пакеты MSYS2, окружение clang64, со всеми зависимостями."""

import argparse
import json
import hashlib
import io
import os
import re
import sys
import tarfile
import struct
import time
import urllib.error
import urllib.request
import zlib

try:
    from compression import zstd

    def unzstd(data):
        return zstd.decompress(data)

except ImportError:
    import subprocess

    def unzstd(data):
        try:
            return subprocess.run(["zstd", "-dc"], input=data, stdout=subprocess.PIPE, check=True).stdout
        except FileNotFoundError:
            sys.exit("пакеты MSYS2 — в .zst: нужен Python 3.14 или утилита zstd (apt install zstd)")

REPO = "https://mirror.msys2.org/mingw/clang64/"
LLVM_MINGW = "20261006"
LLVM_MINGW_FILE = f"llvm-mingw-{LLVM_MINGW}-ucrt-ubuntu-22.04-x86_64"
LLVM_MINGW_SHA256 = "5f9c6ed95b2d4bdb2869a488c5fd5857fbdabcf288a0aa3eb1da43f6a08d8ab4"
PREFIX = "mingw-w64-clang-x86_64-"
PACKAGES = ["SDL2", "cairo", "freetype", "fontconfig", "zlib", "openal", "curl", "gles-headers", "angleproject"]
D3DCOMPILER_PACKAGE = "Microsoft.Windows.SDK.CPP"
D3DCOMPILER_VERSION = "10.0.28000.2705"
D3DCOMPILER_MEMBER = "c/Redist/D3D/x64/d3dcompiler_47.dll"
D3DCOMPILER_SHA256 = "3e56c1a0867aa04cbca948b3664fcac5d51275865cd044a45a3fd6ce636ee914"
SDK_LICENSE = "https://download.microsoft.com/download/0/F/F/0FF2B061-47DD-4F55-89B6-FD1D8C44F14D/sdk_license.rtf"
SDK_LICENSE_SHA256 = "dd07eb178e00c6bba4148457fc00ff77cd4887eb521d504186fe59c9ec8bbe62"

def fetch(url, tries=4):
    for attempt in range(tries):
        try:
            with urllib.request.urlopen(url, timeout=120) as r:
                return r.read()
        except (urllib.error.URLError, OSError) as e:
            if attempt == tries - 1:
                raise
            print(f"{url}: {e} — ещё раз", file=sys.stderr)
            time.sleep(5 * (attempt + 1))

def fetch_range(url, first, last=None):
    spec = f"bytes={first}-" if last is None and first >= 0 else f"bytes={first}" if first < 0 else f"bytes={first}-{last}"
    for attempt in range(4):
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers={"Range": spec}), timeout=120) as r:
                if r.status != 206:
                    sys.exit(f"{url}: сервер не отдаёт части файла (HTTP {r.status})")
                return r.read()
        except (urllib.error.URLError, OSError) as e:
            if attempt == 3:
                raise
            print(f"{url}: {e} — ещё раз", file=sys.stderr)
            time.sleep(5 * (attempt + 1))

def zip_member(url, name):
    tail = fetch_range(url, -65536)
    at = tail.rfind(b"PK\x05\x06")
    if at < 0:
        sys.exit(f"{url}: не zip")
    count, size, offset = struct.unpack("<HII", tail[at + 10:at + 20])
    if offset == 0xFFFFFFFF:
        loc = tail.rfind(b"PK\x06\x07")
        eocd64 = struct.unpack("<Q", tail[loc + 8:loc + 16])[0]
        rec = fetch_range(url, eocd64, eocd64 + 55)
        size, offset = struct.unpack("<QQ", rec[40:56])
    directory = fetch_range(url, offset, offset + size - 1)
    pos = 0
    while pos < len(directory):
        method, packed, unpacked = struct.unpack("<H8xII", directory[pos + 10:pos + 28])
        name_len, extra_len, comment_len = struct.unpack("<HHH", directory[pos + 28:pos + 34])
        header = struct.unpack("<I", directory[pos + 42:pos + 46])[0]
        entry = directory[pos + 46:pos + 46 + name_len].decode()
        pos += 46 + name_len + extra_len + comment_len
        if entry != name:
            continue
        local = fetch_range(url, header, header + 29)
        skip = 30 + sum(struct.unpack("<HH", local[26:30]))
        data = fetch_range(url, header + skip, header + skip + packed - 1)
        if method == 8:
            data = zlib.decompress(data, -15)
        elif method != 0:
            sys.exit(f"{name}: сжатие {method} не поддерживается")
        return data
    sys.exit(f"{url}: нет {name}")

def ensure_d3dcompiler(root):
    dest = os.path.join(root, "d3dcompiler")
    dll, license = os.path.join(dest, "d3dcompiler_47.dll"), os.path.join(dest, "sdk_license.rtf")
    if all(os.path.exists(p) for p in (dll, license)):
        if hashlib.sha256(open(dll, "rb").read()).hexdigest() == D3DCOMPILER_SHA256:
            return
    os.makedirs(dest, exist_ok=True)
    package = D3DCOMPILER_PACKAGE.lower()
    url = (f"https://api.nuget.org/v3-flatcontainer/{package}/{D3DCOMPILER_VERSION}/"
           f"{package}.{D3DCOMPILER_VERSION}.nupkg")
    data = zip_member(url, D3DCOMPILER_MEMBER)
    if hashlib.sha256(data).hexdigest() != D3DCOMPILER_SHA256:
        sys.exit(f"{D3DCOMPILER_MEMBER}: SHA256 не сходится")
    text = fetch(SDK_LICENSE)
    if hashlib.sha256(text).hexdigest() != SDK_LICENSE_SHA256:
        sys.exit(f"{SDK_LICENSE}: SHA256 не сходится")
    with open(dll, "wb") as f:
        f.write(data)
    with open(license, "wb") as f:
        f.write(text)
    print(f"d3dcompiler_47.dll: {D3DCOMPILER_PACKAGE} {D3DCOMPILER_VERSION}, {dest}")

def read_db(data):
    packages = {}
    with tarfile.open(fileobj=io.BytesIO(unzstd(data))) as tar:
        for member in tar:
            if not member.name.endswith("/desc"):
                continue
            text = tar.extractfile(member).read().decode()
            fields = {}
            for block in text.strip().split("\n\n"):
                lines = block.strip().split("\n")
                fields[lines[0].strip("%")] = lines[1:]
            packages[fields["NAME"][0]] = fields
    return packages

def resolve(packages, wanted):
    provides = {}
    for name, fields in packages.items():
        for p in fields.get("PROVIDES", []):
            provides.setdefault(re.split(r"[<>=]", p)[0], name)
    order, seen = [], set()

    def visit(name):
        base = re.split(r"[<>=]", name)[0]
        real = base if base in packages else provides.get(base)
        if real is None:
            sys.exit(f"нет пакета {base}")
        if real in seen:
            return
        seen.add(real)
        for dep in packages[real].get("DEPENDS", []):
            visit(dep)
        order.append(real)

    for name in wanted:
        visit(name if name.startswith(PREFIX) else PREFIX + name)
    return order

def ensure_mingw(folder):
    dest = os.path.join(folder, LLVM_MINGW_FILE)
    if os.path.exists(os.path.join(dest, "bin", "x86_64-w64-mingw32-clang++")):
        return
    os.makedirs(folder, exist_ok=True)
    url = f"https://github.com/mstorsjo/llvm-mingw/releases/download/{LLVM_MINGW}/{LLVM_MINGW_FILE}.tar.xz"
    data = fetch(url)
    if hashlib.sha256(data).hexdigest() != LLVM_MINGW_SHA256:
        sys.exit(f"{LLVM_MINGW_FILE}.tar.xz: SHA256 не сходится")
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:xz") as tar:
        tar.extractall(folder, filter="tar")
    print(f"llvm-mingw {LLVM_MINGW}: {dest}")

def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--root", default="build/win/sysroot")
    parser.add_argument("--mingw")
    parser.add_argument("packages", nargs="*")
    args = parser.parse_args()
    if args.mingw:
        ensure_mingw(args.mingw)
    root = os.path.abspath(args.root)
    cache = os.path.join(root, ".packages")
    os.makedirs(cache, exist_ok=True)

    packages = read_db(fetch(REPO + "clang64.db"))
    order = resolve(packages, args.packages or PACKAGES)
    print(f"пакетов: {len(order)}")
    owners = {}
    for name in order:
        fields = packages[name]
        filename = fields["FILENAME"][0]
        path = os.path.join(cache, filename)
        if not os.path.exists(path):
            data = fetch(REPO + filename)
            if hashlib.sha256(data).hexdigest() != fields["SHA256SUM"][0]:
                sys.exit(f"{filename}: SHA256 не сходится")
            with open(path, "wb") as f:
                f.write(data)
            print(f"  {name} {fields['VERSION'][0]}")
        with tarfile.open(fileobj=io.BytesIO(unzstd(open(path, "rb").read()))) as tar:
            members = [m for m in tar if m.name.startswith("clang64/") and (m.isfile() or m.isdir() or m.issym())]
            tar.extractall(root, members=members, filter="data")
            for m in members:
                if m.isfile():
                    owners[m.name[len("clang64/"):]] = name
    with open(os.path.join(root, "owners.json"), "w") as f:
        json.dump(owners, f, indent=0, sort_keys=True)
    if not args.packages:
        ensure_d3dcompiler(root)
    print(f"готово: {root}/clang64")

if __name__ == "__main__":
    main()
