# ROS 2 主机与 STM32F407 通信协议设计说明

## 1. 文档目的

本文档描述 ROS 2 主机与 STM32F407 下位机之间的串口通信协议，并重点解释每个字段和机制为什么存在。

这套协议是本项目自行定义的 Host Protocol，不是 ROS 2 官方协议，也不是 JOHO 舵机协议。系统中实际存在两条通信链路：

```text
ROS 2 主机
    │
    │ Host Protocol（本文档）
    │ 帧头 AA 55，默认 115200 8N1
    ▼
STM32F407
    │
    │ JOHO UART 总线舵机协议
    │ 帧头 FF FF
    ▼
JOHO 总线舵机
```

STM32 的作用不是运行 ROS 2，而是：

1. 接收 ROS 2 主机发送的关节目标；
2. 检查数据是否完整、合法和安全；
3. 把主机命令转换为 JOHO 舵机协议；
4. 读取舵机状态；
5. 把状态汇总后返回 ROS 2 主机。

## 2. 为什么不能直接发送 ROS 2 消息

ROS 2 控制器输出的是带有消息类型和 DDS 语义的数据，例如 `JointTrajectory` 或 ros2_control 的 position command interface。STM32 裸机程序并不理解：

- ROS 2 topic；
- DDS 序列化；
- `trajectory_msgs/msg/JointTrajectory`；
- controller_manager；
- C++ 对象或动态数组。

串口本身也只提供连续字节流，不提供“这是第几个消息”“消息从哪里开始”“数据是否损坏”等信息。因此必须在串口字节流之上增加一层双方共同遵守的帧协议。

## 3. 串口参数

第一版协议使用：

```text
波特率：115200
数据位：8
停止位：1
校验位：无
硬件流控：无
模式：全双工
```

简称 `115200 8N1`。

STM32 当前计划使用 USART6 与 ROS 2 主机通信：

```text
STM32 PC6 / USART6_TX  → USB-UART RX
STM32 PC7 / USART6_RX  ← USB-UART TX
STM32 GND              ↔ USB-UART GND
```

USART3继续连接 JOHO 舵机，USART1保留为调试日志口，避免日志字节混入 Host Protocol。

## 4. 帧格式

一帧数据定义为：

```text
AA 55 | VERSION | TYPE | SEQ | LEN_L LEN_H | PAYLOAD | CRC_L CRC_H
```

详细布局：

| 偏移 | 长度 | 字段 | 含义 |
| ---: | ---: | --- | --- |
| 0 | 1 | `HEADER_1` | 固定为 `0xAA` |
| 1 | 1 | `HEADER_2` | 固定为 `0x55` |
| 2 | 1 | `VERSION` | 协议版本，第一版为 `0x01` |
| 3 | 1 | `TYPE` | 消息类型 |
| 4 | 1 | `SEQ` | 请求/应答序号 |
| 5 | 1 | `LEN_L` | Payload 长度低字节 |
| 6 | 1 | `LEN_H` | Payload 长度高字节 |
| 7 | N | `PAYLOAD` | 与消息类型相关的数据 |
| 7+N | 1 | `CRC_L` | CRC16 低字节 |
| 8+N | 1 | `CRC_H` | CRC16 高字节 |

总帧长度为：

```text
frame_size = 2 + 5 + payload_length + 2
           = 9 + payload_length
```

没有 Payload 的帧仍然有 9 字节。

## 5. 为什么需要两个字节的帧头

UART 是连续字节流。一次读取可能从任意位置开始，也可能出现：

- 上电垃圾字节；
- 上一帧残留；
- 一帧拆成多次接收；
- 多帧一次性到达；
- 因干扰造成的字节丢失；
- PC 在 STM32 启动前已经发送数据。

例如 STM32 可能看到：

```text
37 00 F2 AA 55 01 01 09 00 00 ...
```

解析器不能把第一个字节当作消息起点，而应持续寻找 `AA 55`。

