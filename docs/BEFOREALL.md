# 开发前必读（Before All）

这份文档面向第一次参与项目的成员。请在新建分支、修改接口或连接真机之前完整阅读。它的目的不是限制每个人的实现方式，而是统一基本环境、协作流程和安全边界，减少多人并行开发时的重复劳动与集成冲突。

> 当前项目仍处于架构搭建阶段。STM32 舵机控制已有实际代码，多数 ROS 2 包仍是规划或占位内容。README 中描述的“目标架构”不等于已经实现的功能。

## 1. 先了解项目边界

系统分为四个主要部分：

- `firmware/stm32f407_snake/`：STM32F407 固件，负责实时舵机控制和底线安全保护；
- `ROS-Packages/`：ROS 2 接口、硬件接入、步态、安全、导航、仿真和监控包；
- `Stacks/`：control、autonomy、monitoring 等容器镜像及整机编排；
- `simulation/`、`local_orchestration/`：仿真和本地联调入口。

### 1.1 目前已完成的模块

目前已经具备实际代码并可独立构建的是 **STM32F407 舵机控制模块**，位于 `firmware/stm32f407_snake/`。

该模块当前已实现：

- JOHO 总线舵机通信、位置读写、Ping、ID 扫描与修改；
- USART3 半双工 TTL 通信及收发缓冲；
- 多舵机 SyncWrite 同步控制；
- 舵机 ID、安装方向和零位偏置映射；
- 平面蜿蜒、侧向蜿蜒等参数化步态；
- GCC 与 Makefile 命令行构建。

可使用以下命令验证固件能否编译：

```bash
cd firmware/stm32f407_snake
make clean
make
```

这里的“已完成”表示该模块已有可运行的主体实现，不表示整机控制链路已经完成。ROS 2 硬件接入、命令门控、安全控制、仿真和整机联调仍需继续实现和验证。

## 2. 开发前置基础

### 2.1 必备能力

开始开发前，至少应能独立完成以下操作：

- 使用 Linux 终端浏览、复制、移动和查找文件，查看进程、端口与串口设备；
- 使用 Git 完成 clone、branch、switch、status、add、commit、fetch、 或 merge；
- 理解 ROS 2 的 node、topic、service、action、parameter、launch ；
- 使用 Docker 构建镜像、启动容器、查看日志并进入容器排障；


### 2.2 推荐软件环境

- Git；
- Docker Engine 或 Docker Desktop（支持 `linux/amd64` 或 `linux/arm64`）；
- ROS 2 Jazzy（推荐通过项目 Docker 镜像使用）；
- 固件开发需要 `make` 和 `arm-none-eabi-gcc`；
- 真机调试需要 ST-Link/烧录工具以及串口访问权限。

首次克隆后先做基础检查：

```bash
git status
git branch --show-current
docker --version
```

ROS 2 开发者可构建公共基础镜像：

```bash
./script/build-base.sh
```



如果只负责某一模块，不必先启动整套系统；优先完成该模块的最小构建或 mock 验证。

## 3. 分支与协作约定

仓库可能同时存在多个长期或历史分支，但开发时不要按“每人永久占用一个分支”的方式协作。建议以集成分支为起点，每个任务创建一个短生命周期分支：

```bash
git fetch origin
git switch ros2
git switch -c feature/<简短任务名>
```

分支名建议使用：

- `feature/...`：新增功能；


注意：示例中的 `ros2` 是当前 ROS 2 集成分支。实际开发前应确认团队当期的目标分支，固件任务也可能以固件分支为基线。

### 3.1 当前各分支内容

以下内容根据仓库中现有的本地及 `origin` 分支记录

| 分支 | 当前主要内容 | 状态说明 |
| --- | --- | --- |
| `ros2` | 当前项目目录架构、ROS 2 包规划、control/autonomy/monitoring 容器、Docker 构建脚本及 STM32 基础工程 | 当前默认分支和 ROS 2 集成基线 |
| `feature/robot-description_control` | `snake_description` 的 URDF/Xacro 模型、模型查看 launch，以及基于 mock hardware 的 ros2_control bringup 和控制器配置 | 已完成初步模型与 mock control 验证，尚需补充真实舵机参数和真机接入 |
| `firmwork` | STM32 SPI3 从机通信链路，包括 `spi_slave_link`、主循环接入、Makefile 和固件说明 | 已完成树莓派与单片机 SPI3 初步通信搭建，仍需纳入上层完整协议和整机验证 |
| `feature/robot-description` | 早期 `snake_description`、关节标定配置、URDF/Xacro、RViz 配置和显示 launch | 已完成早期 RViz2 可视化搭建；目录结构较旧，后续成果已在其他分支持续演进 |
| `feature/robot-bringup` | 早期 bringup、Docker 启停脚本以及 Foxglove 蛇形机器人控制面板 | 上位机界面已有实现，但提交说明明确标注尚未对接 ROS 2 节点；目录结构较旧 |
| `feature/robot-firmware` | STM32 上位机通信协议文档与头文件，以及早期 ROS 2 工作区、模型和接口骨架 | 以协议设计和工程骨架为主，多个源文件仍为空，不应视为可运行的完整硬件接入模块 |
| `main` | 早期 Dockerfile、Compose 和 ROS 入口脚本 | 旧版容器方案，目前不是默认集成分支 |

本地目前检出的分支有 `ros2`、`firmwork` 和 `feature/robot-description_control`；其余分支仅存在于 `origin` 远程跟踪记录中。开始任务前先运行 `git fetch origin --prune` 获取最新状态，再用下面的命令确认分支是否已经合并：

```bash
git branch -a
git log --graph --oneline --decorate --all
git branch --merged ros2
```

不要直接删除看似“旧”的分支。应先确认其中是否仍有未合并提交，并由该模块负责人决定合并、保留或删除。

提交前必须查看修改范围：

```bash
git status
git diff
git diff --staged
```

一次提交只解决一个明确问题。提交信息应能说明修改意图，例如：

```text
feat(gait): add joint limit validation
fix(mcu): stop motion after command timeout
docs(bringup): document simulation startup
```

不要提交构建产物、日志、编辑器缓存、密钥、设备专属配置或与当前任务无关的文件。不要为了消除冲突而覆盖其他人的工作。

## 4. 接口先行

跨模块修改应先确认接口，再分别实现。接口提案至少写清：

- 名称和责任方；
- 字段、数据类型和物理单位；
- 坐标系、正方向和零点定义；
- 发布/调用频率与允许延迟；
- 超时、断线、非法输入和急停时的行为；
- 版本兼容方式与一组示例数据。


对公共消息、串口协议、URDF 关节名或 launch 参数的修改，会影响多个模块，合并前必须让上下游负责人共同评审。

## 5. 有关PR
PR 至少应包含：

```text
目的：为什么要改
修改：改了哪些模块或接口
验证：执行了哪些命令，结果如何
```

“在我的电脑上能运行”不是完成标准。构建命令、配置和测试输入应当可被其他成员复现。

## 6. 真机安全规则

暂无


## 7. 开始第一个任务前的清单

- [ ] 已阅读根目录 README 和自己负责模块的 README；
- [ ] 已确认当前集成分支和自己的任务分支；
- [ ] 已确认上下游负责人、接口、单位和验收条件；

如果以上任一项无法确认，先在任务或 PR 中提出问题。越早明确接口和安全边界，后续集成成本越低。


# 最后的最后
对于这个架构有任何问题可以找贺琦坤，或者如果你有更好的框架我们可以一起研究一起进步！！！
理想情况下推荐大家可以创建一个github群组，方便老师与组长进行管理
