# 蛇形机器人控制台 (snake-control-panel)

一个用于 **Foxglove Studio** 的自定义扩展面板，用于蛇形机器人的控制与状态监视。

## 功能

- 机器人控制台面板：连接/断开机器人，发送控制指令
- 状态监视：显示蛇形机器人各关节的实时状态

## 安装

在扩展目录下执行：

```bash
npm install       # 安装依赖
npm run build     # 构建扩展
npm run local-install  # 本地安装到 Foxglove Studio
```

## 使用

1. 打开 Foxglove Studio
2. 在面板列表中找到 **"机器人控制台"** 面板
3. 拖拽到布局中即可使用

## 开发

```bash
npm run build    # 构建
npm run package  # 打包为 .foxe 扩展文件
```

## 许可

MIT