使用两个字节是为了降低误同步概率。单个 `AA` 很容易出现在 Payload 中；连续出现 `AA 55` 的概率更低。帧头只负责找到候选帧起点，不能证明帧一定正确，最终仍要依靠长度和 CRC。

选择 `AA 55` 没有特殊行业含义，它只是一个易识别、位模式交替明显的同步标记。只要 PC 和 STM32 使用同样的值即可。

## 6. 为什么需要 VERSION

第一版：

```c
#define HOST_PROTOCOL_VERSION 0x01
```

以后协议可能增加：

- 新消息类型；
- 更多关节状态；
- 电流、温度和错误码；
- 不同的 Payload 布局；
- 更大的关节数量；
- 新的安全字段。

如果没有版本号，新主机可能按新格式解析旧固件，或者旧固件错误执行新主机命令。收到不支持的版本时，STM32应丢弃该帧，或者在能够安全解析基本字段时返回版本错误。

协议版本描述的是数据格式，不应与 STM32 固件软件版本混为一谈。

## 7. 为什么需要 TYPE

同一条串口需要承载多种操作，因此用一个字节说明 Payload 的含义。

初步分配：

| TYPE | 名称 | 方向 | 说明 |
| ---: | --- | --- | --- |
| `0x01` | `PING` | PC → STM32 | 通信探测 |
| `0x81` | `PONG` | STM32 → PC | PING 应答 |
| `0x10` | `SET_ONE_SERVO` | PC → STM32 | 单舵机测试命令 |
| `0x90` | `COMMAND_ACK` | STM32 → PC | 命令执行结果 |
| `0x11` | `SET_ALL_JOINTS` | PC → STM32 | 整组关节位置命令，后续实现 |
| `0x91` | `ALL_JOINTS_ACK` | STM32 → PC | 整组命令应答，后续实现 |
| `0x20` | `READ_STATE` | PC → STM32 | 请求状态，后续实现 |
| `0xA0` | `JOINT_STATE` | STM32 → PC | 关节状态反馈，后续实现 |
| `0x30` | `SET_TORQUE` | PC → STM32 | 扭矩开关，后续实现 |
| `0x40` | `HEARTBEAT` | PC → STM32 | 主机存活通知，后续实现 |

使用以下编号约定便于调试：

```text
0x01～0x7F：PC 发出的请求或命令
0x81～0xFF：STM32 返回的应答或状态
```

请求和应答通常相差 `0x80`，例如 `PING=0x01`、`PONG=0x81`。这只是本项目约定，不是 UART 标准。

## 8. 为什么需要 SEQ

`SEQ` 是 0～255 循环的消息序号。PC 每发送一个请求就递增：

```text
0, 1, 2, ... 254, 255, 0, 1 ...
```

STM32 返回应答时复制请求序号：

```text
PC    → PING，SEQ=27
STM32 → PONG，SEQ=27
```

这样 PC 才能判断应答属于哪一个请求。它可以识别：

- 上一个请求的迟到应答；
- 重复帧；
- 丢失的应答；
- 顺序错误；
- STM32 重启后序列异常。

`SEQ` 不等同于可靠传输。协议第一版不自动重发，是否超时重试由 PC 测试工具或 `snake_hardware` 决定。

## 9. 为什么需要 LEN

不同消息的 Payload 长度不同：

```text
PING                  0 或 4 字节
SET_ONE_SERVO         5 字节
SET_ALL_JOINTS        变长
JOINT_STATE           变长
```

`LEN` 让解析器知道 Payload 到哪里结束，以及 CRC 在哪里。

长度使用 16 位小端：

```text
payload_length = LEN_L | (LEN_H << 8)
```

示例：

```text
长度 5   → 05 00
长度 300 → 2C 01
```

当前实现仍限制：

```c
#define HOST_MAX_PAYLOAD_SIZE 64
```

保留两字节长度是为了将来扩展，64 字节限制则用于保护 STM32 RAM 和防止错误长度导致数组越界。解析器必须先检查：

