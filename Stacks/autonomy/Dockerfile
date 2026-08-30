FROM ros:jazzy-ros-base-noble
# 避免 Docker 构建因为等待键盘输入而卡住。
ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=jazzy
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp


RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-vcstool \
    ros-jazzy-geometry-msgs \
    ros-jazzy-nav2-bringup \
    ros-jazzy-navigation2 \
    ros-jazzy-rmw-fastrtps-cpp \
    ros-jazzy-robot-state-publisher \
    ros-jazzy-sensor-msgs \
    ros-jazzy-tf2 \
    ros-jazzy-tf2-geometry-msgs \
    ros-jazzy-tf2-ros \
    && rm -rf /var/lib/apt/lists/*

# 将 /opt/snake_ws 设置为 ROS 2 工作空间。
# 使用 /opt 而不是用户主目录，适合容器内的固定部署。
WORKDIR /opt/snake_ws

COPY ROS-Packages/snake_interfaces src/snake_interfaces
COPY ROS-Packages/snake_description src/snake_description
COPY ROS-Packages/snake_navigation src/snake_navigation
COPY ROS-Packages/snake_command_gate src/snake_command_gate
COPY ROS-Packages/snake_safety src/snake_safety



# 根据各 ROS 包的 package.xml 自动安装系统依赖。
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

RUN printf '%s\n' \
    "source /opt/ros/${ROS_DISTRO}/setup.bash" \
    "source /opt/snake_ws/install/setup.bash" \
    >> /root/.bashrc

# 没有指定运行命令时进入 Bash，
CMD ["bash"]