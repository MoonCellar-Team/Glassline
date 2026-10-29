#!/usr/bin/env python3
"""Запуск APK или существующего Android SDK emulator; инструменты не устанавливаются."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import zipfile

ROOT = Path(__file__).resolve().parent.parent
IMAGE = 'system-images;android-35;google_apis;x86_64'

def sdk_env():
    env = os.environ.copy()
    sdk = Path(env.get('ANDROID_HOME') or env.get('ANDROID_SDK_ROOT') or ROOT / 'build/android/sdk').resolve()
    env.update(ANDROID_HOME=str(sdk), ANDROID_SDK_ROOT=str(sdk))
    java = ROOT / 'build/android/jdk'
    if not env.get('JAVA_HOME') and (java / 'bin/java').exists():
        env['JAVA_HOME'] = str(java)
    return sdk, env

def tool(sdk, relative, name):
    local = sdk / relative
    found = str(local) if local.is_file() else shutil.which(name)
    if not found:
        raise RuntimeError(f'Нет {name}. Подготовьте SDK: python3 -I tools/android_deps.py --emulator')
    return found

def output(command, env):
    result = subprocess.run(command, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=30)
    if result.returncode:
        raise RuntimeError((result.stderr or result.stdout).strip() or f'Не удалось выполнить {command[0]}')
    return result.stdout.strip()

def avds(emulator, env):
    found = {name: env.copy() for name in output([emulator, '-list-avds'], env).splitlines() if name.strip()}
    project_env = dict(env, ANDROID_AVD_HOME=str(ROOT / 'build/android/avd'))
    if Path(project_env['ANDROID_AVD_HOME']).is_dir():
        found.update({name: project_env for name in output([emulator, '-list-avds'], project_env).splitlines() if name.strip()})
    return found

def select_avd(name, emulator, sdk, env):
    found = avds(emulator, env)
    if name:
        if name not in found:
            raise RuntimeError(f'Нет AVD «{name}». Доступны: {", ".join(found) or "нет"}.')
        return name, found[name]
    if 'glassline' in found:
        return 'glassline', found['glassline']
    if len(found) == 1:
        name = next(iter(found))
        return name, found[name]
    if found:
        raise RuntimeError(f'Укажите AVD: ./run.sh android-emulator <имя>. Доступны: {", ".join(found)}.')
    if not (sdk / IMAGE.replace(';', '/')).is_dir():
        raise RuntimeError('Нет образа Android 35 x86_64. Подготовьте его: python3 -I tools/android_deps.py --emulator')
    manager = tool(sdk, 'cmdline-tools/latest/bin/avdmanager', 'avdmanager')
    home = ROOT / 'build/android/avd'
    home.mkdir(parents=True, exist_ok=True)
    local_env = dict(env, ANDROID_AVD_HOME=str(home))
    subprocess.run([manager, 'create', 'avd', '--name', 'glassline', '--package', IMAGE,
                    '--device', 'pixel_tablet', '--path', str(home / 'glassline.avd')],
                   input='no\n', text=True, env=local_env, check=True)
    return 'glassline', local_env

def apk_abis(apk):
    with zipfile.ZipFile(apk) as archive:
        return {name.split('/')[1] for name in archive.namelist()
                if name.startswith('lib/') and name.endswith('.so')}

def choose_apk(abis, explicit=None):
    choices = [Path(explicit).resolve()] if explicit else (
        list((ROOT / 'build/release').glob('*-android*.apk')) +
        list((ROOT / 'build/android').glob('*-android*.apk')))
    matching = []
    for apk in choices:
        if not apk.is_file():
            raise RuntimeError(f'Нет APK: {apk}')
        if apk_abis(apk).intersection(abis):
            matching.append(apk)
    if not matching:
        raise RuntimeError(f'Нет APK для {", ".join(abis)}. Соберите android/build.sh x86_64 '
                           '(для x86_64-эмулятора) или ./build.sh android (для ARM-устройства).')
    return max(matching, key=lambda p: p.stat().st_mtime).resolve()

def install(adb, env, apk=None, serial=None):
    serial = serial or env.get('ANDROID_SERIAL')
    if not serial:
        devices = [line.split()[0] for line in output([adb, 'devices'], env).splitlines()[1:]
                   if len(line.split()) >= 2 and line.split()[1] == 'device']
        if len(devices) != 1:
            raise RuntimeError('Подключите одно устройство/эмулятор или укажите --serial <adb serial>.')
        serial = devices[0]
    command = [adb, '-s', serial]
    abis = output(command + ['shell', 'getprop', 'ro.product.cpu.abilist'], env).split(',')
    apk = choose_apk(abis, apk)
    print(f'Установка Glassline: {apk.name} → {serial}', flush=True)
    subprocess.run(command + ['install', '-r', str(apk)], env=env, check=True)
    subprocess.run(command + ['shell', 'monkey', '-p', 'space.mooncellar.rooftops',
                              '-c', 'android.intent.category.LAUNCHER', '1'], env=env, check=True)

def running_avd(adb, name, env):
    for line in output([adb, 'devices'], env).splitlines()[1:]:
        fields = line.split()
        if len(fields) < 2 or not fields[0].startswith('emulator-') or fields[1] != 'device':
            continue
        try:
            if output([adb, '-s', fields[0], 'emu', 'avd', 'name'], env).splitlines()[0] == name:
                return fields[0]
        except (RuntimeError, subprocess.TimeoutExpired):
            continue
    return None

def wait_for_boot(adb, serial, env, process, timeout=180):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process and process.poll() is not None:
            raise RuntimeError('Эмулятор завершился до загрузки Android.')
        try:
            result = subprocess.run([adb, '-s', serial, 'shell', 'getprop', 'sys.boot_completed'],
                                    env=env, text=True, capture_output=True, timeout=5)
            if result.returncode == 0 and result.stdout.strip() == '1':
                return
        except subprocess.TimeoutExpired:
            pass
        time.sleep(1)
    raise RuntimeError(f'Android не загрузился за {timeout} с: {serial}.')

def launch_emulator(emulator, adb, name, env, extra, apk=None):
    serial = running_avd(adb, name, env)
    process = None
    if not serial:
        devices = output([adb, 'devices'], env)
        if '-ports' in extra:
            raise RuntimeError('Для выбора порта используйте -port, а не -ports.')
        if '-port' in extra:
            try:
                port = int(extra[extra.index('-port') + 1])
            except (IndexError, ValueError):
                raise RuntimeError('После -port нужен чётный номер порта.') from None
            if not 1024 <= port <= 65534 or port % 2:
                raise RuntimeError('Порт эмулятора должен быть чётным, от 1024 до 65534.')
            if f'emulator-{port}\t' in devices:
                raise RuntimeError(f'Порт {port} уже занят другим Android-эмулятором.')
        else:
            port = next((n for n in range(5554, 5586, 2) if f'emulator-{n}\t' not in devices), None)
            if port is None:
                raise RuntimeError('Нет свободного порта Android-эмулятора.')
            extra = [*extra, '-port', str(port)]
        serial = f'emulator-{port}'
        process = subprocess.Popen([emulator, '-avd', name, *extra], env=env)
    try:
        print(f'Ожидание загрузки Android: {serial}', flush=True)
        wait_for_boot(adb, serial, env, process)
        install(adb, env, apk, serial)
    except BaseException:
        if process and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        raise
    return process.wait() if process else 0

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('device', 'emulator'))
    parser.add_argument('avd', nargs='?', help='имя AVD; без имени выбирается единственный или glassline')
    parser.add_argument('--check', action='store_true', help='проверить инструменты без запуска и создания AVD')
    parser.add_argument('--list-avds', action='store_true')
    parser.add_argument('--apk', help='APK для установки на устройство')
    parser.add_argument('--serial', help='adb serial устройства или эмулятора')
    parser.add_argument('--no-app', action='store_true', help='запустить только эмулятор без установки Glassline')
    tokens = list(sys.argv[1:] if argv is None else argv)
    extra = []
    if '--' in tokens:
        split = tokens.index('--')
        tokens, extra = tokens[:split], tokens[split + 1:]
    args = parser.parse_args(tokens)
    if args.mode == 'device' and (extra or args.avd or args.list_avds or args.no_app):
        parser.error('для устройства доступны --apk, --serial и --check')
    sdk, env = sdk_env()
    if args.mode == 'device':
        adb = tool(sdk, 'platform-tools/adb', 'adb')
        output([adb, 'version'], env)
        if not args.check:
            install(adb, env, args.apk, args.serial)
    else:
        emulator = tool(sdk, 'emulator/emulator', 'emulator')
        output([emulator, '-version'], env)
        if not args.no_app and not args.list_avds:
            adb = tool(sdk, 'platform-tools/adb', 'adb')
            output([adb, 'version'], env)
        if not args.list_avds and ('-accel' not in extra or extra[extra.index('-accel') + 1:][:1] != ['off']):
            output([emulator, '-accel-check'], env)
        if args.list_avds:
            print('\n'.join(avds(emulator, env)))
            return 0
        if not args.check:
            name, env = select_avd(args.avd, emulator, sdk, env)
            print(f'Запуск Android AVD «{name}»', flush=True)
            if args.no_app:
                os.execve(emulator, [emulator, '-avd', name, *extra], env)
            return launch_emulator(emulator, adb, name, env, extra, args.apk)
    if args.check:
        print('Инструменты Android работают.')
    return 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        print(f'Android: {error}', file=sys.stderr)
        sys.exit(1)
