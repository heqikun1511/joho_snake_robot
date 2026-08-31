# 蛇形机器人 ROS 2 控制系统

本分支用于设计和实现蛇形机器人的 ROS 2 / ros2_control 控制系统。树莓派负责步态生成、控制模式切换、导航和监控，STM32 负责确定性的关节命令执行、舵机总线通信和底层安全保护。

## 系统分层

```text
导航 / 遥控 / cmd_vel
          ↓
snake_gait_controller
速度与转向 → 步态参数 → 各关节目标角
          ↓
ros2_control position command interfaces
          ↓
snake_mcu_hardware
关节角数组 ↔ SPI 协议
          ↓
STM32
校验 / 限位 / 看门狗 → UART SyncWrite
          ↓
总线舵机

舵机状态 → STM32 → SPI → ros2_control state interfaces
                              ↓
                    joint_state_broadcaster
                              ↓
                         /joint_states
```

### ROS 2 的职责

- 生成平面蜿蜒、侧向蜿蜒、螺旋翻滚等步态。
- 将 `cmd_vel` 转换为振幅、频率、相位差和转向偏置。
- 输出每个关节的目标位置，内部单位统一使用弧度。
- 负责控制模式切换、参数调整、仿真、数据记录和导航集成。

### STM32 的职责

- 解析和校验 SPI 命令帧。
- 执行关节角度、速度和变化率限制。
- 保存舵机 ID、安装方向和机械零偏映射。
- 使用 UART SyncWrite 同步更新全部舵机。
- 实现通信看门狗、急停和故障上报。
- 缓存并返回关节位置、通信计数和错误状态。

固件中已有的三角函数步态可以保留为脱机测试或失联后的安全备用模式，但正常 ROS 2 运行时只允许一个命令源控制舵机。不要在 `snake_mcu_hardware::write()` 中计算步态；硬件插件只负责关节接口与 SPI 数据帧之间的转换。

## ROS 包职责

| 包 | 职责 |
| --- | --- |
| `snake_description` | URDF/Xacro、关节名称、方向、限位和 ros2_control 描述 |
| `snake_mcu_hardware` | SPI 通信和 `hardware_interface::SystemInterface` |
| `snake_gait_controller` | 三角函数步态生成和 `cmd_vel` 映射 |
| `snake_interfaces` | 步态命令、机器人状态和故障消息 |
| `snake_bringup` | 启动 Controller Manager、硬件和控制器 |
| `snake_safety` | 急停、超时、状态检查和命令限幅 |
| `snake_sim_hardware` | 不连接实体硬件时的仿真/假硬件接口 |
| `snake_sim_bringup` | 仿真系统启动入口 |
| `snake_navigation` | 导航速度到蛇形步态请求的转换 |
| `snake_joint_state_estimator` | 反馈不足时的关节状态估计 |

## 推荐开发顺序

### 1. 建立机器人描述

在 `snake_description` 中完成 URDF/Xacro。当前 4 个正交双轴模块应定义为 8 个转动关节，例如：

```text
joint_1_yaw    joint_1_pitch
joint_2_yaw    joint_2_pitch
joint_3_yaw    joint_3_pitch
joint_4_yaw    joint_4_pitch
```

先在 RViz 中确认关节顺序、旋转方向、零位和限位正确。

### 2. 建立 ros2_control 位置接口

为每个关节提供：

```text
command interface: position
state interface:   position
```

第一版只实现位置控制。速度、温度、电压和故障状态在基本闭环稳定后再增加。

### 3. 使用假硬件打通控制链

先使用 mock/generic system 或 `snake_sim_hardware`，不连接 STM32 和舵机。启动：

- `controller_manager`
- `joint_state_broadcaster`
- 关节组位置控制器
- `robot_state_publisher`
- RViz

确认位置命令能够反映到 `/joint_states` 和机器人模型。

### 4. 实现第一版步态节点