```c
if (payload_length > HOST_MAX_PAYLOAD_SIZE)
{
    /* 立即丢弃，不继续写入数组 */
}
```

## 10. 为什么需要 Payload

Payload 保存真正的业务数据，其解释方式由 TYPE 决定。协议公共层只负责安全地传输字节，不应该猜测业务含义。

### 10.1 PING/PONG Payload

PING 可以使用空 Payload，也可以携带 PC 单调时钟时间戳：

```text
host_time_ms: uint32
```

STM32在 PONG 中原样返回。PC 可检查 Payload 是否一致，并计算往返延迟：

```text
RTT = 接收 PONG 的时间 - 发送 PING 的时间
```

原样回传比只返回固定字符串覆盖更多测试范围，因为它同时验证了 Payload 长度、字节顺序和内容传输。

### 10.2 单舵机测试 Payload

`SET_ONE_SERVO` 定义为：

```text
servo_id       uint8
target_raw     uint16 little-endian
move_time_ms   uint16 little-endian
```

布局：

```text
ID | RAW_L RAW_H | TIME_L TIME_H
```

例如 ID1、原始位置2048、运行时间500 ms：

```text
01 00 08 F4 01
```

STM32不能收到后立即执行，必须先检查：

```text
Payload 长度必须等于 5
servo_id 必须在 1～250
target_raw 必须在 0～4095
move_time_ms 必须位于项目允许范围
当前是否允许主机控制
```

所有检查通过后才能调用 JOHO 驱动。

### 10.3 整组关节命令 Payload

真实 ros2_control 运行时不应逐个发送 8 条单舵机命令，否则会产生启动时间差和额外串口负载。计划使用：

```text
move_time_ms    uint16
joint_count     uint8

重复 joint_count 次：
    servo_id    uint8
    target_raw  uint16
```

STM32解析并检查全部关节后，一次调用 JOHO `SYNC_WRITE`。任何一个关节字段非法时，整组命令都不执行，避免机器人进入部分更新状态。

## 11. 为什么 PC—STM32 使用小端

STM32F407 Cortex-M4 和常见 x86 ROS 2 主机都是小端，因此 Host Protocol 的多字节整数使用小端可以简化处理。

例如 2048，即 `0x0800`：

```text
PC → STM32：00 08（小端）
```

但 JOHO 协议使用自己的大端编码：

```text
STM32 → JOHO：08 00（大端）
```

两段协议的字节序不同并不矛盾。STM32读取 Host Protocol 的数值后，再通过 JOHO 驱动函数按 JOHO 格式重新编码。禁止直接把 PC Payload 原样转发给舵机。

## 12. CRC16 的定义与原因

协议使用 CRC16-CCITT：

```text
初始值：0xFFFF
多项式：0x1021
输出：16 bit
传输字节序：低字节在前
```

CRC计算范围：

```text
VERSION | TYPE | SEQ | LEN_L | LEN_H | PAYLOAD
```

不包含 `AA 55` 帧头。帧头只用于同步，真正影响解析和执行的字段全部受到 CRC 保护。

CRC用于检测：

- 单比特错误；
- 多比特错误；
- 部分突发错误；
- 长度或消息类型被干扰修改；
- Payload 中舵机位置被修改。

接收方重新计算 CRC：

```text
received_crc == calculated_crc → 继续解析
received_crc != calculated_crc → 丢弃整帧
```

CRC不是加密，也不能阻止恶意命令。它只用于检测传输错误。

CRC错误时通常不能返回普通 ACK，因为 TYPE、SEQ 和长度本身可能已经损坏，贸然应答可能对应错误请求。应记录错误计数，然后重新寻找下一帧。

## 13. 字节流解析状态机

UART 接收可能出现半包和粘包：

```text
半包：一次只收到一帧的前几个字节
粘包：一次收到两帧甚至更多帧
```

因此不能假设一次 `HAL_UART_Receive` 就是一帧。解析器逐字节运行：

