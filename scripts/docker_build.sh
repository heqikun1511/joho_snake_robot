#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_dir}"

export HOST_UID="${HOST_UID:-$(id -u)}"
export HOST_GID="${HOST_GID:-$(id -g)}"
export DIALOUT_GID="${DIALOUT_GID:-$(getent group dialout | cut -d: -f3 || true)}"
export DIALOUT_GID="${DIALOUT_GID:-20}"

docker compose build snake-dev
