#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Paths to Dockerfiles (relative to repo root)
BASE_DOCKERFILE="Stacks/base/Dockerfile"
AUTONOMY_DOCKERFILE="Stacks/autonomy/Dockerfile"
CONTROL_DOCKERFILE="Stacks/control/Dockerfile"
MONITOR_DOCKERFILE="Stacks/monitoring/foxglove/Dockerfile"

# Image tags
IM_BASE="snake-ros-base:jazzy"
IM_AUTON="snake_autonomy:jazzy"
IM_CTRL="snake_control:jazzy"
IM_MON="snake_monitoring:jazzy"

# Container names
CON_AUTON="snake_autonomy"
CON_CTRL="snake_control"
CON_MON="snake_monitoring"

echo "Running from: $ROOT_DIR"

which docker >/dev/null 2>&1 || { echo "docker not found. Install docker and re-run." >&2; exit 1; }

echo "Building images (this may take a while on Raspberry Pi)..."
docker build -f "$ROOT_DIR/$BASE_DOCKERFILE" -t "$IM_BASE" "$ROOT_DIR"
docker build --build-arg BASE_IMAGE="$IM_BASE" -f "$ROOT_DIR/$AUTONOMY_DOCKERFILE" -t "$IM_AUTON" "$ROOT_DIR"
docker build --build-arg BASE_IMAGE="$IM_BASE" -f "$ROOT_DIR/$CONTROL_DOCKERFILE" -t "$IM_CTRL" "$ROOT_DIR"
docker build --build-arg BASE_IMAGE="$IM_BASE" -f "$ROOT_DIR/$MONITOR_DOCKERFILE" -t "$IM_MON" "$ROOT_DIR"

echo "Stopping and removing old containers if present..."
docker rm -f $CON_AUTON $CON_CTRL $CON_MON >/dev/null 2>&1 || true

echo "Starting containers (host network)"
# Host network simplifies ROS2 DDS discovery on Raspberry Pi
docker run -d --name $CON_AUTON --network host --restart unless-stopped $IM_AUTON
docker run -d --name $CON_CTRL --network host --privileged --restart unless-stopped $IM_CTRL
docker run -d --name $CON_MON --network host --restart unless-stopped $IM_MON

echo "Waiting for containers to initialize..."
sleep 8

echo "Launching robot bringup inside $CON_CTRL (mock_control)..."
docker exec -d $CON_CTRL bash -lc "source /opt/ros/jazzy/setup.bash && source /opt/snake_ws/install/setup.bash && ros2 launch snake_bringup mock_control.launch.py"

echo "Starting rosbridge websocket inside $CON_MON..."
docker exec -d $CON_MON bash -lc "source /opt/ros/jazzy/setup.bash && source /opt/snake_ws/install/setup.bash && ros2 launch rosbridge_server rosbridge_websocket_launch.py"

echo "All containers started."
echo "Foxglove Bridge (foxglove_bridge) should be on port 8765 and rosbridge on 9090 (both use host network)."
echo "Connect Foxglove Studio from your PC to:"
echo "  - Foxglove Bridge: ws://<RASPBERRY_IP>:8765"
echo "  - rosbridge WebSocket: ws://<RASPBERRY_IP>:9090"

echo "To follow logs:"
echo "  docker logs -f $CON_MON" 
echo "  docker logs -f $CON_CTRL" 

exit 0
