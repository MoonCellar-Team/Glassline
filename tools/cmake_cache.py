#!/usr/bin/env python3
"""Очистить перенесённую CMake-сборку: cmake_cache.py <исходники> <каталог в build/>."""
import argparse
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parent.parent

def relocate_pkgconfig(sysroot):
    sysroot = Path(sysroot).resolve()
    allowed = (ROOT / 'build').resolve()
    if not sysroot.is_relative_to(allowed) or sysroot == allowed:
        raise ValueError(f'Отказ менять sysroot вне build/: {sysroot}')
    suffix = '/' + sysroot.relative_to(ROOT.resolve()).as_posix()
    changed = 0
    for folder in ('lib/pkgconfig', 'share/pkgconfig'):
        for path in (sysroot / folder).glob('*.pc'):
            text = path.read_text()
            prefixes = [line.split('=', 1)[1] for line in text.splitlines() if line.startswith('prefix=')]
            if not prefixes:
                continue
            old = prefixes[0]
            if old != str(sysroot) and old.endswith(suffix):
                path.write_text(text.replace(old, str(sysroot)))
                changed += 1
    if changed:
        print(f'pkg-config: обновлены пути в {changed} файлах {sysroot}', flush=True)
    return changed

def prepare(source, build):
    source, build = Path(source).resolve(), Path(build)
    if build.is_symlink():
        raise ValueError(f'Папка сборки — символическая ссылка: {build}')
    build = build.resolve()
    cache = build / 'CMakeCache.txt'
    if not cache.is_file():
        return False
    values = {}
    for line in cache.read_text().splitlines():
        for key in ('CMAKE_HOME_DIRECTORY', 'CMAKE_CACHEFILE_DIR'):
            if line.startswith(key + ':') and '=' in line:
                values[key] = Path(line.split('=', 1)[1]).resolve()
    if not values:
        raise ValueError(f'Не удалось определить пути в {cache}; сборка не удалена.')
    if values.get('CMAKE_HOME_DIRECTORY', source) == source and values.get('CMAKE_CACHEFILE_DIR', build) == build:
        return False
    allowed = (ROOT / 'build').resolve()
    if build == allowed or not build.is_relative_to(allowed) or source.is_relative_to(build):
        raise ValueError(f'Отказ очищать каталог вне отдельной сборки в {allowed}: {build}')
    print(f'CMake: путь проекта изменился, очищаем генерируемую сборку {build}', flush=True)
    shutil.rmtree(build)
    return True

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source')
    parser.add_argument('build')
    parser.add_argument('--sysroot', help='исправить пути перенесённых .pc только этого sysroot')
    args = parser.parse_args()
    try:
        changed = relocate_pkgconfig(args.sysroot) if args.sysroot else 0
        reset = prepare(args.source, args.build)
        if changed and not reset:
            (Path(args.build) / 'CMakeCache.txt').unlink(missing_ok=True)
    except (OSError, ValueError) as error:
        print(f'CMake: {error}', file=sys.stderr)
        return 1
    return 0

if __name__ == '__main__':
    sys.exit(main())
