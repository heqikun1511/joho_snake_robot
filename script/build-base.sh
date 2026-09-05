#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCKERFILE="$PROJECT_ROOT/Stacks/base/Dockerfile"
IMAGE_NAME="${IMAGE_NAME:-snake-ros-base:jazzy}"
ROS_BASE_IMAGE="${ROS_BASE_IMAGE:-ros:jazzy-ros-base-noble}"
TARGET_PLATFORM="${TARGET_PLATFORM:-}"

case "$(uname -m)" in
  x86_64|amd64)
    HOST_PLATFORM="linux/amd64"
    ;;
  aarch64|arm64)
    HOST_PLATFORM="linux/arm64"
    ;;
  *)
    echo "错误：暂不支持当前架构 $(uname -m)；支持 x86_64/amd64 和 aarch64/arm64。" >&2
    exit 1
    ;;
esac

command -v docker >/dev/null 2>&1 || {
  echo "错误：未找到 Docker，请先安装 Docker Engine 或 Docker Desktop。" >&2
  exit 1
}

if ! docker info >/dev/null 2>&1; then
  echo "错误：无法连接 Docker，请确认 Docker 已启动且当前用户有访问权限。" >&2
  exit 1
fi

# 默认构建宿主机原生架构；也可通过 TARGET_PLATFORM 显式交叉构建。
PLATFORM_ARGS=()
if [[ -n "$TARGET_PLATFORM" ]]; then
  case "$TARGET_PLATFORM" in
    linux/amd64|linux/arm64) ;;
    *)
      echo "错误：TARGET_PLATFORM 仅支持 linux/amd64 或 linux/arm64。" >&2
      exit 1
      ;;
  esac
  PLATFORM_ARGS=(--platform "$TARGET_PLATFORM")
else
  TARGET_PLATFORM="$HOST_PLATFORM"
fi

echo "正在构建公共 ROS 2 基础镜像..."
echo "宿主机平台：$HOST_PLATFORM"
echo "目标平台：$TARGET_PLATFORM"
echo "基础镜像：$ROS_BASE_IMAGE"
echo "输出镜像：$IMAGE_NAME"

docker build \
  "${PLATFORM_ARGS[@]}" \
  --build-arg "ROS_BASE_IMAGE=$ROS_BASE_IMAGE" \
  --tag "$IMAGE_NAME" \
  --file "$DOCKERFILE" \
  "$PROJECT_ROOT/Stacks/base"

BUILT_PLATFORM="$(docker image inspect "$IMAGE_NAME" --format '{{.Os}}/{{.Architecture}}')"

echo
echo "构建完成：$IMAGE_NAME ($BUILT_PLATFORM)"
echo "测试命令："
echo "  docker run --rm -it --network host $IMAGE_NAME bash"
