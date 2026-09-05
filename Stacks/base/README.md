# ROS 2 基础镜像

基础镜像会根据当前电脑自动选择原生架构，支持 Linux `amd64`（常见 PC）和
`arm64`（64 位树莓派、ARM 开发机）。每台开发电脑在仓库根目录执行：

```bash
./script/build-base.sh
```

默认生成同名的本地镜像 `snake-ros-base:jazzy`。因此业务 Dockerfile 在不同
架构的电脑上无需修改；Docker 会拉取与本机架构匹配的
`ros:jazzy-ros-base-noble`。

如需修改本地标签或显式指定目标平台：

```bash
IMAGE_NAME=snake-ros-base:dev ./script/build-base.sh
TARGET_PLATFORM=linux/arm64 ./script/build-base.sh
TARGET_PLATFORM=linux/amd64 ./script/build-base.sh
```

显式构建非宿主机架构时，Docker 必须已配置相应的跨架构模拟支持。日常多电脑
并行开发建议省略 `TARGET_PLATFORM`，让每台电脑构建自己的原生镜像。
