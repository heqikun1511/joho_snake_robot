# F407_JOHO 舵机UART通讯工程总结

## 项目概述
STM32F407 通过 USART3 控制 JOHO 35kg 总线舵机。
- 舵机协议：单线半双工 TTL（S + VCC + GND）
- 转接板：将舵机单线S转为双线 TX/RX 连接 STM32
- 波特率：115200，8N1

---

## 问题1：PB11收不到数据

### 现象
RX count 始终为0，[RAW RX] 0 bytes，完全收不到任何数据。

### 原因
PB11 被配置为 `GPIO_MODE_INPUT`（普通输入）。STM32 的 USART 外设通过**复用功能（Alternate Function）**连接引脚，普通 GPIO 输入模式会断开 USART 连接。

### 修改

**修改前（❌ 错误）：**
```c
// usart.c - PB11配置
GPIO_InitStruct.Pin = GPIO_PIN_11;
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;    // ← 错误！USART连接断开
GPIO_InitStruct.Pull = GPIO_NOPULL;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
```

**修改后（✅ 正确）：**
```c
// usart.c - PB11配置
GPIO_InitStruct.Pin = GPIO_PIN_11;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;    // ← 必须用复用模式
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF7_USART3; // ← USART3复用功能
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
```

### 教训
STM32 所有外设的 IO 引脚必须用 `GPIO_MODE_AF_xx`（复用功能模式），普通 GPIO 模式无法连接到外设。

---

## 问题2：Ping显示"OK"但实际没通

### 现象
US_Ping 返回 `SUCCESS`，但实际舵机没有回复 `FF F5`。
测试输出显示 [RAW RX] 0 bytes 但仍返回成功。

### 原因
帧头检测代码错误地接受了 `0xFFFF`（请求帧头）作为有效响应帧头。
回显 `FF FF 01 02 01 FB` 被解析为有效响应，校验和也恰好匹配。

### 修改

**修改前（❌ 错误——接受0xFFFF）：**
```c
// uart_servo_lite.c - 帧头检测
if (hdrBE == JOHO_PACK_RESPONSE_HEADER || hdrLE == JOHO_PACK_RESPONSE_HEADER ||
    hdrBE == JOHO_PACK_REQUEST_HEADER || hdrLE == JOHO_PACK_REQUEST_HEADER) {
    // 帧头匹配成功 ← 0xFFFF也被接受！
    pkg->header = hdrBE;
    pkg->status = JOHO_RECV_FLAG_HEADER;
}
```

**修改后（✅ 正确——只接受0xF5FF）：**
```c
// uart_servo_lite.c - 帧头检测
if (hdrBE == JOHO_PACK_RESPONSE_HEADER || hdrLE == JOHO_PACK_RESPONSE_HEADER) {
    // 只接受响应帧头 0xF5FF
    pkg->header = JOHO_PACK_RESPONSE_HEADER;
    pkg->status = JOHO_RECV_FLAG_HEADER;
}
```

### 教训
请求帧头和响应帧头必须严格区分。
JOHO协议：请求 `0xFFFF`，响应 `0xFFF5`（小端）/ `0xF5FF`（大端）。

---

## Git 仓库瘦身记录

### 问题
Git 仓库体积膨胀到 **3.1 GB**，虽然加了 `.gitignore` 仍然无法上传。

### 根因分析

| 问题 | 大小 | 说明 |
|------|------|------|
| `.git/` pack 文件 | **3.01 GiB** | 历史中包含了大量大文件的每个版本 |
| `.vscode/browse.vc.db` | **1.4 GB × 7 个版本** | VS Code 的 C/C++ 浏览缓存数据库被误提交，每次保存都会变更 |
| `build/` 目录（81 个文件） | 被 Git 跟踪 | 虽然 `.gitignore` 写了 `build/`，但文件是在规则**之前**就被跟踪了，Git 不会自动 untrack |
| `build/*.lst` 等多版本 | 每个 ~1 MB | .lst 等汇编列表文件有多份历史版本 |

### 解决方案

#### 第一步：更新 `.gitignore`（预防未来）
完善了 `.gitignore`，新增忽略规则：
- VS Code 缓存：`browse.vc.db` / `ipch/` / `tags/`
- Python 虚拟环境：`.venv/` / `__pycache__/` / `*.pyc`
- 更多构建产物：`*.crf` / `*.htm` / `*.dep` / `*.axf` / `*.sct`
- 编辑器备份：`*.bk` / `*.bak` / `*.patch`

#### 第二步：取消已跟踪文件的追踪
```bash
# 取消跟踪 build/ 目录（保留本地文件）
git rm -r --cached build/
git commit -m "chore: 取消跟踪 build 目录"
```

#### 第三步：从 Git 历史中彻底删除大文件
使用 `git-filter-repo` 重写历史，从所有提交中永久删除：
```bash
pip install git-filter-repo
git filter-repo --path '.vscode/browse.vc.db' \
                --path '.vscode/browse.vc.db-shm' \
                --path '.vscode/browse.vc.db-wal' \
                --path 'build/' \
                --invert-paths --force
```

> ⚠️ `git filter-repo` 会重写所有历史提交 hash。如果是多人协作项目，所有协作者需要重新 clone。

### 效果对比

