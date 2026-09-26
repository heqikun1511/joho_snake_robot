# 灵蛇智采机器人

面向 ROS 2 Jazzy 的原生（非 Docker）开发工作区。项目采用一个 colcon 工作空间：构建脚本只扫描 `src/` 中的 ROS 包，构建产物统一生成在根目录的 `build/`、`install/`、`log/`。

## 目录结构

```text
.
├── src/            # 通过 repos/joho_snake_robot.repos 导入的 ROS 2 仓库
├── firmware/       # STM32F407 固件仓库
├── simulation/     # 仿真资源与工具仓库
├── repos/          # 唯一的 vcstool 清单
├── scripts/        # 原生开发辅助脚本
├── docs/           # 架构、部署与开发文档
└── web-demo/       # 项目静态网站
```

`src/`、`firmware/`、`simulation/` 下的仓库均有各自的 Git 历史；本集成仓库只跟踪清单、脚本和共享文档。Dockerfile、Compose 和容器启动脚本不再是本项目架构的一部分。

## 初始化

```bash
sudo apt install python3-vcstool python3-colcon-common-extensions
./scripts/import_repos.sh
```

该命令只克隆缺失仓库，不会覆盖已有仓库的分支或本地修改。

## 构建与运行

本机安装 ROS 2 Jazzy 后：

```bash
./scripts/build.sh
source install/setup.bash
```

构建产物可安全删除后重新生成：

```bash
rm -rf build install log
```

## 更新仓库清单

新增或移除外部仓库时，只修改 `repos/joho_snake_robot.repos`。不要将外部仓库复制到多个工作空间，也不要修改 `.repos` 文件以外的克隆路径。
