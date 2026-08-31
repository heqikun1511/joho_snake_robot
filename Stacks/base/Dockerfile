ARG ROS_BASE_IMAGE=ros:jazzy-ros-base-noble
FROM ${ROS_BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive \
    ROS_DISTRO=jazzy \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    RMW_IMPLEMENTATION=rmw_fastrtps_cpp

# 三个业务镜像共同使用的编译工具、rosdep 和 DDS 实现。
# 保持标准 Dockerfile 语法，以兼容未安装 Buildx 的 Raspberry Pi。
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      git \
      python3-colcon-common-extensions \
      python3-rosdep \
      python3-vcstool \
      ros-jazzy-rmw-fastrtps-cpp \
    && rm -rf /var/lib/apt/lists/*

# rosdep 数据随基础镜像缓存；业务源码变化不会再次执行该步骤。
RUN rosdep update --rosdistro "${ROS_DISTRO}"

WORKDIR /opt/snake_ws
