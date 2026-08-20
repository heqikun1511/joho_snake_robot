FROM ros:jazzy-ros-base

ARG DEBIAN_FRONTEND=noninteractive
ARG USER_NAME=snake
ARG USER_UID=1000
ARG USER_GID=1000

SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    sudo \
    udev \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-vcstool \
    libboost-system-dev \
    ros-jazzy-controller-manager \
    ros-jazzy-joint-state-broadcaster \
    ros-jazzy-joint-state-publisher-gui \
    ros-jazzy-joint-trajectory-controller \
    ros-jazzy-robot-state-publisher \
    ros-jazzy-ros2-control \
    ros-jazzy-ros2-controllers \
    ros-jazzy-rviz2 \
    ros-jazzy-urdf \
    ros-jazzy-xacro \
    && rm -rf /var/lib/apt/lists/*

RUN if getent group "${USER_GID}" >/dev/null; then \
      EXISTING_GROUP="$(getent group "${USER_GID}" | cut -d: -f1)"; \
      useradd --uid "${USER_UID}" --gid "${EXISTING_GROUP}" --create-home --shell /bin/bash "${USER_NAME}"; \
    else \
      groupadd --gid "${USER_GID}" "${USER_NAME}"; \
      useradd --uid "${USER_UID}" --gid "${USER_GID}" --create-home --shell /bin/bash "${USER_NAME}"; \
    fi \
    && usermod --append --groups sudo "${USER_NAME}" \
    && echo "${USER_NAME} ALL=(ALL) NOPASSWD:ALL" > "/etc/sudoers.d/${USER_NAME}" \
    && chmod 0440 "/etc/sudoers.d/${USER_NAME}"

COPY Docker/entrypoint.sh /usr/local/bin/snake-entrypoint
RUN chmod +x /usr/local/bin/snake-entrypoint

WORKDIR /workspace/ros2_snake/ros2_ws

USER ${USER_NAME}

ENTRYPOINT ["/usr/local/bin/snake-entrypoint"]
CMD ["bash"]
