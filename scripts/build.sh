#!/usr/bin/env bash
# Build the native ROS 2 workspace from the repository root.
set -Eeuo pipefail

readonly PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROS_VERSION="${ROS_DISTRO:-jazzy}"
readonly ROS_SETUP="/opt/ros/${ROS_VERSION}/setup.bash"

if [[ ! -f "${ROS_SETUP}" ]]; then
  echo "error: ROS 2 ${ROS_VERSION} is not installed at ${ROS_SETUP}" >&2
  exit 1
fi

if ! command -v colcon >/dev/null 2>&1; then
  echo 'error: colcon is required; install it with: sudo apt install python3-colcon-common-extensions' >&2
  exit 1
fi

cd -- "${PROJECT_ROOT}"
# shellcheck disable=SC1090
set +u
source "${ROS_SETUP}"
set -u
colcon build --base-paths src --symlink-install "$@"
