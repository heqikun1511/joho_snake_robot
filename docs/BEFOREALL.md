# 开发前必读

本项目使用一个原生 ROS 2 Jazzy/colcon 工作区，不使用 Docker 或 Compose。

## 目录边界

| 目录 | 用途 |
| --- | --- |
| `src/` | ROS 2 软件包仓库；唯一的 colcon 源目录 |
| `firmware/` | STM32F407 固件仓库 |
| `simulation/` | 仿真资源和工具仓库 |
| `repos/` | vcstool 依赖清单 |
| `scripts/` | 导入和构建脚本 |
| `build/`、`install/`、`log/` | colcon 自动生成，不提交 |

构建脚本只扫描 `src/`，因此固件和仿真资源不会被 colcon 误识别为 CMake 包。不要创建第二个 ROS 工作区，不要复制已有仓库到多个目录，也不要把 Dockerfile 或 Compose 重新放回本仓库。

## 初始化环境

安装 ROS 2 Jazzy 后，安装工作区工具：

```bash
sudo apt update
sudo apt install python3-vcstool python3-colcon-common-extensions
```

从仓库根目录导入缺失依赖：

```bash
./scripts/import_repos.sh
```

该命令使用 `repos/joho_snake_robot.repos`，只克隆尚不存在的仓库，绝不重置已有仓库的分支或本地改动。

## 构建

```bash
./scripts/build.sh
source install/setup.bash
```

只构建指定包时：

```bash
./scripts/build.sh --packages-select <package_name>
```

## 协作规则

- 在具体的包仓库中创建分支、提交与推送；集成仓库只维护清单、脚本和共享文档。
- 新增外部仓库时，先在 `repos/joho_snake_robot.repos` 添加条目，再执行导入脚本。
- ROS 包只允许落在 `src/`；固件只允许落在 `firmware/`；仿真资源只允许落在 `simulation/`。
- `build/`、`install/`、`log/` 发生异常时可以删除后重新构建。

## 固件

STM32F407 固件位于 `firmware/stm32f407_snake/`，可按其仓库中的说明和 Makefile 进行独立构建。固件不是 colcon 包，不应放入 `src/`。
