# 蛇形机器人 Foxglove 控制台

当前目录已恢复历史版本中的蛇形机器人上位机界面，并迁移到新的 ROS monitoring 目录结构。

## 界面功能

- 机器人连接、标识码和 LED 控制
- 通用、木偶式、导向式和目标式四种控制模式
- 八关节仿生脊柱姿态与角度调节
- ROS 2 命令发送和实时遥测数据显示
- RGB、深度图和三维重建三路图像窗口

## 构建与安装

```bash
npm install
npm run build
npm run local-install
```

在 Foxglove 的面板列表中添加“蛇形机器人”即可。

## Purpose
- Preserve Foxglove custom UI logic and plugins.
- Keep extension development close to the ROS monitoring package.
- Make the visualization layer a first-class part of the ROS architecture.

## Suggested source layout
- `src/` - TypeScript/React source for the custom panel
- `package.json` - extension package metadata
- `tsconfig.json` - TypeScript configuration
- `dist/` - built output for runtime usage

## Migration principle
The Foxglove extension should not live as a standalone repository island. Instead, it sits under the ROS monitoring package and is consumed by the monitoring stack from here.
