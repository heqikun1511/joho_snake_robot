# 灵蛇智采网页原型

这是一个不依赖框架和后端的静态项目工作台，用于查看项目进展、成员信息并进入自建 GitLab。

网页不再提供上位机追踪、实时遥测或采摘监视等无法远程连接的演示页面，也不显示模拟设备在线状态。

## 本地预览

在仓库根目录执行：

```bash
python3 -m http.server 8080 --directory web-demo
```

然后访问 <http://localhost:8080>。首次使用须填写真实姓名、设置至少 6 位密码并选择项目职责进行注册；注册成功后会直接进入工作台。之后可使用已注册的姓名和密码登录。

职责包括控制与路径规划、SLAM、系统架构设计、指导老师、负责人，也可以选择“其他”并自行填写。

## GitLab 开发入口

侧边栏的“GitLab 开发”会直接跳转到本地部署的 GitLab：<http://gitlab.xmutros2snake.com>。

如 GitLab 域名变更，修改 `app.js` 顶部的 `GITLAB_URL`。

> 说明：这是静态网站，注册账户及密码摘要仅保存在当前浏览器的 `localStorage` 中，不能用于多人共享的正式认证。正式上线应接入后端账号系统或 GitLab OAuth / OpenID Connect。
