ARG BASE_IMAGE=snake-ros-base:jazzy
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=jazzy
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    nodejs \
    npm \
    ros-jazzy-diagnostic-aggregator \
    ros-jazzy-diagnostic-msgs \
    ros-jazzy-foxglove-bridge \
    ros-jazzy-rosbridge-suite \
    ros-jazzy-sensor-msgs

WORKDIR /opt/snake_ws


COPY ROS-Packages/snake_interfaces src/snake_interfaces
COPY ROS-Packages/snake_description src/snake_description
COPY ROS-Packages/snake_monitoring src/snake_monitoring
COPY Stacks/monitoring/foxglove /opt/snake_monitoring/foxglove

RUN rosdep install \
      --from-paths src \
      --ignore-src \
      --rosdistro "${ROS_DISTRO}" \
      -r -y


RUN . "/opt/ros/${ROS_DISTRO}/setup.sh" && \
    colcon build \
      --symlink-install \
      --event-handlers console_direct+

WORKDIR /opt/snake_ws/src/snake_monitoring/foxglove_extensions

RUN npm install && npm run build


RUN printf '%s\n' \
    "source /opt/ros/${ROS_DISTRO}/setup.bash" \
    "source /opt/snake_ws/install/setup.bash" \
    >> /root/.bashrc

WORKDIR /opt/snake_ws

EXPOSE 8765

CMD ["ros2", "launch", "foxglove_bridge", "foxglove_bridge_launch.xml"]
