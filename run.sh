#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

PLATFORM=linux
case "${1:-}" in
linux | windows) PLATFORM=$1; shift ;;
windows-check) exec tools/win_check.sh ;;
android)
	eval "$(python3 -I tools/android_deps.py)"
	APK=$(ls build/release/*-android*.apk 2>/dev/null | head -1)
	[ -n "$APK" ] || { echo "нет APK в build/release — ./build.sh android" >&2; exit 1; }
	adb install -r "$APK"
	exec adb shell monkey -p space.mooncellar.rooftops -c android.intent.category.LAUNCHER 1 ;;
esac

NAME=$(sed -n 's/^#define MEDECOMP_PORT_NAME "\(.*\)"$/\1/p' port/include/port/brand.h)

export MEDECOMP_DEBUG=${MEDECOMP_DEBUG:-1} MEDECOMP_NET_LOG=${MEDECOMP_NET_LOG:-1}

if [ "$PLATFORM" = windows ]; then
	EXE=build/release/windows/$NAME.exe
	[ -f "$EXE" ] || { echo "нет $EXE — ./build.sh windows" >&2; exit 1; }
	command -v wine >/dev/null || { echo "нужен wine" >&2; exit 1; }
	exec wine "$EXE" "$@"
fi

BIN=build/release/linux/$(echo "$NAME" | tr 'A-Z' 'a-z')
[ -x "$BIN" ] || { echo "нет $BIN — ./build.sh linux" >&2; exit 1; }
DATA=${MEDECOMP_DATA:-${XDG_DATA_HOME:-$HOME/.local/share}/mirrors-edge}
mkdir -p "$DATA"
[ -f "$DATA/medecomp.log" ] && mv -f "$DATA/medecomp.log" "$DATA/medecomp.prev.log"
echo "журналы: $DATA/medecomp.log, debug.log, net.log"
"$BIN" "$@" 2>&1 | tee "$DATA/medecomp.log"
exit "${PIPESTATUS[0]}"
