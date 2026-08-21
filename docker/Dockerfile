FROM ros:jazzy-ros-base-noble

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=jazzy
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    python3 \
    python3-argcomplete \
    python3-bson \
    python3-colcon-common-extensions \
    python3-pip \
    python3-rosdep \
    python3-vcstool \
    ros-jazzy-rosbridge-suite \
    ros-jazzy-rmw-fastrtps-cpp \
    ros-jazzy-foxglove-bridge \
    ros-jazzy-joint-state-publisher \
    ros-jazzy-robot-state-publisher \
    ros-jazzy-rviz2 \
    && rm -rf /var/lib/apt/lists/*


# Ubuntu 24.04 基础镜像已经提供 UID/GID 1000 的 ubuntu 用户。
# 复用它可避免重复创建 GID 1000，并保持挂载源码的权限与宿主机一致。
RUN usermod -aG dialout ubuntu

WORKDIR /workspace/ros2_snake


COPY docker/ros_entrypoint.sh /ros_entrypoint.sh
RUN chmod +x /ros_entrypoint.sh

USER ubuntu

ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ENTRYPOINT ["/ros_entrypoint.sh"]
CMD ["bash"]
