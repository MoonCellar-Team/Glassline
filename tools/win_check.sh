#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

NAME=$(sed -n 's/^#define MEDECOMP_PORT_NAME "\(.*\)"$/\1/p' port/include/port/brand.h)
EXE=$PWD/build/release/windows/$NAME.exe
[ -f "$EXE" ] || { echo "нет $EXE — ./build.sh windows" >&2; exit 1; }
for tool in wine gamescope; do
	command -v "$tool" > /dev/null || { echo "нужен $tool" >&2; exit 1; }
done
GAME=${MEDECOMP_GAME:-$(python3 -I -c '
import json, os
base = os.environ.get("XDG_DATA_HOME") or os.path.expanduser("~/.local/share")
try:
    print(json.load(open(os.path.join(base, "mirrors-edge", "settings.json"))).get("game", ""))
except Exception:
    pass
')}
[ -f "$GAME" ] || { echo "нет копии игры: MEDECOMP_GAME=<путь к .ipa>" >&2; exit 1; }

OUT=$PWD/build/wine-check
rm -rf "${OUT:?}/data" "$OUT/menu.png" "$OUT/run.log"
mkdir -p "$OUT/data"
export WINEPREFIX=$OUT/prefix WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="
if [ ! -d "$WINEPREFIX/drive_c" ]; then
	echo "папка wine — один раз, около минуты"
	wineboot -i > /dev/null 2>&1
fi
win() { wine winepath -w "$1" 2> /dev/null | tr -d '\r'; }
export MEDECOMP_GAME_WIN=$(win "$GAME") MEDECOMP_DATA_WIN=$(win "$OUT/data") EXE OUT
export MEDECOMP_CHECK_SCRIPT="9:tap@512/300,13:tap@512/300,17:tap@512/300,22:shot"

echo "запуск $NAME.exe под wine без окна…"
timeout 180 gamescope --backend headless -W 1024 -H 768 -- bash -c '
	MEDECOMP_GAME="$MEDECOMP_GAME_WIN" MEDECOMP_DATA="$MEDECOMP_DATA_WIN" MEDECOMP_SHOT_DIR="$MEDECOMP_DATA_WIN" \
		MEDECOMP_SCRIPT="$MEDECOMP_CHECK_SCRIPT" ALSOFT_DRIVERS=null wine "$EXE" --no-launcher > "$OUT/run.log" 2>&1 &
	for _ in $(seq 1 120); do
		[ -s "$OUT/data/shot-1.png" ] && break
		sleep 1
	done
	sleep 1
	wineserver -k
' > "$OUT/gamescope.log" 2>&1 || true

if [ -s "$OUT/data/shot-1.png" ] && [ "$(stat -c %s "$OUT/data/shot-1.png")" -gt 20000 ]; then
	cp "$OUT/data/shot-1.png" "$OUT/menu.png"
	echo "до меню: да — кадр $OUT/menu.png"
else
	echo "до меню: нет — журнал $OUT/run.log" >&2
	tail -20 "$OUT/run.log" >&2 || true
	exit 1
fi
