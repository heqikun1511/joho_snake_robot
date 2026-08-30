FROM ros:jazzy-ros-base-noble

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=jazzy
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    nodejs \
    npm \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-vcstool \
    ros-jazzy-diagnostic-aggregator \
    ros-jazzy-diagnostic-msgs \
    ros-jazzy-foxglove-bridge \
    ros-jazzy-rmw-fastrtps-cpp \
    ros-jazzy-rosbridge-suite \
    ros-jazzy-sensor-msgs \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/snake_ws


COPY ROS-Packages/snake_interfaces src/snake_interfaces
COPY ROS-Packages/snake_description src/snake_description
COPY ROS-Packages/snake_monitoring src/snake_monitoring
COPY Stacks/monitoring/foxglove /opt/snake_monitoring/foxglove

RUN rosdep update && \
    rosdep install \
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