#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "用法: $0 <镜像仓库/用户名> [标签]" >&2
  echo "示例: $0 docker.io/my_dockerhub_name jazzy" >&2
  exit 2
fi

case "$(uname -m)" in
  aarch64|arm64) ;;
  *)
    echo "当前系统不是 64 位 ARM：$(uname -m)。请使用 64 位 Raspberry Pi OS/Ubuntu。" >&2
    exit 1
    ;;
esac

REGISTRY="$1"
TAG="${2:-jazzy}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
COMPOSE_FILE="$PROJECT_ROOT/Stacks/snake_robot_system/compose.pi.yaml"
ENV_FILE="$PROJECT_ROOT/Stacks/snake_robot_system/.env"

command -v docker >/dev/null 2>&1 || {
  echo "未找到 docker。请先在 Pi 上安装 Docker Engine 和 Compose 插件。" >&2
  exit 1
}

export IMAGE_REGISTRY="$REGISTRY"
export IMAGE_TAG="$TAG"

echo "拉取 ARM64 业务镜像..."
docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE" pull

echo "启动 snake robot 服务..."
docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE" up -d --remove-orphans

docker compose --env-file "$ENV_FILE" -f "$COMPOSE_FILE" ps
