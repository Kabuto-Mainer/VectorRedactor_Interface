#!/usr/bin/env bash

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIST="${1:-$SCRIPT_DIR/plugins_list.txt}"
shift || true

if [ ! -f "$LIST" ]; then
    echo "Список плагинов не найден: $LIST" >&2
    exit 1
fi

LIST_DIR="$(cd "$(dirname "$LIST")" && pwd)"
LIST="$(cd "$LIST_DIR" && pwd)/$(basename "$LIST")"

built=0
skipped=0
failed=0
built_dirs=()
skipped_dirs=()
failed_dirs=()
seen_dirs=""

while IFS= read -r line || [ -n "$line" ]; do
    line="${line%$'\r'}"
    line="${line#"${line%%[![:space:]]*}"}"
    [[ -z "$line" || "$line" == \#* || "$line" == //* ]] && continue

    plugin_dir="$(dirname "$line")"
    [ "$plugin_dir" = "." ] && plugin_dir=""

    case "$plugin_dir" in
        /*) dir_path="$plugin_dir" ;;
        *)  dir_path="$LIST_DIR/$plugin_dir" ;;
    esac

    abs_dir="$(cd "$dir_path" 2>/dev/null && pwd)" || {
        echo "[SKIP] папка не найдена: $plugin_dir"
        skipped=$((skipped + 1))
        skipped_dirs+=("$plugin_dir")
        continue
    }

    case " $seen_dirs " in
        *" $abs_dir "*) continue ;;
    esac
    seen_dirs="$seen_dirs $abs_dir"

    if [ ! -f "$abs_dir/Makefile" ]; then
        echo "[SKIP] Makefile не найден: $plugin_dir"
        skipped=$((skipped + 1))
        skipped_dirs+=("$plugin_dir")
        continue
    fi

    echo
    echo "[BUILD] $plugin_dir"
    echo "------------------------------------------------------------"
    if make -C "$abs_dir" "${@}"; then
        built=$((built + 1))
        built_dirs+=("$plugin_dir")
    else
        status=$?
        echo "[FAIL] $plugin_dir (код ошибки make: $status)" >&2
        failed=$((failed + 1))
        failed_dirs+=("$plugin_dir")
    fi
    echo "------------------------------------------------------------"
done < "$LIST"

echo "[BUILD Launcher]"
make

echo
echo "Итог: собрано $built, пропущено $skipped, ошибок $failed"

if [ "$failed" -gt 0 ]; then
    echo "Не собрались:" >&2
    for d in "${failed_dirs[@]}"; do
        echo "  - $d" >&2
    done
    exit 1
fi
