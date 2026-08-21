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


ARG USER_ID=1000
ARG GROUP_ID=1000

#声明两个构建参数，默认值都是 1000（通常是宿主机第一个普通用户的 UID/GID）。
#声明两个构建参数，默认值都是 1000（通常是宿主机第一个普通用户的 UID/GID）。
RUN groupadd --gid ${GROUP_ID} developer \
    && useradd --uid ${USER_ID} \
               --gid ${GROUP_ID} \
               --create-home \
               --shell /bin/bash \
               developer \
    && usermod -aG dialout developer

WORKDIR /workspace/ros2_snake


COPY docker/ros_entrypoint.sh /ros_entrypoint.sh
RUN chmod +x /ros_entrypoint.sh

USER developer

ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ENTRYPOINT ["/ros_entrypoint.sh"]
CMD ["bash"]