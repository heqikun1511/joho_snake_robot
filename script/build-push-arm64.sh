#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "用法: $0 <镜像仓库/用户名> [标签]" >&2
  echo "示例: $0 docker.io/my_dockerhub_name jazzy" >&2
  exit 2
fi

REGISTRY="$1"
TAG="${2:-jazzy}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILDER="snake-arm64-builder"

command -v docker >/dev/null 2>&1 || {
  echo "未找到 docker。" >&2
  exit 1
}

if ! docker buildx inspect "$BUILDER" >/dev/null 2>&1; then
  docker buildx create --name "$BUILDER" --driver docker-container --use
else
  docker buildx use "$BUILDER"
fi

docker buildx inspect --bootstrap >/dev/null

cd "$PROJECT_ROOT"

echo "[1/2] 构建并推送公共 ARM64 基础镜像..."
REGISTRY="$REGISTRY" TAG="$TAG" PLATFORM=linux/arm64 \
  docker buildx bake base --push

echo "[2/2] 并行构建并推送三个业务镜像..."
REGISTRY="$REGISTRY" TAG="$TAG" PLATFORM=linux/arm64 \
  docker buildx bake apps --push

echo "镜像已推送到 $REGISTRY，标签为 $TAG。"