| 指标 | 优化前 | 优化后 |
|------|--------|--------|
| `.git/` 目录 | **3.1 GB** | **5.6 MB** |
| Git pack | **3.01 GiB** | **~5 MiB** |
| 工作目录 | 1.4 GB（含 browse.vc.db） | 63 MB |
| 提交数 | 12 | 12（重写后） |

### 远程推送须知
因为 `git filter-repo` 重写了历史，推送时需要强制推送：
```bash
git push origin main --force
```

> ⚠️ 强制推送后，其他协作者需要重新 clone，或执行 `git fetch --force && git reset --hard origin/main`。

### 日常维护建议
1. **提交前检查**：用 `git status` 确认没有意外跟踪大文件
2. **善用 `git status --short`**：`??` 开头的未跟踪文件如果不应提交，及时加入 `.gitignore`
3. **`.gitignore` 模板**：STM32 项目可参考 [github/gitignore/C](https://github.com/github/gitignore/blob/main/C.gitignore)

---

## 问题3：舵机不回复（推挽输出导致S线被强拉高）

### 现象
STM32发送后舵机能执行指令，但舵机无法回复数据。
转接板S线测量电压约4V。

### 原因
PB10（TX）配置为 `AF_PP`（推挽输出），空闲时STM32强拉高电平。
舵机的驱动器无法克服STM32的推挽驱动，无法把S线拉低发送数据。

### 修改

**修改前（❌ 错误——一直推挽输出）：**
```c
// usart.c
void Usart_SendAll(Usart_DataTypeDef *usart)
{
    // ... 直接发送，发完后PB10仍然是AF_PP推挽输出
    HAL_UART_Transmit(usart->huart, data, len, HAL_MAX_DELAY);
}
```

**修改后（✅ 正确——发完释放S线）：**
```c
// usart.c - 新增TX_Enable/TX_Release
static void TX_Enable(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static void TX_Release(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;    // ← 切换到输入，释放S线
    gpio.Pull = GPIO_NOPULL;        // ← 转接板已有外置上拉
    HAL_GPIO_Init(GPIOB, &gpio);
}

void Usart_SendAll(Usart_DataTypeDef *usart)
{
    TX_Enable();                      // 发前切推挽
    // ... 发送数据 ...
    HAL_UART_Transmit(usart->huart, data, len, HAL_MAX_DELAY);
    TX_Release();                     // 发后切输入，释放S线
}
```

### 教训
半双工通信必须发送后**释放总线**。STM32推挽输出在空闲时强拉高电平，
从机无法驱动总线。切为INPUT高阻后，外部上拉保持电平，从机能轻松拉低。

---

## 问题3补充：TX释放后PB10仍为3.3V导致接收失败

### 现象
发送后 `TX_Release()` 将 PB10 切为 `INPUT`，但万用表测量 PB10 仍为 3.3V。

### 原因
转接板有外部上拉电阻，PB10 设为 INPUT 后虽为高阻，但外部上拉将其拉到 3.3V。
此电压通过转接板耦合到 RX 线路，导致接收到的全是自己发送指令的回显
（如收到 `FF 0A 02 01 F2 00`，实际发送的是 `FF FF 0A 02 01 F2`），
舵机的 `FF F5` 应答被淹没。

### 尝试的解决方案（失败）

**方案A：TX_Release 改为 OUTPUT_PP 输出低电平**
```c
gpio.Mode = GPIO_MODE_OUTPUT_PP;
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
```
结果：PB10 为 0V，但此时 S 线被强拉低，舵机完全无法驱动总线。
RX count 始终为 0，回显和舵机应答都收不到。

### 最终方案
维持 `TX_Release` 为 INPUT 模式，在**软件层面**处理回显：
1. 发送命令
2. 等待 ~10ms（回显到齐）
3. **不清缓冲区**，直接调用 `USL_RecvPackage`
   - 帧头检测自动跳过非 `FF F5` 的回显字节
   - 舵机的 `FF F5` 应答到达后被正常解析
4. 最终收到完整应答：`FF F5 01 02 00 FC`（Ping）或 `FF F5 01 04 00 XX XX CS`（Read）

### 教训
- 回显字节不可怕，帧头检测能跳过它们
- 注意不要在清缓冲区时把舵机应答也清掉了
- 读操作的正确时序：清→发→等10ms→收（不清缓冲区）

---

## 问题4：噪声淹没舵机回复

### 现象
主循环中 Read 操作返回 err=4 或 err=5，[RAW RX] 显示大量噪声。
测试中（无角度指令时）Read正常。

### 原因
主循环中角度指令发完后等待1秒，期间S线积累大量噪声。
ClearServoRxBuf 清空后，立即发送Read指令，回显+噪声再次涌入，
舵机的 `FF F5` 回复被噪声淹没。

### 修改

**读取函数内部修改（添加清空+等待）：**
```c
// US_Ping (Ping命令)
JOHO_PackageBuild_Send(usart, servo_id, 2, CMDType_Ping, NULL);
SysTick_DelayMs(5);
// 不清空缓冲区，帧头检测自动跳过回显FF FF，只认FF F5
PackageTypeDef pkg;
statusCode = USL_RecvPackage(usart, &pkg);
```

```c
// USL_GETPositionVal, USL_GetServoStatus 等读函数
JOHO_PackageBuild_Send(usart, servo_id, 4, CMDType_Read, content);
SysTick_DelayMs(5);       // 等待回显到齐
ClearServoRxBuf_Safe(usart->recvBuf);  // 清掉回显+噪声
SysTick_DelayMs(20);      // 等待舵机回复
USL_RecvPackage(usart, &pkg);
```

**主循环中清空缓冲区：**
```c
// main.c - while(1)循环中
ClearServoRxBuf();                    // 清掉之前的噪声
USL_SetServoAngle(...);               // 发角度指令
SysTick_DelayMs(1000);                // 等舵机到达

ClearServoRxBuf();                    // 清掉角度指令的回显+噪声
err = USL_GetServoStatus(...);        // 读状态
```

### 教训
- 测试环境干净但主循环有大量积累噪声
- 读操作需要：发命令→等回显→清缓冲区→等回复→接收
- 写操作和读操作之间必须清缓冲区

---

## 问题5：AF_OD（开漏）不适合做舵机TX

### 尝试的方案（最终未采用）
```c
// usart.c - PB10配置为AF_OD
GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;  // 开漏输出
GPIO_InitStruct.Pull = GPIO_PULLUP;
```

### 失败原因
开漏输出只能拉低不能推高，靠上拉电阻恢复高电平。
上升沿太慢（RC时间常数），在115200波特率下信号完整性差。
舵机收不到完整指令，有时动有时不动。

### 结论
半双工TX必须用 **AF_PP推挽** 发送，发完后**切INPUT释放**。
不能用AF_OD一直开漏。

---

## 问题6：HDSEL半双工模式也不适合

### 尝试的方案
```c
USART3->CR3 |= USART_CR3_HDSEL;  // 开启硬件半双工
```

### 失败原因
- HDSEL要求TX引脚配置为AF_OD开漏（信号弱，问题同上）
- 空闲时引脚高阻，但外部上拉到4V，USART输入不稳定
- PB11被HDSEL内部断开，只能靠TX pin接收

### 结论
手动GPIO切换（发送时AF_PP，发完INPUT）比HDSEL更可控。

---

## 问题7：中断编译导致ELF文件损坏

### 现象
```bash
$ pyocd flash build/F407_JOHO.elf --target stm32f407zgtx
Error: Magic number does not match
```

### 原因
用户在 `make` 编译过程中按取消/中断，生成的 .elf 文件不完整。
下次 `make` 时 Makefile 认为文件已最新，不重新链接。

### 解决
```bash
make clean   # 删除所有编译中间文件
make -j4     # 强制全部重新编译
```

---

## 问题8：S线电压4V是否正常

### 现象
转接板S线对GND测量约4V（STM32工作电压3.3V）。

### 原因
转接板自带外部上拉到舵机供电电压（通过电平转换芯片），
导致S线空闲时被拉高到4V。

### 结论
**正常。** STM32F407的PB10/PB11是5V耐受引脚，4V在安全范围内。
内部上拉设为`NOPULL`避免和外部上拉打架。

---

## 最终代码状态总结

### usart.c - 核心收发函数
```c
// PB10: 发前AF_PP推挽 → 发完INPUT释放
// PB11: 一直AF_PP连接USART3接收器
// Usart_SendAll: TX_Enable → 发送 → TX_Release
```

### uart_servo_lite.c - 协议层
```c
// 帧头检测: 只接受0xF5FF，不接0xFFFF
// Ping: 发→等5ms→收（帧头跳回显）
// Read: 发→等5ms→清→等20ms→收
```

### uart_servo_lite.h - 配置
```c
#define JOHO_TIMEOUT_MS 500    // 超时从100ms提到500ms
// #define DEBUG_SERVO_RAW     // 调试输出原始接收字节
```

### main.c - 主循环
```c
// 每次发指令前 ClearServoRxBuf（RingBuffer_Reset）
// 角度指令后等1秒再读状态
// 添加了RX count调试输出
```

---

## 调试工具和方法

### 1. DEBUG_SERVO_RAW宏
启用后超时时打印收到的所有原始字节，判断是否有数据进来。
```c
#define DEBUG_SERVO_RAW  // uart_servo_lite.h中取消注释
```

### 2. RX count计数器
```c
volatile uint32_t usart3_rx_count;  // usart.c中定义
// 在HAL_UART_RxCpltCallback中++，在main.c中打印delta
```

### 3. 裸数据测试函数
`test_servo.c` 中的 `RunServoTest()`：
- 发0xFF → 看噪声
- 发Ping → 看回复
- 扭矩使能后再Ping → 看回复
- 发Read → 看数据

### 4. 万用表测S线电压
- 空闲：约3.3-5V（取决于外部上拉）
- 通信时：应有0V-高电平的波动
- 如果悬空或电压不稳 → 上拉问题

### 5. 排查舵机通讯问题的万用表使用法
- **测供电**：舵机VCC-GND（7.4~12V），转接板VCC-GND（3.3V或5V）
- **测S线静态电压**：空闲时应有3.3~5V（有上拉），0V说明总线被拉死
- **测PB10（TX）空闲电压**：`TX_Release()` 后应为INPUT高阻，外部上拉拉到3.3V正常
- **测PB11（RX）电压**：空闲时约3.3V（USART内部上拉）
- **通断测试**（断电）：确认PB10→转接板TX、PB11→转接板RX、转接板S→舵机S、所有GND共通

---

## 常见问题排查 Q&A

### Q1: Ping 收到 `FE E0 00` 而不是 `FF F5`，是什么？
**A:** 收到的不是舵机应答，是 TX 信号通过转接板泄漏到 RX 的回显。说明RX通路是通的，但转接板没有切换到接收模式。先确认 `TX_Release()` 正确执行且 PB10 未锁死在输出模式。

### Q2: 为什么单个舵机测试可以，但扫描找不到ID？
**A:** 最常见的原因是 Ping 和 Read 的响应被回显字节干扰。解决办法：发送后等 10ms（回显到齐），然后调用 `USL_RecvPackage`（帧头检测会自动跳过回显，只认 `FF F5`）。**不要清缓冲区**，否则舵机应答也被清掉。

### Q3: 所有Ping都失败，err=2（超时），但舵机能动（广播指令有效）？
**A:** 广播ID=254不问应答，所有舵机都执行。但单独Ping一个ID时，如果那个ID没有舵机（或舵机应答没到RX），就会超时。**如果所有舵机都是出厂默认ID=1**，则只有Ping ID=1会成功，其他ID都没舵机。

### Q4: 为什么舵机都能动，但只有ID=1回复？
**A:** 因为所有舵机出厂默认ID=1。广播指令（ID=254）发给所有舵机，不管ID是什么都执行。但 Ping ID=2 时总线上没有ID=2的舵机，自然无人应答。需要用 `DetectAndAssignIDs()` 函数逐个把ID=1的舵机挪到临时ID，统计数量，再分配为1,2,3...。

### Q5: TX_Release 设 INPUT 后 PB10 仍有 3.3V，正常吗？
**A:** 正常。转接板有外部上拉电阻，INPUT 高阻时外部上拉把 PB10 拉到 3.3V。这不是问题，帧头检测能跳过来自 TX 的回显。

### Q6: TX_Release 设 OUTPUT_PP 输出低电平会怎样？
**A:** PB10=0V 会通过转接板把 S 线也拉到 0V。舵机无法驱动被强拉的 S 线，完全收不到任何数据（RX count=0）。**不要用 OUTPUT_PP 低电平。**

### Q7: 如何用万用表排查舵机不回复的问题？
**A:** 
1. 测 S 线空闲电压（应为 3.3~5V，0V 说明被拉死）
2. 测 PB10 在发送时有无波动（示波器更好）
3. 测 PB11 空闲电压（应为 ~3.3V）
4. 通断档测 PB11→转接板RX→S 通路
5. 确认所有舵机已上电（测 VCC-GND）

### Q8: 接收到的数据有时 `FF F5 01 00 09`（5字节），有时完整8字节？
**A:** 5字节是被中途截断的（如 `DumpAllRxBytes` 读走前5字节后剩下3字节才到）。用 `USL_RecvPackage`（按字节读取+超时等待）就能收到完整帧。

### Q9: 舵机ID改不了怎么办？
**A:** 写入寄存器 0x05（ID寄存器）即可。格式：`FF FF <当前ID> 04 03 05 <新ID> <CS>`。
注意校验和计算：`~(当前ID + 04 + 03 + 05 + 新ID) & 0xFF`。
写入后无需重启，新ID立即可用。建议写完后用 Ping 验证。

---

# 步态控制器模块文档

## 概述

`APP/gait.h` + `APP/gait.c` 实现了基于三角函数的舵机步态协调控制。

核心思想：**用数学公式计算每一条腿在每一时刻的角度，然后自动发给对应的物理舵机。**

---

## 什么是"映射" (Mapping)？

### 问题由来

你有 **N 条腿**，每条腿有 **2 个舵机**（一个控制偏航 Yaw，一个控制俯仰 Pitch）。  

但舵机本身不认识"腿"，只认识自己的 **ID 号**（比如 ID=1、ID=2 ...）。

所以需要一个 **映射表** 来回答这个问题：

> "逻辑上的第 0 条腿的偏航舵机，到底是物理上 ID 为几的那个舵机？"

```
┌──────────────────────────────────────────────────┐
│                  逻辑层 (你关心的)                    │
│                                                    │
│  腿0  ─── 偏航(Yaw)                                │
│        ─── 俯仰(Pitch)                              │
│  腿1  ─── 偏航(Yaw)                                │
│        ─── 俯仰(Pitch)                              │
│  ...                                               │
│                          │                          │
│                     ▼ 映射 ▼                       │
│                          │                          │
│                  物理层 (舵机通讯)                    │
│                                                    │
│  舵机ID=1  ← 腿0 的偏航                              │
│  舵机ID=2  ← 腿0 的俯仰                              │
│  舵机ID=3  ← 腿1 的偏航                              │
│  舵机ID=4  ← 腿1 的俯仰                              │
│  ...                                               │
└──────────────────────────────────────────────────┘
```

### 在代码中的体现

**结构体 `LegServoMap`（`gait.h` 中定义）：**
```c
typedef struct {
    uint8_t  yaw_servo_id;      // 这条腿的偏航舵机，物理ID是多少？
    uint8_t  pitch_servo_id;    // 这条腿的俯仰舵机，物理ID是多少？
    float    yaw_offset_deg;    // 偏航安装偏移补偿（度）
    float    pitch_offset_deg;  // 俯仰安装偏移补偿（度）
} LegServoMap;
```

**配置映射的两种方式：**

**方式A — 手动映射（灵活）：**
```c
// 腿0: 偏航用ID=1，俯仰用ID=2，无安装偏移
Gait_SetMapping(&gc, 0, 1, 2, 0.0f, 0.0f);
// 腿1: 偏航用ID=3，俯仰用ID=4，偏航装偏了3°所以补偿
Gait_SetMapping(&gc, 1, 3, 4, 3.0f, 0.0f);
```

**方式B — 顺序映射（舵机ID连续时最简洁）：**
```c
// base_id=1, leg_count=4
// 自动: Leg0(Yaw=1,Pitch=2), Leg1(Yaw=3,Pitch=4) ...
Gait_SetMappingSequential(&gc, 1);
```

### 为什么需要安装偏移补偿？

舵机装在机器人腿上时，机械结构可能有安装误差。比如腿2的偏航舵机装歪了 5°，如果不补偿，腿2走出来的轨迹就会偏。

解决办法：在映射表中记下这个偏移，`Gait_Update` 发角度时会自动加上它。

```c
Gait_SetMapping(&gc, 2, 5, 6, 5.0f, 0.0f);
//                      ↑   ↑    ↑
//                偏航ID=5  俯仰ID=6  偏航装偏了5°，自动补偿
```

---

## 步态公式

整个步态控制基于两个余弦函数：

```
θ_yi = α_y · cos(ω_y · t + (i-1) · β_y) + γ_y
θ_pi = α_p · cos(ω_p · t + (i-1) · β_p + φ) + γ_p
```

**变量说明：**
| 符号 | 含义 | 单位 |
|------|------|------|
| θ_yi | 第 i 条腿的偏航角 | 弧度 |
| θ_pi | 第 i 条腿的俯仰角 | 弧度 |
| i | 腿编号（从1开始） | — |
| t | 时间 | 秒 |

**参数说明（通过 `GaitParams` 结构体设置）：**
| 参数 | 含义 | 作用 |
|------|------|------|
| α_y, α_p | 振幅 | 腿摆动幅度有多大 |
| ω_y, ω_p | 角频率 | 腿摆动有多快（2π=每秒转一圈） |
| β_y, β_p | 腿间相位差 | 相邻腿之间差多少相位（决定步态模式） |
| φ | 偏航-俯仰相位差 | 腿画圆还是画椭圆的关键 |
| γ_y, γ_p | 中心偏移 | 整体角度偏移（调重心） |

### 三角步态示例（6腿机器人）

假设 β = π，6条腿的相位分布：
```
腿0: cos(ωt + 0)       → 与腿3同相（差3π）
腿1: cos(ωt + π)       → 与腿0反相
腿2: cos(ωt + 2π) = cos(ωt) → 与腿0同相
腿3: cos(ωt + 3π) = cos(ωt+π) → 与腿0反相
腿4: cos(ωt + 4π) = cos(ωt) → 与腿0同相
腿5: cos(ωt + 5π) = cos(ωt+π) → 与腿0反相
```
结果：腿0/2/4 一组，腿1/3/5 一组，交替抬起 → **三角步态**

---

## 预设步态

代码中内置了三种预设步态，可直接使用：

| 步态名 | 宏 | 说明 | 适用 |
|--------|-----|------|------|
| 螺旋翻滚 | `GAIT_SPIRAL` | 腿画圆轨迹，做翻滚动作 | 单腿/双腿测试 |
| 三角步态 | `GAIT_TRIPOD` | 6腿分成两组交替摆动 | 6腿快速行走 |
| 波浪步态 | `GAIT_WAVE` | 6腿依次抬起，像波浪 | 6腿慢速稳定行走 |

切换步态只需一行：
```c
Gait_SetParams(&gc, &GAIT_TRIPOD);  // 换三角步态
```

你也可以定义自己的步态：
```c
GaitParams myGait = {
    .alpha_y = 0.5f,       // ≈28.6°
    .alpha_p = 0.4f,       // ≈22.9°
    .omega_y = 4.0f,       // 更快
    .omega_p = 4.0f,
    .beta_y  = 1.57f,      // π/2
    .beta_p  = 1.57f,
    .phi     = 1.57f,      // π/2
    .gamma_y = 0.0f,
    .gamma_p = -0.17f,     // 俯仰偏-10°，重心前倾
};
Gait_SetParams(&gc, &myGait);
```


## Gait_Update 内部工作流程

```
Gait_Update(&gc, HAL_GetTick())
│
├─ 计算当前时间 t = (now - start_tick) / 1000
│
├─ 对每条腿 i:
│   ├─ 查映射表 → 得到 yaw_servo_id, pitch_servo_id, offset
│   │
│   ├─ 计算 θ_yi = α_y·cos(ω_y·t + (i-1)·β_y) + γ_y
│   ├─ 计算 θ_pi = α_p·cos(ω_p·t + (i-1)·β_p + φ) + γ_p
│   │
│   ├─ 加上安装偏移: θ_y += yaw_offset
│   │                   θ_p += pitch_offset
│   │
│   ├─ 弧度 → 度 → 舵机原始值(0~4095)
│   │
│   └─ 发送: USL_SetServoAngle(servoUsart, yaw_servo_id, raw_y, 100)
│            USL_SetServoAngle(servoUsart, pitch_servo_id, raw_p, 100)
│
└─ 更新 last_update = current_tick
```

---



# 代码改动历程与工程复盘（2026-07-28）

> 本节记录本轮针对蛇形机器人的实际改动。若与前面的早期调试记录冲突，
> 以本节和当前源码为准。项目对象已经从早期的“腿式映射示例”明确为：
> **4 个正交双轴关节、8 只总线舵机组成的蛇形机器人**。

## 一、最终目标和当前状态

当前硬件结构：

```text
Joint0: 垂直轴 V=ID1，水平轴 H=ID2
Joint1: 垂直轴 V=ID3，水平轴 H=ID4
Joint2: 垂直轴 V=ID5，水平轴 H=ID6
Joint3: 垂直轴 V=ID7，水平轴 H=ID8
```

当前程序状态：

```c
#define SERVO_ID_SETUP_MODE 0
```

即正常运行步态，不再执行舵机 ID 写入。当前启用的是螺旋翻滚：

```c
#define SPIRAL_HORIZONTAL_AMPLITUDE_DEG 25.0f
#define SPIRAL_VERTICAL_AMPLITUDE_DEG   25.0f
#define SPIRAL_PERIOD_S                  2.0f
#define SPIRAL_JOINT_PHASE_DEG \
        (360.0f / SNAKE_JOINT_COUNT)
#define SPIRAL_AXIS_PHASE_DEG           90.0f
```

启动后振幅在 3 秒内从 0 平滑增加到目标值，避免舵机突然大幅转动。

---

## 二、改动时间线

### 阶段1：修复“舵机能动，但状态读不出来”

#### 现象

- 控制指令能够让部分舵机运动；
- Ping 或寄存器读取经常得到 `[RAW RX] 0 bytes`；
- 有时把发送回显当成舵机回复；
- 回复稍晚到达时，程序又把它清掉了。

#### 根因

这不是单一问题，而是三个问题叠加：

1. JOHO 总线可能在 RX 端看到本机发送回显；
2. 请求帧头是 `FF FF`，真实响应帧头是 `FF F5`，两者不能混用；
3. 原接收流程在舵机已经开始回复后清空缓冲区，导致有效数据丢失。

#### 修改

主要修改 `HARDWARE/uart_servo/uart_servo_lite.c/.h`：

- 接收解析器只接受真实响应帧头；
- 扫描接收流并自动跳过发送回显和前导噪声；
- 对长度、ID、舵机状态字和校验和分别验证；
- 在发送新事务之前清除旧数据，禁止在回复到达后再清缓冲；
- 新增通用寄存器读取接口 `USL_ReadRegisters()`；
- `US_Ping()` 检查返回 ID、包长度和舵机状态；
- 环形缓冲区读写索引改为 `volatile`，避免中断和主循环共享状态被错误优化。

状态读取使用的寄存器：

```text
当前位置：0x38，2 字节
当前电流：0x2E，2 字节
当前电压：0x3E，1 字节
当前温度：0x3F，1 字节
```

#### 教训

- “舵机能执行写命令”不等于“主控能收到舵机回复”；
- 广播写不要求回复，因此不能用广播动作成功证明 RX 通道正常；
- 串口协议调试必须保留原始字节、响应帧头、长度和校验和四类证据；
- 清接收缓冲区的时机属于协议事务的一部分，不能随意放置延时和清空操作。

---

### 阶段2：确认舵机映射不是舵机 ID 配置

#### 现象

最开始只有 ID1 和 ID5 对应的机构上下转，其他舵机没有按预期形成平面波。
状态日志后来显示 ID1、ID2、ID5、ID6 能回复，而 ID3、ID4、ID7、ID8 离线。

#### 根因

这里曾混淆两个完全不同的概念：

```text
舵机映射：告诉步态程序“某个逻辑轴使用哪个物理 ID”
舵机改 ID：真正修改舵机 EEPROM 中的总线地址
```

修改映射表不会自动把实际舵机改成 ID1～ID8。如果多只舵机仍使用相同出厂
ID，Ping 会冲突，单播控制也无法区分它们。

另外，根据实机动作观察，奇偶 ID 的轴定义需要交换：

```text
奇数 ID：垂直轴 V
偶数 ID：水平轴 H
```

因此当前映射为：

```c
uint8_t vertical_id   = GAIT_BASE_ID + joint * 2;
uint8_t horizontal_id = vertical_id + 1;
```

#### 教训

- 软件映射只能引用地址，不能创造地址；
- 必须先逐只给舵机分配唯一 ID，再进行多舵机映射；
- H/V、Yaw/Pitch 的命名必须以实机轴向验证为准，不能只根据装配图猜测；
- 映射表应同时保存 ID、安装方向 `+1/-1` 和零位偏置。

---

### 阶段3：建立可靠的 8 舵机启动检测

正常步态启动前，程序现在会：

1. 打印每个关节的 H/V 到物理 ID 的映射；
2. 依次 Ping ID1～ID8；
3. 在线舵机才使能扭矩；
4. 离线舵机从本次同步写列表移除；
5. 后续每 250 ms 轮询一只在线舵机的状态。

典型正确输出应包含：

```text
[Map] Joint0 H=ID2 ... V=ID1 ...
[Map] Joint1 H=ID4 ... V=ID3 ...
[Map] Joint2 H=ID6 ... V=ID5 ...
[Map] Joint3 H=ID8 ... V=ID7 ...

[Bus] Joint0 H ID=2 ONLINE (status=0)
...
[Bus] Joint3 V ID=7 ONLINE (status=0)
```

如果某个 ID 显示：

```text
OFFLINE (status=2)
```

表示等待回复超时，优先检查：

- 该舵机是否真的已经写入这个 ID；
- 是否存在重复 ID；
- 总线接线和公共地；
- 舵机供电是否在动作瞬间下降；
- 回复是否被噪声或半双工方向切换破坏。

#### 教训

不要在不知道在线节点的情况下直接把全部 ID 加入运动控制。先发现、再使能、
最后运动，可以把“通讯故障”和“步态公式故障”分开。

---

### 阶段4：同步写解决多舵机更新时间不一致

逐只发送角度会造成蛇头和蛇尾接收目标的时间不同，舵机越多越明显。
本轮将同步写容量提高到 16 只舵机，并扩大协议内容缓冲区，使 8 个双轴关节
也可以在一个控制周期中更新。

当前 4 个关节共 8 只舵机，通过：

```c
Gait_UpdateSync(&gc, HAL_GetTick());
```

一次收集并发送全部在线舵机的位置目标。主循环控制周期约为 20 ms。

#### 教训


- 行波的相位精度不只由数学公式决定，也受指令实际到达时间影响；
- 多节点周期运动应优先使用同步写；
- 协议最大包长、同步写节点数和本地数组长度必须一起调整，不能只改其中一个。

---

### 阶段5：实现平面蜿蜒

平面蜿蜒阶段采用：

```text
水平轴：θ_Hi = αH·cos(ωt + iβ)
垂直轴：θ_Vi = 0
```

加入了：

- 可调振幅、周期、相邻关节相位差；
- 显式 H/V 映射；
- 同步写；
- 启动振幅缓升；
- 在线节点筛选和状态轮询。

这个阶段验证了 ID 映射和水平轴识别是否正确，也是进入三维步态前必须完成的
基础测试。

#### 教训

复杂的三维步态不要一步到位。先让一个自由度保持中位，只测试另一个自由度的
空间波；确认轴向、相位传播和 ID 顺序后，再启用第二个自由度。

---

### 阶段6：单舵机 ID 配置方案的两次迭代

#### 第一版：USART1 终端输入

最初增加了串口交互模式：

```text
Enter target ID, then press ENTER:
>
```

输入目标 ID 后广播写寄存器 `0x05`，等待 EEPROM 保存，再 Ping 新 ID 三次。

这版暴露了两个工程问题：

1. 不带换行的 `printf` 提示可能被 newlib 行缓冲暂存，需要换行或
   `fflush(stdout)`；
2. 电脑能看到 USART1 输出，只能证明 `PA9 -> USB-TTL RX` 正常，不能证明
   `USB-TTL TX -> PA10` 正常。终端本地回显还可能让人误以为 STM32 收到了输入。

USART1 正确接线：

```text
STM32 PA9  (TX) -> USB-TTL RX
STM32 PA10 (RX) <- USB-TTL TX
STM32 GND       -- USB-TTL GND
```

#### 第二版：宏定义自动写入

为了减少串口输入链路对 ID 配置的影响，最终改为编译期宏：

```c
#define SERVO_ID_SETUP_MODE 1
#define TARGET_SERVO_ID     3
```

程序上电后自动：

1. 使用广播 ID `0xFE` 写 ID 寄存器 `0x05`；
2. 等待 1 秒保存 EEPROM；
3. Ping 新 ID，最多三次；
4. 打印成功或失败；
5. 停止运行，避免循环擦写 EEPROM。

配置下一只舵机时，断电、只连接下一只舵机、修改 `TARGET_SERVO_ID`、重新编译
烧录。全部配置完成后必须恢复：

```c
#define SERVO_ID_SETUP_MODE 0
```

#### 最重要的安全规则

广播改 ID 时，总线上只能连接一只舵机。若连接多只，所有收到广播写命令的
舵机都会变成同一个 ID，随后产生地址冲突。

#### 教训

- 配置 EEPROM 的程序必须是“一次性动作”，不能放进无限循环反复写；
- 配置模式和步态模式必须有明确开关；
- 写入成功后必须用新 ID 单播 Ping 验证，不能只打印“发送完成”；
- 更换舵机前应断电，避免带电插拔和意外总线冲突。

---

### 阶段7：实现螺旋翻滚

新增接口：

```c
Gait_ConfigureSpiralRolling(
    &gc,
    horizontal_amplitude_deg,
    vertical_amplitude_deg,
    period_s,
    joint_phase_deg,
    axis_phase_deg);
```

数学模型：

```text
θ_Hi = αH·cos(ωt + iβ)
θ_Vi = αV·cos(ωt + iβ + φ)
```

当前参数：

```text
αH = 25°
αV = 25°
T  = 2 s，因此 ω = 2π/T
β  = 360°/4 = 90°
φ  = 90°
```

水平和垂直两个正交自由度同频且相差 90°，使单个关节的 H/V 角度组合形成
圆周相位；相邻关节再依次错开 90°，从而沿蛇身形成螺旋波。

改变翻滚方向：

```c
#define SPIRAL_AXIS_PHASE_DEG -90.0f
```

改变螺旋波沿蛇身的传播方向：

```c
#define SPIRAL_JOINT_PHASE_DEG \
        (-360.0f / SNAKE_JOINT_COUNT)
```

初次实机测试建议：

- 先架空或使用支撑架；
- 振幅从 15°～25°开始；
- 周期从 2～3 秒开始；
- 检查每对 H/V 是否真正正交；
- 单个舵机方向错误时，修改对应映射的 `direction`，不要用全局相位掩盖装配反向；
- 确认 ID1～ID8 全部在线后再落地测试。

#### 教训

- `φ=90°` 决定单关节的圆周旋转关系；
- `β` 决定形状沿蛇身的空间传播；
- `ω` 决定时间速度；
- `α` 决定弯曲幅度；
- “波形看起来正确”不保证机器人一定能有效翻滚，地面摩擦、重心、关节间距、
  舵机速度和机械限位都需要实机调参。

---

## 三、本轮主要文件改动

| 文件 | 主要改动 |
|------|----------|
| `Core/Src/main.c` | ID 配置/步态模式开关、4 关节映射、启动 Ping、在线筛选、状态轮询、螺旋翻滚参数 |
| `APP/gait.h` | 蛇形关节映射定义、最大关节数、平面蜿蜒和螺旋翻滚接口 |
| `APP/gait.c` | 蛇形波公式、方向/偏置、振幅缓升、同步更新、螺旋翻滚参数生成 |
| `HARDWARE/uart_servo/uart_servo_lite.h` | 状态码、协议限制、寄存器地址、同步写容量 |
| `HARDWARE/uart_servo/uart_servo_lite.c` | 稳健接收解析、Ping、通用寄存器读取、状态读取、同步写 |
| `Core/Src/usart.c` | USART3 半双工发送/释放、RX 中断环形缓冲、USART1 调试输出 |
| `HARDWARE/ring_buffer/ring_buffer.h` | 中断共享索引的 `volatile` 修正 |

---

## 四、推荐的调试顺序

以后增加舵机、修改机构或更换控制板时，按下面顺序调试：

1. **断开运动负载**：先架空机构，确认不会碰撞；
2. **单舵机通讯**：只接一只，Ping 并读取位置；
3. **单舵机改 ID**：广播写入后，用新 ID Ping；
4. **逐只配置唯一 ID**：记录标签，不依赖记忆；
5. **整总线只 Ping**：不运动，确认每个 ID 恰好回复一次；
6. **逐轴小角度测试**：分别验证 H 和 V、方向和零位；
7. **平面单自由度行波**：验证关节顺序和空间相位；
8. **启用第二自由度**：先低振幅、慢周期；
9. **同步写三维步态**：确认全部在线后再运行；
10. **最后落地调参**：逐步增加振幅和速度，并监控电源、温度、电流。

这个顺序的核心是每一步只增加一个变量，出现问题时才能快速定位。

---

## 五、仍需实机确认的问题

### 1. 电压显示倍率

日志中出现过 `voltage=55V`、`58V`。若实际供电约为 5.5V～5.8V，则寄存器
可能以 0.1V 为单位，当前日志标签需要改成小数显示。必须用万用表和具体型号
手册确认，不能仅凭数值猜测。

### 2. 电流数值

曾出现 `8704mA`、按 256 跳变或 `-1mA`。这可能来自：

- 寄存器地址或长度与具体舵机型号不完全一致；
- 大端字节序解释错误；
- 单位缩放尚未处理；
- 堵转或协议错误值。

在电流值未与钳形表/电流表对照前，不应把日志数值直接用于过流保护。

### 3. 电源与复位

启动时乱码通常是复位瞬间或串口残留字节；完整启动横幅重复出现则意味着程序
重新启动。若它持续重复：

- 先断开舵机，仅给主控供电；
- 检查 NRST；
- 舵机使用足够电流能力的独立电源；
- 主控和舵机必须共地；
- 检查舵机动作瞬间电压跌落。

### 4. 总线噪声和状态轮询

状态读取会占用半双工总线时间。如果螺旋波出现周期性顿挫：

- 暂时关闭状态轮询，判断是否为读取事务造成；
- 增大 `STATUS_POLL_INTERVAL_MS`；
- 检查同步写回显和读回复是否在环形缓冲中交叉；
- 使用逻辑分析仪同时观察 TX、RX 和舵机 S 线。

---

## 六、构建与交付

编译命令：

```bash
make -j4
```

本轮修改已通过 ARM GCC 完整编译和链接，生成：

```text
build/F407_JOHO.elf
build/F407_JOHO.hex
build/F407_JOHO.bin
```

编译仍会报告少量旧调试辅助函数未使用的 warning，例如
`USART3_DumpStatus`、`ReadReg`，不影响当前固件生成，但后续应清理，避免真正
重要的 warning 被噪声淹没。

---

## 七、最值得保留的工程经验

1. **动作成功不代表通信闭环成功**：广播写能动，RX 仍可能完全不通。
2. **映射不等于分配 ID**：先配置物理地址，再谈逻辑关节映射。
3. **调协议先看原始字节**：不要只相信上层函数打印的 `SUCCESS`。
4. **请求和响应必须严格区分**：不能把本机回显当成从机应答。
5. **清缓冲区要以事务为边界**：错误时机会删除真正回复。
6. **广播配置必须单节点操作**：否则会制造重复 ID。
7. **EEPROM 写入只执行一次**：禁止放在高频循环中。
8. **先单轴再双轴**：平面波验证通过后再做三维螺旋。
9. **多舵机步态使用同步写**：保证空间相位在时间上也尽量同步。
10. **软件参数必须经过实机校准**：方向、零位、摩擦和电源能力都无法只靠公式确定。
