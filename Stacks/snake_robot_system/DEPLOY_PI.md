# Raspberry Pi 镜像部署

要求：树莓派运行 64 位 Raspberry Pi OS/Ubuntu，`uname -m` 输出 `aarch64`，并已安装 Docker Engine 与 Docker Compose 插件。

## 1. 在开发机登录并推送 ARM64 镜像

以 Docker Hub 用户名 `myname` 为例：

```bash
docker login docker.io
chmod +x script/build-push-arm64.sh script/pi-pull-run.sh
./script/build-push-arm64.sh docker.io/myname jazzy
```

脚本先推送公共镜像 `snake-ros-base:jazzy`，再并行构建并推送三个业务镜像。后续修改源码时会复用仓库中的 BuildKit 缓存。

Docker Hub 中需要允许推送以下仓库：

- `snake-ros-base`
- `snake-autonomy`
- `snake-control`
- `snake-monitoring`
- `snake-build-cache`

如果这些仓库设为 Public，Pi 拉取时不需要登录；设为 Private 时，需要先在 Pi 上执行 `docker login docker.io`。

## 2. 把仓库部署文件复制到 Pi

Pi 至少需要以下文件：

```text
script/pi-pull-run.sh
Stacks/snake_robot_system/compose.pi.yaml
Stacks/snake_robot_system/.env
```

首次部署先创建 `.env`，再确认其中的镜像仓库和串口配置：

```bash
cp Stacks/snake_robot_system/pi.env.example Stacks/snake_robot_system/.env
```

```dotenv
IMAGE_REGISTRY=docker.io/myname
IMAGE_TAG=jazzy
ROS_DOMAIN_ID=0
SERIAL_DEVICE=/dev/ttyUSB0
DIALOUT_GID=20
```

串口组 ID 可在 Pi 上查询：

```bash
getent group dialout
```

## 3. 在 Pi 上拉取并启动

```bash
chmod +x script/pi-pull-run.sh
./script/pi-pull-run.sh docker.io/myname jazzy
```

检查架构和运行状态：

```bash
uname -m
docker image inspect docker.io/myname/snake-control:jazzy --format '{{.Architecture}}'
docker compose -f Stacks/snake_robot_system/compose.pi.yaml ps
docker logs -f snake-monitoring
```

以后发布新版时，只需在开发机重新运行构建推送脚本，再在 Pi 上重新运行拉取启动脚本。
