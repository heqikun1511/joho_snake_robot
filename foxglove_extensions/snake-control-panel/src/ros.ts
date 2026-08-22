//这个文件是 ROS 通信层——负责 Foxglove 面板与 ROS 2 之间的数据收发（订阅、发布）。
import type { Immutable, MessageEvent, PanelExtensionContext, RenderState } from "@foxglove/studio";

import type { TelemetryLine, UiCommand } from "./types";

export const TOPICS = {
  command: "/snake/ui/command",//控制指令
  feedback: "/snake/ui/feedback",//回传反馈
  status: "/snake/ui/status",//机器人状态
  telemetry: "/snake/telemetry",//遥测
  jointStates: "/joint_states",//关节状态
} as const;
//初始化ros2 连接
export function initializeRos(context: PanelExtensionContext): void {
  context.watch("currentFrame");//订阅当前帧数据
  context.watch("topics");//监听可用列表
  context.subscribe([
    { topic: TOPICS.feedback },
    { topic: TOPICS.status },
    { topic: TOPICS.telemetry },
    { topic: TOPICS.jointStates },
  ]);
  context.advertise?.(TOPICS.command, "std_msgs/msg/String");
}

export function publishCommand(context: PanelExtensionContext, command: UiCommand): boolean {
  if (context.publish == undefined) return false;
  context.publish(TOPICS.command, { data: JSON.stringify(command) });
  return true;
}

function messageText(event: MessageEvent<unknown>): string {
  const message = event.message as { data?: unknown };
  if (typeof message?.data === "string") return message.data;
  try { return JSON.stringify(event.message); } catch { return String(event.message); }
}

export function renderStateLines(state: Immutable<RenderState>, startId: number): TelemetryLine[] {
  return (state.currentFrame ?? []).map((event, index) => ({
    id: startId + index,
    time: new Date().toLocaleTimeString("zh-CN", { hour12: false }),
    topic: event.topic,
    value: messageText(event),
  }));
}
