#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GAME_EXE="${GAME_EXE:-$ROOT_DIR/bin/main.exe}"

find_proton() {
    if [[ -n "${PROTON_BIN:-}" && -x "${PROTON_BIN}" ]]; then
        printf '%s\n' "${PROTON_BIN}"
        return 0
    fi

    local candidates=(
        "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common/Proton - Experimental/proton"
        "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common/Proton Experimental/proton"
        "$HOME/.steam/steam/steamapps/common/Proton - Experimental/proton"
        "$HOME/.steam/steam/steamapps/common/Proton Experimental/proton"
        "$HOME/.local/share/Steam/steamapps/common/Proton - Experimental/proton"
        "$HOME/.local/share/Steam/steamapps/common/Proton Experimental/proton"
        "$HOME/.steam/root/steamapps/common/Proton - Experimental/proton"
        "$HOME/.steam/root/steamapps/common/Proton Experimental/proton"
    )
    local path
    for path in "${candidates[@]}"; do
        if [[ -x "$path" ]]; then
            printf '%s\n' "$path"
            return 0
        fi
    done

    local steam_common_dirs=(
        "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common"
        "$HOME/.steam/steam/steamapps/common"
        "$HOME/.local/share/Steam/steamapps/common"
        "$HOME/.steam/root/steamapps/common"
    )
    local dir
    for dir in "${steam_common_dirs[@]}"; do
        [[ -d "$dir" ]] || continue
        while IFS= read -r path; do
            if [[ -x "$path" ]]; then
                printf '%s\n' "$path"
                return 0
            fi
        done < <(find "$dir" -maxdepth 2 -path '*/Proton */proton' -o -path '*/Proton-*/proton' 2>/dev/null | sort -r)
    done

    return 1
}

if [[ ! -f "$GAME_EXE" ]]; then
    printf 'Missing game executable: %s\n' "$GAME_EXE" >&2
    exit 1
fi

case "$(uname -s)" in
    Linux)
        PROTON_PATH="$(find_proton)" || {
            printf 'Could not find Proton. Set PROTON_BIN to your proton launcher path.\n' >&2
            exit 1
        }

        export STEAM_COMPAT_CLIENT_INSTALL_PATH="${STEAM_COMPAT_CLIENT_INSTALL_PATH:-$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam}"
        export STEAM_COMPAT_DATA_PATH="${STEAM_COMPAT_DATA_PATH:-$ROOT_DIR/.proton-prefix}"
        mkdir -p "$STEAM_COMPAT_DATA_PATH"

        exec "$PROTON_PATH" run "$GAME_EXE"
        ;;
    *)
        exec "$GAME_EXE"
        ;;
esac
