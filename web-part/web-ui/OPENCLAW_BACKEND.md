# Smart Robot OpenClaw Backend

这套网页现在使用独立的 `OpenClaw` 轻后端：

- 文件：`openclaw-server.js`
- 默认监听：`127.0.0.1:9012`
- 对外 nginx 路径：`/api/sr/openclaw/`

## 已提供接口

- `GET /api/openclaw/status`
- `POST /api/openclaw/chat`
- `POST /api/openclaw/recommend`
- `POST /api/openclaw/execute`
- `POST /api/openclaw/session/reset`
- `POST /api/openclaw/device/telemetry`
- `GET /api/openclaw/device/next`
- `GET /api/openclaw/device/status`

## 默认定位

它不再是蝴蝶调参后端，而是：

- 智能巡拢家居机器人的任务理解层
- 寻物 / 推拢 / 夹取 / 退出 / 回到用户的高层策略层
- 设备命令队列与设备状态汇总层

## 当前命令风格

后端现在先输出高层命令预览，方便网页链路和设备端解耦推进，例如：

```text
MODE AUTO
TASK SEARCH
ZONE sofa_under
TARGET 遥控器
TASK GRAB
TASK EXIT
TASK RETURN user_feet
```

## 启动方式

```bash
cd /home/deploy/sr/web-part/web-ui
node openclaw-server.js
```

或：

```bash
pm2 start openclaw-server.js --name sr-openclaw
```

## 关键环境变量

- `OPENCLAW_PORT=9012`
- `OPENCLAW_PROVIDER=openai`
- `OPENCLAW_ENDPOINT=https://api.openai.com/v1`
- `OPENCLAW_MODEL=gpt-5.4`
- `OPENCLAW_API_KEY=...`
- `OPENCLAW_EXECUTOR_MODE=queue`
- `OPENCLAW_DEVICE_ID=sr-robot-01`
- `OPENCLAW_DEVICE_TOKEN=...`

完整示例见：

- `server.env.example`

## 当前设备执行模式

默认是 `queue`：

```text
网页 -> sr-openclaw -> 命令队列 -> 设备轮询 next -> 本地执行
```

这样先把网页和 OpenClaw 服务搭稳，不依赖你当前设备端已经完全打通。
