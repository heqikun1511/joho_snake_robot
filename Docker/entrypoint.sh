#!/usr/bin/env bash
set -e

source /opt/ros/jazzy/setup.bash

workspace_dir="${SNAKE_ROS_WS:-/workspace/ros2_snake/ros2_ws}"

if [[ -f "${workspace_dir}/install/setup.bash" ]]; then
  source "${workspace_dir}/install/setup.bash"
fi

exec "$@"
