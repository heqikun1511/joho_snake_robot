ARG ROS_BASE_IMAGE=ros:jazzy-ros-base-noble
FROM ${ROS_BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive \
    ROS_DISTRO=jazzy \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    RMW_IMPLEMENTATION=rmw_fastrtps_cpp

# 三个业务镜像共同使用的编译工具、rosdep 和 DDS 实现。
# BuildKit cache mount 避免重复下载 apt 索引和软件包。
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      git \
      python3-colcon-common-extensions \
      python3-rosdep \
      python3-vcstool \
      ros-jazzy-rmw-fastrtps-cpp

# rosdep 数据随基础镜像缓存；业务源码变化不会再次执行该步骤。
RUN rosdep update --rosdistro "${ROS_DISTRO}"

WORKDIR /opt/snake_ws
