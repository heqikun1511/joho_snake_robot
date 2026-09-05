#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "提示：build-base-on-pi.sh 已兼容保留，推荐改用 script/build-base.sh。" >&2
exec "$SCRIPT_DIR/build-base.sh" "$@"
