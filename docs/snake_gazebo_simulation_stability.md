# CAD 蛇形机器人 Gazebo 仿真稳定性说明

## 目标

使用 SolidWorks 导出的 20 自由度 CAD 蛇形机器人，在 Gazebo Sim 中稳定放置、保持标定姿态，并支持步态控制、单舵机调试和 RGB-D 仿真。静止状态的验收标准是：关节速度接近 `0 rad/s`，机器人不会因控制器振荡而自行滑移。

## 问题、证据与处理

| 现象 | 根因 | 实现处理 |
| --- | --- | --- |
| 启动后姿态从 RViz 中确认的姿态变形 | 控制节点此前将目标统一限制在过小范围；而 CAD 标定中关节 `1`、`11`、`18` 接近 `±pi`。 | `joint_limit_rad` 使用 `pi`，使控制节点不再把标定姿态截断。 |
| 关节 `11`、`18` 报越限 | 恰好取 `±pi` 时会受浮点边界误差影响。 | 初始目标采用 `±3.13 rad`，但 CAD 模型的机械范围仍保留。 |
| CAD 模型落地不稳定或穿透 | 原始 STL 用作动态碰撞体，20 链节多点接触的计算代价和不确定性很高。 | 保留原 STL 作为视觉模型；仿真模型改用内缩 box 作为碰撞代理。 |
| 初始姿态正确，但之后关节与机体高速抖动、滑移 | 停止状态下控制节点每个周期把“上一帧测量角度”重发为新轨迹。位置接口会追逐这个延迟目标，构成离散振荡。曾观察到所有关节速度接近 `±2 rad/s`。 | 停止且没有单舵机覆盖时，控制节点不发布新轨迹；轨迹控制器保持最后一个有效目标。 |
| 仍有伺服接触振荡 | `gz_ros2_control` 的位置命令接口把位置误差换算成速度目标。原参数 `position_proportional_gain=8.0` 超出适合稳定位置跟踪的范围，且控制器更新率比 `1 ms` 物理步慢。 | 位置比例增益设为 `0.25`；控制管理器更新率改为 `1000 Hz`，对齐物理步长。 |

## 架构与数据流

```text
cad_snake_neutral.yaml
    └─ CAD / RViz 标定的 20 个关节角（rad）
             │
             ▼
set_initial_pose.py
    └─ 仅在启动阶段发布初始 JointTrajectory，并等待收敛
             │
             ▼
joint_trajectory_controller
    └─ ros2_control 轨迹控制器
             │
             ▼
gz_ros2_control 插件
    └─ Position command → Gazebo joint velocity setpoint
             │
             ▼
Gazebo Sim / DART
    └─ 重力、碰撞、地面接触与关节动力学
```

运行期命令流如下：

```text
/snake/gait_command ─┐
                     ├─> snake_control_node ─> /joint_trajectory_controller/joint_trajectory
/snake/servo_command ┘
```

`/snake/gait_command` 用于行波步态；`/snake/servo_command` 用于上位机单舵机位置调试。没有这两类有效运动命令时，控制节点不重复发布静止轨迹。

## 文件职责

| 文件 | 职责 |
| --- | --- |
| `src/snake_description/config/cad_snake_neutral.yaml` | CAD / RViz 确认的初始姿态；所有数值单位为弧度。 |
| `src/snake_description/models/cad_snake/urdf/蛇身体总装2.-.urdf` | 未改动的 SolidWorks 原始导出模型，作为 CAD 参考。 |
| `src/snake_description/models/cad_snake/urdf/蛇身体总装2.-_sim_collision.urdf` | 仿真碰撞版本：原始视觉 STL、简化碰撞代理、关节阻尼。 |
| `src/snake_description/urdf/snake_sim_urdf.xacro` | 仿真机器人入口，组合 CAD 碰撞模型、ros2_control 和 RGB-D 相机。 |
| `src/snake_description/urdf/snake_cad_gz_ros2_control.xacro` | `gz_ros2_control` 插件、20 个关节接口和位置比例增益。 |
| `src/snake_description/worlds/snake_rgbd_arena.sdf` | DART 物理世界、地面与 RGB-D 视觉特征物。 |
| `src/snake_control_node/config/cad_controllers.yaml` | `controller_manager` 与 `joint_trajectory_controller` 配置，更新率为 `1000 Hz`。 |
| `src/snake_control_node/config/cad_snake_control_node.yaml` | PID / MPC 控制节点参数、关节范围与输出话题。 |
| `src/snake_control_node/src/snake_control_node.cpp` | 步态、PID/MPC、停止保持和单舵机命令实现。 |
| `src/snake_sim_bringup/scripts/set_initial_pose.py` | 启动阶段施加并验证初始姿态。 |
| `src/snake_sim_bringup/launch/snake_sim.launch.py` | 启动顺序：Gazebo → 控制器 → 初始姿态 → 控制节点。 |

## `snake_control_node` 的改动

### 配置调整

`config/cad_snake_control_node.yaml`：

- `control_rate_hz`：`50.0` 改为 `30.0`，匹配控制节点的轨迹发布频率；Gazebo 内部控制器仍以 `1000 Hz` 更新。
- `joint_limit_rad`：设为 `3.141592653589793`，避免 CAD 标定姿态被错误截断。
- PID：`kp` 从 `1.0` 调为 `0.30`，`kd` 从 `0.02` 调为 `0.01`，降低步态启停时的姿态修正冲击。

`config/cad_controllers.yaml`：

- `controller_manager.update_rate`：`100` 改为 `1000 Hz`，与 Gazebo 世界 `max_step_size=0.001 s` 对齐。

### 控制逻辑调整

`src/snake_control_node.cpp`：

1. 新增 `/snake/servo_command` 订阅，接收 `snake_interfaces/msg/ServoCommand`。
2. 单舵机命令会检查关节名、有限数值和命令范围；仅使被选关节移动，其余关节维持当前测量姿态。
3. 当 `GaitCommand.stop=true` 且没有单舵机覆盖时直接返回，不再以控制周期反复发布 `JointTrajectory`。
4. 重新开始步态或单舵机控制时，PID / MPC 以当前关节测量值重置，避免停止阶段累积的状态造成首帧冲击。

## `position_proportional_gain` 的含义

它是官方 `gz_ros2_control` Gazebo Sim 插件的参数，不是 DART 地面摩擦参数，也不是实体舵机 PID。对于位置命令接口，插件使用近似关系：

```text
joint_velocity_setpoint
  = position_proportional_gain
  × joint_position_error
  × controller_manager_update_rate
```

因此高增益会直接产生很高的关节速度目标。当前采用 `0.25` 而非原来的 `8.0`，用于避免多点接触下的关节速度饱和和接触能量注入。

## 启动与验证

```bash
cd /home/amoy/F407_active/joho_snake_robot
source install/setup.bash
ros2 launch snake_sim_bringup snake_sim.launch.py use_rviz:=false use_foxglove:=false
```

等待日志出现 `CAD initial joint pose reached` 后，静置约 5 秒：

```bash
ros2 topic echo --once /joint_states
```

验收要点：

- 关节位置接近 `cad_snake_neutral.yaml` 的目标；
- `velocity` 接近 `0`，不应普遍出现 `2.0` 或 `-2.0`；
- 无步态命令时，模型不应持续平移、旋转或产生明显抖动；
- 发出 `/snake/servo_command` 时，只应有指定关节执行调试动作。

