import type { Immutable, MessageEvent, PanelExtensionContext, RenderState } from "@foxglove/studio";
import type { TelemetryLine, UiCommand } from "./types";

//定义了需要的ros2节点
export const TOPICS = {
  command: "/snake/ui/command",
  feedback: "/snake/ui/feedback",
  status: "/snake/ui/status",
  telemetry: "/snake/telemetry",
  jointStates: "/joint_states",
} as const;

export function initializeRos(context: PanelExtensionContext): void {
  context.watch("currentFrame");
  context.watch("topics");
  context.subscribe([
    { topic: TOPICS.feedback }, { topic: TOPICS.status },
    { topic: TOPICS.telemetry }, { topic: TOPICS.jointStates },
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
