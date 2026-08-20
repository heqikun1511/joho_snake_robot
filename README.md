# STM32F407 蛇形机器人 ROS 2 工程

本仓库用于实现一套由 STM32F407 和 JOHO UART 总线舵机驱动的蛇形机器人系统，并通过 `ros2_control` 接入 ROS 2。

项目采用单仓库结构，同时管理：

- STM32F407 下位机固件；
- ROS 2 机器人描述；
- `ros2_control` 硬件接口；
- 标准轨迹控制器；
- 蛇形步态、遥控和仿真模块。

> 当前项目处于早期开发阶段。STM32 固件迁移和机器人描述已有初版，ROS 2 bringup 正在搭建，真实串口硬件接口、上位机协议及步态节点尚未完成。

## 1. 系统架构

```text
键盘 / 手柄 / 导航命令
          │
          ▼
snake_teleop / snake_controller
          │ JointTrajectory
          ▼
joint_trajectory_controller
          │ position command interface（rad）
          ▼
snake_hardware / ros2_control SystemInterface
          │ PC—STM32 二进制串口协议
          ▼
STM32F407（建议使用 USART6）
          │ JOHO UART 总线舵机协议
          ▼
8 个 JOHO 舵机（USART3）
```

状态反馈方向：

```text
JOHO 舵机位置/状态
        → STM32F407 汇总
        → snake_hardware::read()
        → joint_state_broadcaster
        → /joint_states
        → robot_state_publisher / RViz
```

建议的 STM32 串口分工：

| STM32 串口 | 用途 |
| --- | --- |
| USART1 | 调试日志 |
| USART3 | JOHO 舵机总线 |
| USART6 | ROS 2 主机通信（待实现） |

## 2. 当前机器人配置

当前 STM32 固件按 4 个正交双轴关节模块配置，共使用 8 个舵机：

| ROS 2 关节 | 物理轴 | 舵机 ID |
| --- | --- | ---: |
| `pitch_joint_1` | 垂直/Pitch | 1 |
| `yaw_joint_1` | 水平/Yaw | 2 |
| `pitch_joint_2` | 垂直/Pitch | 3 |
| `yaw_joint_2` | 水平/Yaw | 4 |
| `pitch_joint_3` | 垂直/Pitch | 5 |
| `yaw_joint_3` | 水平/Yaw | 6 |
| `pitch_joint_4` | 垂直/Pitch | 7 |
| `yaw_joint_4` | 水平/Yaw | 8 |

JOHO 舵机原始位置范围为 `0~4095`，中位值为 `2048`。ROS 2 侧统一使用弧度。当前机器人描述中的尺寸、质量、关节方向、机械零位和安全限位仍需要根据实物标定。

## 3. 仓库结构

```text
ros2_snake/
├── firmware/
│   └── stm32f407_snake/          # STM32F407 固件
│       ├── Application/          # 步态和应用逻辑
│       ├── Core/                 # CubeMX/HAL 入口、中断、GPIO、UART
│       ├── Drivers/
│       │   ├── Board/
│       │   │   ├── joho_servo/   # JOHO 舵机协议
│       │   │   └── ring_buffer/  # UART 环形缓冲
│       │   ├── CMSIS/
│       │   └── STM32F4xx_HAL_Driver/
│       ├── Makefile
│       ├── F407_JOHO.ioc
│       └── MIGRATION.md
│
├── ros2_ws/                      # ROS 2 Jazzy 工作空间
│   ├── src/
│   │   ├── snake_description/    # URDF/Xacro、RViz、关节映射
│   │   ├── snake_hardware/       # ros2_control 硬件插件（待实现）
│   │   ├── snake_controller/     # 蛇形步态生成（待实现）
│   │   ├── snake_bringup/        # 控制器配置和整机启动
│   │   ├── snake_interfaces/     # 自定义消息/服务（待实现）
│   │   ├── snake_teleop/         # 键盘/手柄控制（待实现）
│   │   └── snake_simulation/     # 模拟与仿真（待实现）
│   ├── build/                    # colcon 自动生成，不提交 Git
│   ├── install/                  # colcon 自动生成，不提交 Git
│   └── log/                      # colcon 自动生成，不提交 Git
│
├── docs/                         # 设计和使用文档
├── scripts/                      # 构建、部署和调试脚本
├── Docker/                       # 容器环境（预留）
├── .gitignore
└── README.md
```

