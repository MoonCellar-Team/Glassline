#!/usr/bin/env python3
"""Папка игры для Windows (J1): <имя порта>.exe, DLL, настройки шрифтов, строки, лицензии — и zip."""

import argparse
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import zipfile

RUNTIME_DLLS = ["libcurl-4.dll", "libEGL.dll", "libGLESv2.dll"]
PREFIX = "mingw-w64-clang-x86_64-"
README = """{port} — PC-порт для Mirror's Edge (iPad) от MoonCellar Team

Нужна своя копия игры (.ipa с iPad, версия 1.1.48 или новее). Запуск — {port}.exe: откроется лаунчер,
в нём укажите путь к своему .ipa. Сохранения, настройки и журнал (medecomp.log) — в %APPDATA%\\mirrors-edge.

Нужна Windows 10 или 11 (64 бита). Сторонние библиотеки и их лицензии — в папке licenses.
"""

def port_name():
    text = open("port/include/port/brand.h", encoding="utf-8").read()
    found = re.search(r'^#define MEDECOMP_PORT_NAME "(.*)"$', text, re.M)
    if not found:
        sys.exit("нет MEDECOMP_PORT_NAME в port/include/port/brand.h")
    return found.group(1)

def objdump():
    found = (sorted(glob.glob("build/win/llvm-mingw-*/bin/llvm-objdump")) or
             [shutil.which("x86_64-w64-mingw32-objdump") or shutil.which("llvm-objdump")])
    if not found or not found[-1]:
        sys.exit("нет llvm-objdump (llvm-mingw в build/win)")
    return found[-1]

def imports(tool, path):
    out = subprocess.run([tool, "-p", path], capture_output=True, text=True, check=True).stdout
    return [line.split("DLL Name:")[1].strip() for line in out.splitlines() if "DLL Name:" in line]

def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--build", default="build/win64")
    parser.add_argument("--sysroot", default="build/win/sysroot")
    port = port_name()
    parser.add_argument("--name", default=port.lower() + "-windows-x64")
    args = parser.parse_args()
    bindir = os.path.join(args.sysroot, "clang64", "bin")
    owners = json.load(open(os.path.join(args.sysroot, "owners.json")))
    exe = os.path.join(args.build, "medecomp.exe")
    out = os.path.join(args.build, "dist", args.name)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    shutil.copy2(exe, os.path.join(out, port + ".exe"))

    tool = objdump()
    ffmpeg_bin = os.path.join(args.build, "ffmpeg", "bin")
    own = glob.glob(os.path.join(ffmpeg_bin, "*.dll")) + glob.glob(os.path.join(args.build, "lib", "*.dll"))
    dlls, todo, system = set(), [exe] + [os.path.join(bindir, d) for d in RUNTIME_DLLS] + own, set()
    dlls.update(RUNTIME_DLLS)
    while todo:
        for dll in imports(tool, todo.pop()):
            if os.path.exists(os.path.join(ffmpeg_bin, dll)) or os.path.exists(os.path.join(args.build, "lib", dll)):
                continue
            if os.path.exists(os.path.join(bindir, dll)):
                if dll not in dlls:
                    dlls.add(dll)
                    todo.append(os.path.join(bindir, dll))
            else:
                system.add(dll.lower())
    for dll in sorted(dlls):
        shutil.copy2(os.path.join(bindir, dll), out)

    for dll in glob.glob(os.path.join(args.build, "ffmpeg", "bin", "*.dll")):
        shutil.copy2(dll, out)
        dlls.add(os.path.basename(dll))
    ffmpeg_license = os.path.join(args.build, "ffmpeg", "LICENSE.txt")
    if os.path.exists(ffmpeg_license):
        os.makedirs(os.path.join(out, "licenses", "ffmpeg"), exist_ok=True)
        shutil.copy2(ffmpeg_license, os.path.join(out, "licenses", "ffmpeg", "LICENSE.txt"))

    gns = os.path.join(args.build, "lib", "libGameNetworkingSockets.dll")
    if os.path.exists(gns):
        shutil.copy2(gns, out)
        for lic in glob.glob(os.path.join(args.build, "lib", "*-LICENSE.txt")):
            os.makedirs(os.path.join(out, "licenses"), exist_ok=True)
            shutil.copy2(lic, os.path.join(out, "licenses"))

    shutil.copytree(os.path.join(args.sysroot, "clang64", "etc", "fonts"), os.path.join(out, "etc", "fonts"))
    os.makedirs(os.path.join(out, "config"))
    shutil.copy2("config/string_overrides.tsv", os.path.join(out, "config"))
    shutil.copytree("config/achievements", os.path.join(out, "config", "achievements"))
    shutil.copytree(os.path.join(args.build, "branding"), os.path.join(out, "branding"))

    packages = {owners.get("bin/" + dll) for dll in dlls} - {None}
    for path, package in owners.items():
        if package in packages and path.startswith("share/licenses/"):
            dest = os.path.join(out, "licenses", package[len(PREFIX):], os.path.basename(path))
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            shutil.copy2(os.path.join(args.sysroot, "clang64", path), dest)
    with open(os.path.join(out, "README.txt"), "w", newline="\r\n") as f:
        f.write(README.format(port=port))

    archive = os.path.join(args.build, args.name + ".zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for dirpath, _, files in os.walk(out):
            for name in files:
                full = os.path.join(dirpath, name)
                z.write(full, os.path.join(args.name, os.path.relpath(full, out)))
    size = os.path.getsize(archive) / 1048576
    print(f"DLL: {len(dlls)}, пакетов с лицензиями: {len(packages)}; системные: {', '.join(sorted(system))}")
    print(f"готово: {out} и {archive} ({size:.0f} МБ)")

if __name__ == "__main__":
    main()
