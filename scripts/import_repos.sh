#!/usr/bin/env bash
# Import only missing repositories declared by the canonical manifest.
set -Eeuo pipefail

readonly PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly MANIFEST="${PROJECT_ROOT}/repos/joho_snake_robot.repos"

if ! command -v vcs >/dev/null 2>&1; then
  echo 'error: vcstool is required; install it with: sudo apt install python3-vcstool' >&2
  exit 1
fi

cd -- "${PROJECT_ROOT}"
vcs import --recursive --skip-existing . < "${MANIFEST}"
