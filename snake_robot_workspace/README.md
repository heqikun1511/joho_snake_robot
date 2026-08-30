# snake_robot_workspace

This workspace contains the ROS-oriented project structure for the snake robot platform.

## Architecture

- `snake_robot_system/` - Raspberry Pi deployment and production compose files
- `simulation/` - Gazebo and full-system simulation
- `local_orchestration/` - local runtime orchestration for hardware, MCU simulator, and simulation
- `snake_robot_workspace/` - workspace-level developer shortcuts and project docs
- `Stacks/` - runtime service stacks
  - `control/`
  - `autonomy/`
  - `monitoring/`
- `ROS-Packages/` - ROS packages and ROS-adjacent code
  - `snake_interfaces/`
  - `snake_description/`
  - `snake_mcu_hardware/`
  - `snake_joint_state_estimator/`
  - `snake_gait_controller/`
  - `snake_command_gate/`
  - `snake_safety/`
  - `snake_navigation/`
  - `snake_bringup/`
  - `snake_monitoring/`
  - `snake_sim_hardware/`
  - `snake_sim_bringup/`

## Foxglove migration note
Foxglove content is preserved and integrated into the ROS architecture by keeping:
- runtime dashboard configuration under `Stacks/monitoring/foxglove/`
- custom extension sources under `ROS-Packages/snake_monitoring/foxglove_extensions/`

This keeps visualization assets available while maintaining a ROS-first package layout.