## 4. ROS 2 包职责

### snake_description

负责机器人本体描述，不直接操作串口：

- link 和 joint 的父子关系；
- Pitch/Yaw 旋转轴；
- 关节机械限位；
- 碰撞、外观、质量和惯量；
- 舵机 ID、方向、零位参数；
- `<ros2_control>` 硬件声明；
- RViz 显示。

### snake_hardware

实现 `hardware_interface::SystemInterface`，作为 ROS 2 与 STM32 的边界：

- `snake_system`：ros2_control 生命周期、`read()` 和 `write()`；
- `serial_transport`：Linux 串口收发；
- `stm32_protocol`：PC—STM32 数据帧、CRC 和解析；
- 弧度与舵机原始位置转换；
- 通信超时和错误处理。

### snake_controller

负责生成蛇形运动，不直接访问硬件：

- 平面蜿蜒；
- 侧向蜿蜒；
- 转向和停止；
- 后续 CPG 步态；
- 发布标准 `trajectory_msgs/msg/JointTrajectory`。

### snake_bringup

负责整机启动和控制器配置：

- `robot_state_publisher`；
- `controller_manager`；
- `joint_state_broadcaster`；
- `joint_trajectory_controller`；
- 模拟硬件/真实硬件启动选项。

### snake_interfaces

只存放标准 ROS 2 接口无法覆盖的诊断和维护功能，例如硬件状态、扭矩开关和关节标定。正常关节控制优先使用 ros2_control 标准接口。

### snake_teleop

把键盘或手柄输入转换成速度、转向和步态参数，不直接发送舵机原始位置。

### snake_simulation

用于无实物开发。早期优先使用 `mock_components/GenericSystem`，后续再接入 Gazebo。

## 5. 环境要求

当前 ROS 2 侧以以下环境为目标：

- Ubuntu 24.04；
- ROS 2 Jazzy；
- `ros2_control` / `ros2_controllers`；
- Xacro、URDF、RViz；
- `colcon` 和 `rosdep`。

STM32 侧需要：

- `arm-none-eabi-gcc`；
- GNU Make；
- STM32F407 开发板；
- JOHO UART 总线舵机及独立舵机电源。

ROS 2 依赖示例：

```bash
sudo apt install \
  ros-jazzy-ros2-control \
  ros-jazzy-ros2-controllers \
  ros-jazzy-xacro \
  ros-jazzy-robot-state-publisher \
  ros-jazzy-joint-state-publisher-gui \
  ros-jazzy-rviz2
```

## 6. 构建 STM32 固件

```bash
cd firmware/stm32f407_snake
make -j2
```

构建结果位于：

```text
firmware/stm32f407_snake/build/
├── STM32F407_SNAKE.elf
├── STM32F407_SNAKE.hex
└── STM32F407_SNAKE.bin
```

固件迁移说明见 `firmware/stm32f407_snake/MIGRATION.md`。

## 7. 构建 ROS 2 工作空间

打开一个已正确加载 ROS 2 Jazzy、且未被其他 Python 虚拟环境污染的终端：

```bash
source /opt/ros/jazzy/setup.bash
cd ros2_ws
```

首次构建前安装依赖：

```bash
rosdep install --from-paths src --ignore-src -r -y
```

当前建议先单独构建已有内容的包：

```bash
colcon build \
  --symlink-install \
  --packages-select snake_description snake_bringup
```

加载工作空间：

```bash
source install/setup.bash
```

其他包目前仍是空框架，不应加入构建，直到对应 `package.xml` 和 `CMakeLists.txt` 完成。

## 8. 验证机器人描述

