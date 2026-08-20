# F407_JOHO — STM32F407 蛇形机器人舵机控制工程

基于 **STM32F407** 的蛇形机器人步态控制工程。MCU 通过 **USART3（半双工 TTL）** 驱动多路 **JOHO 总线舵机**，实现正交双舵机关节模块组成的蛇身平面蜿蜒 / 侧向蜿蜒等步态。

---

## ✨ 功能特性

- 🐍 **蛇形机器人步态控制**：以“正交双舵机关节模块”（水平轴 Yaw + 垂直轴 Pitch）为单位，最多支持 8 个关节（`GAIT_MAX_JOINTS`）。
- 📐 **参数化步态公式**（弧度制）：

  $$θ_{yi} = α_y \cdot \cos(ω_y t + (i-1)β_y) + γ_y$$

  $$θ_{pi} = α_p \cdot \cos(ω_p t + (i-1)β_p + φ) + γ_p$$

- 🎛️ **多种预设步态**：平面蜿蜒 `GAIT_SERPENTINE`、侧向蜿蜒 `GAIT_SIDEWINDING`、行波、螺旋翻滚、三角步态等。
- 🔢 **灵活的舵机 ID 映射**：支持逐个映射 / 顺序映射、安装方向（±1）与偏移补偿、映射校验（重复 ID 检查）。
- ⚡ **同步写（SyncWrite）**：同一控制周期内用一个包批量更新所有舵机目标位置（最多 16 只）。
- 🔌 **JOHO 总线舵机完整驱动**：Ping / 读位置 / 写角度 / 同步写 / 扭矩控制 / ID 扫描与修改。
- 📦 **环形缓冲区（RingBuffer）**：中断 + 主循环并发安全的 UART 收发缓冲。
- 🔨 **GCC + Make 构建**：无 IDE 依赖，命令行即可编译、烧录。

---

## 📁 目录结构

```
F407_JOHO/
├── F407_JOHO.ioc              # STM32CubeMX 工程文件（引脚/外设配置）
├── Makefile                   # GCC/arm-none-eabi Make 构建脚本
├── startup_stm32f407xx.s      # 启动文件
├── STM32F407XX_FLASH.ld       # 链接脚本
├── flash.txt                  # 烧录说明
├── summary.md                 # 工程开发总结（含踩坑记录）
│
├── Core/                      # 主程序与系统配置（CubeMX 生成 + 用户代码）
│   ├── Inc/                   # 头文件：main.h / gpio.h / usart.h / test_servo.h / sys.h / sys_tick.h ...
│   └── Src/                   # 源码：main.c / usart.c / gpio.c / test_servo.c / stm32f4xx_it.c ...
│
├── APP/                       # 应用层
│   ├── gait.h / gait.c        # ★ 步态控制器（正交双舵机关节、ID 映射、预设步态）
│   └── step.h / step.c        # 旧版螺旋翻滚测试（角度计算）
│
├── HARDWARE/                  # 底层硬件驱动
│   ├── ring_buffer/           # 环形缓冲区（UART 收发）
│   └── uart_servo/            # JOHO 总线舵机驱动 uart_servo_lite.c/.h
│
└── Drivers/                   # CMSIS + STM32F4xx_HAL_Driver（HAL 库）
```

### 核心模块说明

| 模块 | 路径 | 说明 |
|------|------|------|
| 步态控制器 | `APP/gait.c` / `gait.h` | 参数化步态计算、关节映射、同步写下发 |
| 舵机驱动 | `HARDWARE/uart_servo/uart_servo_lite.c` | JOHO 协议封包/解析、Ping、读写、同步写 |
| 环形缓冲 | `HARDWARE/ring_buffer/ring_buffer.c` | 中断安全的 UART 收发队列 |
| 串口封装 | `Core/Src/usart.c` | USART1/3/6 初始化，舵机半双工 GPIO 切换 |
| 舵机工具 | `Core/Src/test_servo.c` | 舵机 ID 扫描、批量改 ID、调试演示 |
| 主程序 | `Core/Src/main.c` | 初始化、步态配置、主循环调度 |

---

## 🧰 硬件环境

- **主控**：STM32F407（正点原子 F407 开发板）
- **执行机构**：JOHO 35kg 总线舵机 × N（正交双舵机组成一个关节模块）
- **通讯**：USART3，半双工 TTL 单总线（S + VCC + GND），波特率 **115200 8N1**
- **转接板**：舵机单线 S → 双线 TX/RX 连接 STM32

### 关键引脚

| 引脚 | 功能 | 说明 |
|------|------|------|
| PB10 | USART3_TX | 发送前切 AF_PP 强推挽，发完切回 OUTPUT/输入释放总线 |
| PB11 | USART3_RX | 必须 `GPIO_MODE_AF_PP`（复用功能），否则收不到数据 |
| PF9 | DS0 LED | 低电平点亮，指示程序运行 |

> ⚠️ 舵机与 STM32 **必须分别独立供电并共地**；转接板 S 线外部上拉到 4V 属正常（PB10/PB11 为 5V 耐受引脚）。

---

## 🔧 软件环境与构建

- **工具链**：`arm-none-eabi-gcc` + `make`（工程由 STM32CubeMX 生成，HAL 库驱动）
- **构建**：

```bash
make            # 编译
make clean      # 清理中间产物（中断编译后必须 clean 再重新编译！）
```

- **烧录**：见 `flash.txt`（可使用 `pyocd` / ST-Link 等工具）。

---

## 🚀 快速开始 / 使用流程

1. **修改配置宏**（`Core/Src/main.c` 顶部 `USER CODE BEGIN PD`）：

   ```c
   #define SERVO_ID_SETUP_MODE  0   // 1=舵机ID配置模式, 0=运行蛇形步态
   #define SNAKE_JOINT_COUNT    4   // 正交双舵机关节模块数
   #define GAIT_BASE_ID         1   // 起始舵机ID（每关节 2 个连续 ID）
   ```

2. **初始化步态控制器**：

   ```c
   GaitController gc;
   Gait_Init(&gc, SNAKE_JOINT_COUNT);
   Gait_SetJointMappingSequential(&gc, GAIT_BASE_ID);  // 或逐个 Gait_SetJointMappingEx
   ```

3. **配置步态参数**（平面蜿蜒示例）：

   ```c
   Gait_ConfigurePlanarSerpentine(&gc, amplitude_deg, period_s, phase_deg);
   Gait_SetRampDuration(&gc, 3000);   // 3 秒振幅渐增，避免突然动作
   Gait_Restart(&gc);
   ```

4. **主循环周期调度**：

   ```c
   while (1) {
       Gait_UpdateSync(&gc, HAL_GetTick());   // 同步写更新所有关节
   }
   ```

---

## 📚 相关链接

- **JOHO（炬晖科技）官网**：<https://www.johorobot.com/>
- **JOHO 知识库 / 资料下载**（UART 总线舵机协议、STM32 SDK、上位机等）：<https://data.johorobot.com/index.html>

---

## 📝 备注

- 本工程为个人学习/调试用途，源码基于 STM32CubeMX 生成模板修改。
- 开发过程中的关键排障经验记录在 `summary.md`（如 PB11 复用模式、半双工释放 TX、帧头只认 `0xF5FF` 等）。
