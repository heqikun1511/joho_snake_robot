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
    ros-jazzy-control-msgs \
    ros-jazzy-controller-manager \
    ros-jazzy-hardware-interface \
    ros-jazzy-joint-state-publisher \
    ros-jazzy-robot-state-publisher \
    ros-jazzy-ros2-control \
    ros-jazzy-ros2-controllers \
    ros-jazzy-serial-driver

WORKDIR /opt/snake_ws



COPY ROS-Packages/snake_interfaces src/snake_interfaces
COPY ROS-Packages/snake_description src/snake_description
COPY ROS-Packages/snake_mcu_hardware src/snake_mcu_hardware
COPY ROS-Packages/snake_joint_state_estimator src/snake_joint_state_estimator
COPY ROS-Packages/snake_gait_controller src/snake_gait_controller
COPY ROS-Packages/snake_command_gate src/snake_command_gate
COPY ROS-Packages/snake_safety src/snake_safety
COPY ROS-Packages/snake_bringup src/snake_bringup


RUN rosdep install \
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

CMD ["bash"]
