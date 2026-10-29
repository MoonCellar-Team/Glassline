#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"

android_run() {
	local mode=$1 platform=$2 reason probe ready=0
	shift 2
	if reason=$(python3 -I tools/android_run.py "$mode" --check "$@" 2>&1); then ready=1; fi
	if [ "$ready" = 1 ] &&
		{ [ -z "${MEDECOMP_DISTROBOX:-${PORT_DISTROBOX:-}}" ] || [ "${PORT_BUILD_IN_DISTROBOX:-}" = 1 ] || [ -n "${CONTAINER_ID:-}" ]; }; then
		exec python3 -I tools/android_run.py "$mode" "$@"
	fi
	if [ "$ready" = 0 ]; then printf '%s\n' "$reason" >&2; fi
	source tools/distrobox_build.sh
	printf -v probe '%q ' python3 -I tools/android_run.py "$mode" --check "$@"
	PORT_DISTROBOX=${MEDECOMP_DISTROBOX:-${PORT_DISTROBOX:-}} PORT_DISTROBOX_ACTION="Запуск" distrobox_build 0 \
		bash -c "$probe" -- bash ./run.sh "$platform" "$@"
	exit $?
}

PLATFORM=linux
case "${1:-}" in
linux | windows) PLATFORM=$1; shift ;;
windows-check) exec tools/win_check.sh ;;
android)
	shift
	if [ "${1:-}" = --emulator ]; then shift; android_run emulator android-emulator "$@"; fi
	android_run device android "$@" ;;
android-emulator) shift; android_run emulator android-emulator "$@" ;;
web)
	shift
	[ -f build/release/web/index.html ] || { echo "нет build/release/web — ./build.sh web" >&2; exit 1; }
	exec python3 -I tools/web_serve.py --dir build/release/web --proxy-game "$@" ;;
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
