#!/usr/bin/env python3
"""Инструменты сборки под Android (M1): JDK, Android SDK с NDK, Gradle — в build/android."""

import argparse
import hashlib
import io
import os
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

JDK = ("https://github.com/adoptium/temurin21-binaries/releases/download/jdk-21.0.12.1%2B1/"
       "OpenJDK21U-jdk_x64_linux_hotspot_21.0.12.1_1.tar.gz",
       "sha256", "ce79869e1307ed8ee1e2baa86a412b1eb5b75d10a01006d788a6f968bcfaee94")
CMDLINE_TOOLS = ("https://dl.google.com/android/repository/commandlinetools-linux-13114758_latest.zip",
                 "sha1", "5fdcc763663eefb86a5b8879697aa6088b041e70")
GRADLE = ("https://services.gradle.org/distributions/gradle-8.14.3-bin.zip",
          "sha256", "bd71102213493060956ec229d946beee57158dbd89d0e62b91bca0fa2c5f3531")
NDK = "27.3.13750724"
PACKAGES = ["platform-tools", "platforms;android-35", "build-tools;35.0.0", f"ndk;{NDK}"]
EMULATOR = ["emulator", "system-images;android-35;google_apis;x86_64"]

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

def download(spec):
    url, algo, digest = spec
    print(f"качаю {url}", file=sys.stderr)
    data = fetch(url)
    got = hashlib.new(algo, data).hexdigest()
    if got != digest:
        sys.exit(f"{url}: {algo} {got}, а должно быть {digest}")
    return data

def unpack_top(data, dest, kind):
    tmp = dest.with_name(dest.name + ".part")
    shutil.rmtree(tmp, ignore_errors=True)
    tmp.mkdir(parents=True)
    if kind == "tar":
        with tarfile.open(fileobj=io.BytesIO(data)) as tar:
            tar.extractall(tmp, filter="data")
    else:
        with zipfile.ZipFile(io.BytesIO(data)) as z:
            for info in z.infolist():
                z.extract(info, tmp)
                mode = info.external_attr >> 16
                if mode & 0o111:
                    os.chmod(tmp / info.filename, mode & 0o777)
    (top,) = list(tmp.iterdir())
    shutil.rmtree(dest, ignore_errors=True)
    top.rename(dest)
    tmp.rmdir()

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--root", default=str(ROOT / "build" / "android"))
    ap.add_argument("--emulator", action="store_true", help="эмулятор и образ x86_64 для проверок")
    ap.add_argument("--accept-licenses", action="store_true", help="принять лицензии пакетов SDK")
    args = ap.parse_args()
    root = Path(args.root).resolve()
    root.mkdir(parents=True, exist_ok=True)

    jdk, sdk, gradle = root / "jdk", root / "sdk", root / "gradle"
    if not (jdk / "bin" / "java").exists():
        unpack_top(download(JDK), jdk, "tar")
    if not (gradle / "bin" / "gradle").exists():
        unpack_top(download(GRADLE), gradle, "zip")
    tools = sdk / "cmdline-tools" / "latest"
    if not (tools / "bin" / "sdkmanager").exists():
        unpack_top(download(CMDLINE_TOOLS), tools, "zip")

    env = dict(os.environ, JAVA_HOME=str(jdk), ANDROID_HOME=str(sdk), ANDROID_SDK_ROOT=str(sdk))
    sdkmanager = [str(tools / "bin" / "sdkmanager"), f"--sdk_root={sdk}"]
    if args.accept_licenses:
        subprocess.run(sdkmanager + ["--licenses"], input=b"y\n" * 20, env=env, check=True,
                       stdout=subprocess.DEVNULL)
    wanted = PACKAGES + (EMULATOR if args.emulator else [])
    missing = [p for p in wanted if not (sdk / p.replace(";", "/")).exists()]
    if missing:
        subprocess.run(sdkmanager + missing, env=env, check=True)

    print(f"export JAVA_HOME={jdk}")
    print(f"export ANDROID_HOME={sdk}")
    print(f"export ANDROID_NDK={sdk / 'ndk' / NDK}")
    print(f"export PATH={jdk / 'bin'}:{gradle / 'bin'}:{sdk / 'platform-tools'}:$PATH")
    return 0

if __name__ == "__main__":
    sys.exit(main())
