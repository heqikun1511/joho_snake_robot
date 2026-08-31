variable "REGISTRY" {
  default = "docker.io/CHANGE_ME"
}

variable "TAG" {
  default = "jazzy"
}

variable "PLATFORM" {
  default = "linux/arm64"
}

group "default" {
  targets = ["base", "autonomy", "control", "monitoring"]
}

group "apps" {
  targets = ["autonomy", "control", "monitoring"]
}

target "common" {
  context   = "."
  platforms = [PLATFORM]
}

target "base" {
  inherits   = ["common"]
  dockerfile = "Stacks/base/Dockerfile"
  tags       = ["${REGISTRY}/snake-ros-base:${TAG}"]
  cache-from = ["type=registry,ref=${REGISTRY}/snake-build-cache:base-${TAG}"]
  cache-to   = ["type=registry,ref=${REGISTRY}/snake-build-cache:base-${TAG},mode=max"]
}

target "autonomy" {
  inherits   = ["common"]
  dockerfile = "Stacks/autonomy/Dockerfile"
  args = {
    BASE_IMAGE = "${REGISTRY}/snake-ros-base:${TAG}"
  }
  tags = ["${REGISTRY}/snake-autonomy:${TAG}"]
  cache-from = ["type=registry,ref=${REGISTRY}/snake-build-cache:autonomy-${TAG}"]
  cache-to   = ["type=registry,ref=${REGISTRY}/snake-build-cache:autonomy-${TAG},mode=max"]
}

target "control" {
  inherits   = ["common"]
  dockerfile = "Stacks/control/Dockerfile"
  args = {
    BASE_IMAGE = "${REGISTRY}/snake-ros-base:${TAG}"
  }
  tags = ["${REGISTRY}/snake-control:${TAG}"]
  cache-from = ["type=registry,ref=${REGISTRY}/snake-build-cache:control-${TAG}"]
  cache-to   = ["type=registry,ref=${REGISTRY}/snake-build-cache:control-${TAG},mode=max"]
}

target "monitoring" {
  inherits   = ["common"]
  dockerfile = "Stacks/monitoring/foxglove/Dockerfile"
  args = {
    BASE_IMAGE = "${REGISTRY}/snake-ros-base:${TAG}"
  }
  tags = ["${REGISTRY}/snake-monitoring:${TAG}"]
  cache-from = ["type=registry,ref=${REGISTRY}/snake-build-cache:monitoring-${TAG}"]
  cache-to   = ["type=registry,ref=${REGISTRY}/snake-build-cache:monitoring-${TAG},mode=max"]
}
