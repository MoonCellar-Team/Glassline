#!/usr/bin/env bash

distrobox_build() {
	local check_only=$1 listing name reason project_dir=$PWD
	shift
	local probe=() command=() candidates=()
	while [ "$#" -gt 0 ] && [ "$1" != -- ]; do probe+=("$1"); shift; done
	if [ "$#" = 0 ] || [ "${#probe[@]}" = 0 ]; then
		echo "distrobox_build: нужны проверочная команда, -- и команда сборки." >&2
		return 2
	fi
	shift
	command=("$@")
	if [ "${#command[@]}" = 0 ] || { [ "$check_only" != 0 ] && [ "$check_only" != 1 ]; }; then
		echo "distrobox_build: неверные аргументы." >&2
		return 2
	fi
	if [ "${PORT_BUILD_IN_DISTROBOX:-}" = 1 ] || [ -n "${CONTAINER_ID:-}" ] || [ -e /run/.containerenv ]; then
		echo "Текущее контейнерное окружение не подходит; вложенный Distrobox не запускается." >&2
		return 1
	fi
	if ! command -v distrobox > /dev/null; then
		echo "Нет distrobox: подготовьте инструменты на хосте или существующий Distrobox для сборки." >&2
		return 1
	fi
	if [ -n "${PORT_DISTROBOX:-}" ]; then
		candidates=("$PORT_DISTROBOX")
	else
		listing=$(distrobox list --no-color) || { echo "Не удалось получить список Distrobox." >&2; return 1; }
		while IFS= read -r name; do
			[ -z "$name" ] || candidates+=("$name")
		done < <(printf '%s\n' "$listing" | awk -F '|' 'NR > 1 && NF >= 3 {gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2); print $2}')
	fi
	for name in "${candidates[@]}"; do
		if reason=$(distrobox enter --name "$name" --no-tty -- bash -c \
			'cd -- "$1" && shift && PORT_BUILD_IN_DISTROBOX=1 exec "$@"' bash "$project_dir" "${probe[@]}" 2>&1); then
			echo "${PORT_DISTROBOX_ACTION:-Сборка} через Distrobox «$name»"
			[ "$check_only" = 0 ] || return 0
			distrobox enter --name "$name" --no-tty -- bash -c \
				'cd -- "$1" && shift && PORT_BUILD_IN_DISTROBOX=1 exec "$@"' bash "$project_dir" "${command[@]}"
			return $?
		fi
		printf 'Distrobox «%s» не подходит:\n%s\n' "$name" "$reason" >&2
	done
	echo "Нет подходящего Distrobox. Подготовьте контейнер и выберите его через PORT_DISTROBOX=<имя>." >&2
	return 1
}