先将 `snake_gait_controller` 实现为普通 ROS 2 节点，以 50 Hz 计算并发布 8 个关节目标角。移植现有固件公式：

```text
theta_y(i) = alpha_y * cos(omega_y * t + i * beta_y) + gamma_y
theta_p(i) = alpha_p * cos(omega_p * t + i * beta_p + phi) + gamma_p
```

第一阶段只实现低振幅、低频率的基础平面蜿蜒。先在 RViz 和假硬件中验证，不直接驱动实体舵机。系统稳定后，再考虑把该节点改为自定义 ros2_control Controller 插件。

### 5. 定义 STM32 SPI 协议

在已经验证的 SPI 物理链路上增加正式协议，至少支持：

```text
CMD_PING
CMD_ENABLE
CMD_DISABLE
CMD_SET_JOINTS
CMD_GET_JOINTS
CMD_GET_STATUS
CMD_ESTOP
```

`CMD_SET_JOINTS` 应在一帧中发送全部关节，不要逐关节发送。建议使用 `int16_t position_mrad` 表示弧度，并加入协议版本、帧序号、有效关节数量和 CRC16。

### 6. 完成 STM32 执行层

STM32 接收全部关节目标后执行：

1. 校验帧、序号和关节数量。
2. 检查关节角度与变化率。
3. 应用安装方向和零偏。
4. 转换为舵机原始位置。
5. 使用 `USL_SyncWriteAngles()` 同步下发。
6. 缓存关节和故障状态供树莓派读取。

设置约 200 ms 的通信看门狗。超时后的保持、回中或关闭扭矩策略必须通过实体机器人安全测试后确定。

### 7. 实现 `snake_mcu_hardware`

将整个机器人实现为一个 `hardware_interface::SystemInterface`，因为全部关节共享一块 STM32 和同一条 SPI 总线：

- `write()`：把最新关节位置命令打包并发送给 STM32。
- `read()`：读取并发布 STM32 缓存的关节状态。
- `on_activate()`：完成握手、清除旧命令并安全使能。
- `on_deactivate()`：停止运动并安全释放资源。
- 通信错误：返回明确状态，并交给安全层处理。

第一版可以同步通信；后续将 SPI 放入独立工作线程，避免 `read()`/`write()` 因等待舵机反馈而阻塞 Controller Manager。

### 8. 实体机器人逐级测试

按以下顺序扩大测试范围：

1. 不接舵机，只检查 SPI 命令和状态。
2. 接一个舵机，小角度测试方向与零位。
3. 接一个双轴关节模块。
4. 接全部关节，保持低振幅、低频率。
5. 验证急停、ROS进程退出、SPI断线和STM32复位。

不要第一次就运行完整步态。

### 9. 加入 `cmd_vel` 映射

基础步态稳定后，再将速度命令映射为步态参数：

```text
linear.x  → 步态频率/前进速度
angular.z → 左右关节偏置/转向曲率
```

所有参数变化都应进行斜坡限制，避免振幅、频率或方向瞬间跳变。

### 10. 集成安全、监控和导航

最后加入：

- `snake_safety` 的急停和健康状态机。
- Foxglove/RViz状态显示和 rosbag记录。
- 舵机温度、电压和离线状态。
- `snake_navigation` 与自主导航。
- 更复杂的侧向蜿蜒、翻滚和步态切换。

## 第一阶段里程碑

不连接舵机，让 `snake_gait_controller` 在假硬件和 RViz 中以 50 Hz 生成 8 个关节目标角，并通过 `/joint_states` 正确显示运动。完成这一里程碑后，再开发 `snake_mcu_hardware` 和 STM32 关节数组协议。

建议初始控制频率：

```text
Controller Manager：50 Hz
步态计算：          50 Hz
SPI命令：           50 Hz
舵机 SyncWrite：    50 Hz
舵机反馈：       10~20 Hz
```
