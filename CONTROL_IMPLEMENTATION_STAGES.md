# Control 部分实现阶段说明

本文档按阶段记录蛇形机器人 Control 部分的目标、主要改动和验收方法。
开发顺序为：模型 → 假硬件 → 步态 → 速度控制 → 安全 → STM32 实机。

## 阶段 1：机器人描述

状态：已完成基础版本。

改动：

- 在 `snake_description` 中建立 URDF/Xacro 模型。
- 定义 4 个双轴模块，共 8 个 yaw/pitch 转动关节。
- 定义关节旋转轴、零位、速度限制和正负 60 度位置限制。
- 添加 `robot_state_publisher` 和 RViz 模型查看入口。

验收：RViz 能显示模型，8 个关节的名称、顺序、方向和零位正确。

## 阶段 2：ros2_control 位置接口

状态：已完成基础版本。

改动：

- 添加 `ros2_control` 系统描述。
- 每个关节提供 `position` command interface 和 state interface。
- 当前使用 `mock_components/GenericSystem`，不连接 STM32 和舵机。

验收：Controller Manager 能识别全部 8 个关节接口。

## 阶段 3：假硬件控制链

状态：已完成基础版本。

改动：

- 在 `snake_bringup` 中添加 `mock_control.launch.py`。
- 启动 `robot_state_publisher` 和 `controller_manager`。
- 配置 `joint_state_broadcaster`。
- 配置 `snake_position_controller`，类型为 `ForwardCommandController`。
- 位置命令话题为 `/snake_position_controller/commands`。

验收：

```bash
ros2 control list_controllers
```

`joint_state_broadcaster` 和 `snake_position_controller` 都应为 `active`。

## 阶段 4：基础步态生成器

状态：开发中。

改动：

- 新建 Python 包 `snake_gait_controller`。
- 新建 `GaitController` 节点。
- 使用 50 Hz 定时器生成并发布 8 个关节目标角。
- 添加 yaw/pitch 振幅、频率、相位差、中心偏置和关节限位参数。
- 使用 `Float64MultiArray` 发布位置命令。
- 发布数组顺序与 `controllers.yaml` 中的关节顺序一致。

基础公式：

```text
yaw(i)   = Ay × cos(2πft + iβ) + γy
pitch(i) = Ap × cos(2πft + iβ + φ) + γp
```

- `Ay`、`Ap`：摆动幅度。
- `f`：步态频率，单位 Hz。
- `β`：相邻模块的相位差，用于形成沿身体传播的波。
- `φ`：yaw 和 pitch 运动之间的相位差。
- `γy`、`γp`：中心位置偏置。

验收：节点以约 50 Hz 发布命令；每条命令包含 8 个有限值；目标角不超过关节限位；RViz 中运动连续。

## 阶段 5：步态节点接入 Bringup

状态：开发中。

改动：

- 在 `snake_bringup/package.xml` 中加入 `snake_gait_controller` 依赖。
- 在 `mock_control.launch.py` 中创建步态节点。
- 将节点加入 `LaunchDescription`，实现一键启动。
- 后续增加 `enable_gait` 参数，允许启动假硬件但不自动运动。

验收：执行下面的命令后能看到 `/snake_gait_controller` 节点和持续的关节命令。

```bash
ros2 launch snake_bringup mock_control.launch.py
```

## 阶段 6：接入 /cmd_vel

状态：开发中，尚需完成并验证。

改动：

- 订阅 `geometry_msgs/msg/Twist` 类型的 `/cmd_vel`。
- 使用 `linear.x` 生成目标步态频率。
- 使用 `angular.z` 生成目标转向偏置。
- 添加最大频率、最大转向量和命令超时限制。
- 超时未收到 `/cmd_vel` 时停止运动。

### frequency 为什么与速度关联

`frequency` 是身体波形的摆动频率，不是机器人的平移速度：

```text
frequency：Hz
linear.x：m/s
```

期望速度越大，通常需要身体波传播得更快，因此第一版可使用经验映射：

```text
目标步态频率 = frequency_gain × linear.x
```

两者只是相关，并不相等。实际移动速度还受振幅、相位差、身体尺寸、地面摩擦和负载影响，后续需要通过实体实验标定。

### gain 的作用

`gain` 是输入和输出之间的比例系数，用于单位换算和灵敏度调整：

```text
frequency = frequency_gain × linear.x
steering_offset = steering_gain × angular.z
```

- `frequency_gain` 决定同一个前进速度指令对应多快的摆动。
- `steering_gain` 决定同一个转向指令对应多大的关节偏置。
- 不加 gain 等于强制使用 1:1 映射，通常不符合实体机器人特性。
- gain 不一定无量纲，它也承担不同物理单位之间的转换。

验收：正负 `linear.x` 产生相反方向的波；正负 `angular.z` 产生相反转向；命令超时后停止；超大输入不会突破限制。

## 阶段 7：平滑与安全限制

状态：未实现。

计划改动：

- 对频率、振幅和转向偏置增加斜坡限制。
- 使用 `phase += 2πf × dt` 连续更新相位，避免改变频率时波形跳变。
- 限制每周期关节目标变化量。
- 启动、停止和模式切换时平滑回到安全位置。
- 拒绝 NaN、无穷值和长度错误的命令。

验收：速度指令阶跃变化时，关节目标仍连续且不产生明显冲击。

## 阶段 8：树莓派与 STM32 协议

状态：未实现。

计划改动：

- 定义 `PING`、`ENABLE`、`DISABLE`、`SET_JOINTS`、`GET_JOINTS`、`GET_STATUS` 和 `ESTOP`。
- 一帧发送全部关节目标。
- 加入协议版本、帧序号、有效关节数量和 CRC16。
- 统一 ROS 弧度与协议整数单位之间的换算。
- 定义超时、错误状态和兼容策略。

验收：不连接舵机时，树莓派与 STM32 能稳定交换测试帧，并能识别错误 CRC、错误长度和重复序号。

## 阶段 9：真实硬件插件

状态：未实现。

计划改动：

- 在 `snake_mcu_hardware` 中实现 `hardware_interface::SystemInterface`。
- `write()` 将 8 个关节目标发送给 STM32。
- `read()` 获取关节位置和状态。
- `on_activate()` 完成握手、清除旧命令并安全使能。
- `on_deactivate()` 停止运动并释放资源。
- 导出 pluginlib 插件，添加实体硬件描述和 `hardware.launch.py`。

验收顺序：不接舵机检查 SPI → 单舵机 → 单个双轴模块 → 全部关节低速测试。

## 阶段 10：命令仲裁、安全和反馈

状态：未实现。

计划改动：

- `snake_command_gate` 管理遥控、导航和测试命令的优先级。
- 保证同一时刻只有一个有效命令源。
- `snake_safety` 实现急停、通信超时、状态检查和命令限幅。
- `snake_joint_state_estimator` 在反馈不足时估计关节状态。
- 上报舵机离线、温度、电压、SPI 错误和看门狗状态。
- 验证 ROS 进程退出、SPI 断线和 STM32 复位时的安全行为。

最终验收：任何命令源或通信链路失效时，机器人均能进入确定的安全状态；机器人能够低速启动、转向、停止和恢复。

## 当前开发边界

当前应优先完成阶段 4 至阶段 7，并在假硬件和 RViz 中验证。阶段 8 以后才连接 STM32 和实体舵机。在安全层、超时保护和限幅尚未验证前，不应直接运行完整实体步态。
