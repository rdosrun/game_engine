#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$ROOT_DIR/bin"
OUTPUT="$BIN_DIR/main.exe"

mkdir -p "$BIN_DIR"

find_windows_compiler() {
    if [[ -n "${CC:-}" ]]; then
        printf '%s\n' "$CC"
        return 0
    fi

    local candidates=(
        x86_64-w64-mingw32-gcc
        i686-w64-mingw32-gcc
        gcc
    )
    local candidate
    for candidate in "${candidates[@]}"; do
        if command -v "$candidate" >/dev/null 2>&1; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

CC_BIN="$(find_windows_compiler)" || {
    printf 'No Windows cross-compiler found.\n' >&2
    printf 'Install MinGW-w64 and retry, or set CC to a Windows-targeting compiler.\n' >&2
    exit 1
}

if [[ "$CC_BIN" == "gcc" ]] && [[ "$(uname -s)" == "Linux" ]]; then
    printf 'Native gcc cannot build this Windows binary on Linux.\n' >&2
    printf 'Install MinGW-w64 and use x86_64-w64-mingw32-gcc or set CC explicitly.\n' >&2
    exit 1
fi

"$CC_BIN" "$ROOT_DIR/src/main.c" -o "$OUTPUT" -lgdi32 -lm -mwindows

printf 'Build successful.\n'
