#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_dir}"

export HOST_UID="${HOST_UID:-$(id -u)}"
export HOST_GID="${HOST_GID:-$(id -g)}"
export DIALOUT_GID="${DIALOUT_GID:-$(getent group dialout | cut -d: -f3 || true)}"
export DIALOUT_GID="${DIALOUT_GID:-20}"

mode="${1:-shell}"

allow_x11()
{
  if [[ -n "${DISPLAY:-}" ]] && command -v xhost >/dev/null 2>&1; then
    xhost +local:docker >/dev/null
  fi
}

build_workspace='colcon build --symlink-install --packages-select snake_description snake_bringup'

case "${mode}" in
  shell)
    allow_x11
    docker compose run --rm snake-dev bash
    ;;

  up)
    allow_x11
    docker compose up --detach snake-dev
    echo "Container started: snake-robot-dev"
    echo "Enter it with: docker compose exec snake-dev bash"
    ;;

  build)
    docker compose run --rm snake-dev bash -lc "${build_workspace}"
    ;;

  description)
    allow_x11
    docker compose run --rm snake-dev bash -lc \
      "${build_workspace} && source install/setup.bash && ros2 launch snake_description display.launch.py"
    ;;

  mock)
    allow_x11
    docker compose run --rm snake-dev bash -lc \
      "${build_workspace} && source install/setup.bash && ros2 launch snake_bringup snake_bringup.launch.py use_mock_hardware:=true"
    ;;

  hardware)
    export SERIAL_DEVICE="${SERIAL_DEVICE:-/dev/ttyUSB0}"

    if [[ ! -c "${SERIAL_DEVICE}" ]]; then
      echo "Serial device does not exist or is not a character device: ${SERIAL_DEVICE}" >&2
      echo "Set it explicitly, for example:" >&2
      echo "  SERIAL_DEVICE=/dev/ttyACM0 $0 hardware" >&2
      exit 1
    fi

    allow_x11
    docker compose --profile hardware run --rm snake-hardware bash -lc \
      "${build_workspace} && source install/setup.bash && ros2 launch snake_bringup snake_bringup.launch.py use_mock_hardware:=false serial_port:=${SERIAL_DEVICE}"
    ;;

  *)
    echo "Usage: $0 {shell|up|build|description|mock|hardware}" >&2
    exit 2
    ;;
esac