```text
WAIT_HEADER_1
      │ 收到 AA
      ▼
WAIT_HEADER_2
      │ 收到 55
      ▼
READ_BODY
      │ 收满固定字段、Payload 和 CRC
      ▼
长度检查 → CRC检查 → 版本检查 → 生成 HostFrame
      │
      └──────────────────────────→ WAIT_HEADER_1
```

### 13.1 WAIT_HEADER_1

忽略所有非 `AA` 字节。收到 `AA` 后进入下一状态。

### 13.2 WAIT_HEADER_2

- 收到 `55`：确认候选帧头；
- 再次收到 `AA`：它可能是新帧头的第一个字节，继续等待 `55`；
- 收到其他字节：帧头匹配失败，回到 `WAIT_HEADER_1`。

### 13.3 READ_BODY

先收固定 5 字节：

```text
VERSION TYPE SEQ LEN_L LEN_H
```

得到 LEN 后计算：

```text
expected_body_size = 5 + payload_length + 2
```

收满后按顺序执行：

1. 长度边界检查；
2. CRC检查；
3. 版本检查；
4. 复制 TYPE、SEQ 和 Payload；
5. 设置 `frame_ready`；
6. 重置解析器，继续处理后续字节。

这种逐字节状态机不关心底层一次接收多少字节，因此既能处理半包，也能处理粘包。

## 14. PING/PONG 流程

```text
PC                                      STM32
 │                                        │
 │ 构造 PING                              │
 │ TYPE=0x01，SEQ=12，Payload=时间戳       │
 ├──────────── PING seq=12 ──────────────►│
 │                                        │ 查找 AA 55
 │                                        │ 检查 LEN
 │                                        │ 检查 CRC
 │                                        │ 检查 VERSION
 │                                        │ 识别 TYPE=PING
 │                                        │
 │                                        │ 构造 PONG
 │                                        │ TYPE=0x81
 │                                        │ SEQ=12
 │                                        │ 原样复制 Payload
 │◄──────────── PONG seq=12 ──────────────┤
 │                                        │
 │ 检查 CRC、TYPE、SEQ、Payload            │
 │ 计算往返延迟                            │
```

PING/PONG 不控制舵机。它被优先实现，是因为它可以在不产生机械运动风险的情况下验证：

- TX/RX接线；
- 波特率；
- 帧同步；
- 长度解析；
- 大小端；
- CRC一致性；
- 请求应答匹配；
- 通信延迟和稳定性。

## 15. 命令 ACK 和错误码

能够通过 CRC 只代表数据没有明显损坏，不代表命令合法或执行成功。STM32还需要业务检查，因此命令应答应包含执行结果。

建议 `COMMAND_ACK` Payload：

```text
request_type    uint8
result          uint8
target_id       uint8
```

建议状态码：

| 值 | 名称 | 含义 |
| ---: | --- | --- |
| `0x00` | `OK` | 命令执行成功 |
| `0x01` | `INVALID_LENGTH` | Payload 长度错误 |
| `0x02` | `INVALID_SERVO_ID` | 舵机 ID 非法 |
| `0x03` | `INVALID_POSITION` | 目标位置越界 |
| `0x04` | `SERVO_TIMEOUT` | 舵机无响应 |
| `0x05` | `SERVO_ERROR` | 舵机返回错误状态 |
| `0x06` | `UNSUPPORTED_TYPE` | 不支持的消息类型 |
| `0x07` | `WRONG_CONTROL_MODE` | 当前不允许主机控制 |

ACK中的 SEQ 必须复制原请求 SEQ。

## 16. 心跳与安全状态

真实机器人不能永久执行最后一次收到的运动命令。PC崩溃、USB断开或 ROS 2 节点退出时，STM32必须能够识别通信失效。

计划由 PC 周期发送 `HEARTBEAT`，STM32记录最后有效帧时间：

```text
正常：持续收到合法心跳或关节命令
超时：超过 command_timeout_ms 未收到合法主机帧
```

超时后进入安全状态，可根据实机需求选择：