生成标准 URDF：

```bash
xacro \
  src/snake_description/urdf/snake.urdf.xacro \
  use_mock_hardware:=true \
  > /tmp/snake_robot.urdf
```

检查模型：

```bash
check_urdf /tmp/snake_robot.urdf
```

显示机器人：

```bash
ros2 launch snake_description display.launch.py
```

需要在 RViz 和关节滑块中确认：

- 零角度时蛇身笔直；
- `yaw_joint_*` 在水平面弯曲；
- `pitch_joint_*` 在垂直面弯曲；
- 模块编号从蛇头到蛇尾；
- 关节方向、尺寸和机械限位符合实物。

## 9. 模拟 ros2_control 验证

在 `snake_bringup` 完成后，首先使用模拟硬件：

```bash
ros2 launch snake_bringup snake_bringup.launch.py \
  use_mock_hardware:=true
```

检查状态：

```bash
ros2 control list_hardware_components
ros2 control list_hardware_interfaces
ros2 control list_controllers
ros2 topic echo /joint_states
```

预期目标：

- `SnakeSystem` 正常加载；
- 8 个 position command interface 可用；
- `joint_state_broadcaster` 为 `active`；
- `snake_joint_controller` 为 `active`；
- 模拟关节状态能够通过 `/joint_states` 发布。

## 10. STM32 通信边界

ROS 2 主机与 STM32 使用待定义的二进制串口协议；STM32 与 JOHO 舵机继续使用已有 JOHO 协议。两段协议不可混用。

计划中的 PC—STM32 帧应至少包含：

- 帧头；
- 协议版本；
- 消息类型；
- 序号和负载长度；
- 8 个关节目标或反馈；
- CRC16；
- 心跳与错误状态。

在协议确定前，不应开始实现 `SnakeSystemHardware::read()` 和 `write()`，避免上位机与固件分别形成不兼容的数据格式。

## 11. 当前开发状态

- [x] STM32F407 原始工程复制迁移；
- [x] STM32 迁移副本 Makefile 编译验证；
- [x] JOHO 舵机驱动保留；
- [x] 4 模块、8 舵机基础映射；
- [x] `snake_description` 初版；
- [ ] 根据实物完成尺寸、质量、方向和零位标定；
- [ ] 完成 `snake_bringup` 模拟硬件验证；
- [ ] 定义 PC—STM32 通信协议；
- [ ] STM32 USART6 主机通信；
- [ ] `snake_hardware` ros2_control 插件；
- [ ] 真实硬件位置指令和状态闭环；
- [ ] `snake_controller` 步态节点；
- [ ] 遥控、仿真和系统测试。

## 12. 推荐开发顺序

```text
机器人描述验证
      ↓
模拟 ros2_control bringup
      ↓
PC—STM32 协议定义
      ↓
STM32 主机通信与安全机制
      ↓
snake_hardware 硬件插件
      ↓
单关节真实硬件测试
      ↓
8 关节同步控制
      ↓
蛇形步态和遥控
      ↓
实机集成测试
```

## 13. 安全注意事项

- 舵机必须使用合适的独立电源，不能由 USB-UART 供电；
- STM32、USB-UART 和舵机控制总线必须共地；
- 确认 USB-UART 使用 3.3 V TTL 电平，不要直接连接 RS-232 电平；
- 初次实机测试建议限制在约 `±10°~±20°`；
- 上电时先读取当前位置，再将当前位置作为初始目标；
- 必须实现通信超时、命令限位和紧急关闭扭矩；
- 启动 ros2_control 后，禁止其他程序同时占用同一个串口。

## 14. 版本管理建议

使用 `main + 短期 feature 分支`：

```text
main
├── feature/firmware-migration
├── feature/stm32-host-protocol
├── feature/robot-description
├── feature/ros2-control-hardware
├── feature/controllers-bringup
├── feature/snake-gait
└── feature/integration-test
```

功能分支完成编译和对应测试后再合并到 `main`，并在阶段性成果上打版本标签。
