#!/usr/bin/env bash
# ------------------------------------------------------------------------
# Build the project inside Docker, or drop into a Docker shell.
#
# Uses the docker-debug preset which builds into .build/docker-debug-clang,
# separate from the native .build/debug-clang, so both can coexist.
#
# Usage:
#   ./scripts/docker-build.sh          # Build + test inside Docker
#   ./scripts/docker-build.sh --shell  # Drop into Docker shell
# ------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PRESET="docker-debug"
IMAGE="aipp101"

SHELL_ONLY=false

for arg in "$@"; do
    case "$arg" in
        --shell) SHELL_ONLY=true ;;
        -h|--help)
            sed -n '2,/^# ----/{ /^# ----/d; s/^# //; s/^#//; p }' "$0"
            exit 0
            ;;
        *) echo "Unknown option: $arg"; exit 1 ;;
    esac
done

cd "$PROJECT_DIR"

DOCKER_RUN=(docker run --rm -it
    -u "$(id -u):$(id -g)"
    -v "$PROJECT_DIR:/workspace"
    "$IMAGE"
)

if $SHELL_ONLY; then
    exec "${DOCKER_RUN[@]}"
else
    exec "${DOCKER_RUN[@]}" bash -c "cmake --preset $PRESET && cmake --build --preset $PRESET && ctest --preset $PRESET"
fi
