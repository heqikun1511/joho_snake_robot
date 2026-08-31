#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCKERFILE="$PROJECT_ROOT/Stacks/base/Dockerfile"
IMAGE_NAME="${IMAGE_NAME:-snake-ros-base:jazzy}"
ROS_BASE_IMAGE="${ROS_BASE_IMAGE:-ros:jazzy-ros-base-noble}"

case "$(uname -m)" in
  aarch64|arm64) ;;
  *)
    echo "错误：当前架构是 $(uname -m)，该脚本应在 64 位 Raspberry Pi 上运行。" >&2
    exit 1
    ;;
esac

command -v docker >/dev/null 2>&1 || {
  echo "错误：未找到 Docker，请先安装 Docker Engine。" >&2
  exit 1
}

if ! docker info >/dev/null 2>&1; then
  echo "错误：无法连接 Docker。请启动 Docker，或将当前用户加入 docker 组。" >&2
  echo "可检查：sudo systemctl status docker" >&2
  exit 1
fi

echo "正在 Raspberry Pi 上构建公共 ROS 2 基础镜像..."
echo "基础镜像：$ROS_BASE_IMAGE"
echo "输出镜像：$IMAGE_NAME"

DOCKER_BUILDKIT=1 docker build \
  --build-arg "ROS_BASE_IMAGE=$ROS_BASE_IMAGE" \
  --tag "$IMAGE_NAME" \
  --file "$DOCKERFILE" \
  "$PROJECT_ROOT/Stacks/base"

ARCH="$(docker image inspect "$IMAGE_NAME" --format '{{.Os}}/{{.Architecture}}')"

echo
echo "构建完成：$IMAGE_NAME ($ARCH)"
echo "测试命令："
echo "  docker run --rm -it --network host $IMAGE_NAME bash"