- 停止更新并保持当前位置；
- 缓慢回中；
- 关闭扭矩；
- 拒绝执行迟到的旧命令。

早期测试建议先“保持当前位置并拒绝新动作”，确认机械安全后再确定最终策略。

## 17. 本地步态与主机控制冲突

当前 STM32 固件能够自行运行蛇形步态。接入 ROS 2 后必须明确唯一命令源，否则本地步态和 ROS 2 会互相覆盖。

建议状态：

```c
typedef enum
{
    CONTROL_MODE_LOCAL_GAIT,
    CONTROL_MODE_HOST,
    CONTROL_MODE_SAFE
} ControlMode;
```

- `LOCAL_GAIT`：执行 STM32 自带步态；
- `HOST`：只执行 Host Protocol 命令，停止本地 `Gait_UpdateSync()`；
- `SAFE`：通信异常或错误状态，不执行运动命令。

控制模式切换必须是显式且可诊断的，不能仅凭收到任意串口字节就切换到主机控制。至少需要收到 CRC、版本和命令内容都正确的合法帧。

## 18. 第一阶段测试计划

### 18.1 PING/PONG

目标：连续发送至少 1000 次 PING。

验收：

- PONG 数量与 PING 一致；
- TYPE正确；
- SEQ正确；
- Payload原样返回；
- CRC全部正确；
- STM32不阻塞原主循环。

### 18.2 异常帧

主动测试：

- 帧头前加入随机垃圾字节；
- 分多次发送一帧；
- 一次发送多帧；
- 修改 Payload 但保留旧 CRC；
- 声明超长 LEN；
- 发送不支持的 VERSION；
- 发送未知 TYPE；
- 发送到一半停止。

解析器应丢弃错误帧，并能够在后续合法 `AA 55` 处恢复。

### 18.3 单舵机

PING/PONG稳定后，才连接一个舵机测试 `SET_ONE_SERVO`。初始动作限制在机械零位附近约 `±10°`，并验证 ACK 和实际位置反馈。

### 18.4 整组关节

最后实现 `SET_ALL_JOINTS`，一次更新8个舵机，并由 STM32调用 JOHO `SYNC_WRITE`。

## 19. 与 ros2_control 的最终对应关系

```text
joint_trajectory_controller
        │ 关节目标，单位 rad
        ▼
SnakeSystemHardware::write()
        │ 限位、方向、零位补偿
        │ rad → 0～4095
        ▼
SET_ALL_JOINTS Host Protocol 帧
        │ USART6
        ▼
STM32 检查并执行 JOHO SYNC_WRITE
```

反馈：

```text
STM32读取 JOHO 舵机
        │ 汇总原始位置和状态
        ▼
JOINT_STATE Host Protocol 帧
        │ USART6
        ▼
SnakeSystemHardware::read()
        │ 0～4095 → rad
        ▼
joint_state_broadcaster → /joint_states
```

## 20. 设计原则总结

这套协议的核心逻辑是：

1. `AA 55`：从无边界串口字节流中寻找帧起点；
2. `VERSION`：允许协议以后升级并拒绝不兼容数据；
3. `TYPE`：说明 Payload 的业务含义；
4. `SEQ`：让请求和响应一一对应；
5. `LEN`：支持可变长度数据并确定帧结束位置；
6. `CRC16`：避免损坏的数据驱动机器人；
7. `ACK`：区分“收到数据”和“成功执行命令”；
8. `HEARTBEAT`：在主机断线后让机器人进入安全状态；
9. 状态机解析：处理半包、粘包、垃圾字节和丢字节；
10. STM32协议转换：隔离 ROS 2 语义与 JOHO 舵机总线细节。

第一版先追求简单、可验证和安全，而不是一次加入所有功能。正确顺序是：

```text
固定字符串 → 字节回显 → PING/PONG → 异常帧测试
→ 单舵机命令 → 单舵机反馈 → 8关节同步命令
→ 心跳和安全模式 → ros2_control 接入
```
