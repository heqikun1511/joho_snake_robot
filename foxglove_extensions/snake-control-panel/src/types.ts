export type ControlMode = "general" | "puppet" | "guided" | "target";
export type ConnectionState = "disconnected" | "connecting" | "connected";

export type UiCommand = {
  robot_id: string;
  mode: ControlMode;
  action: string;
  parameters: Record<string, unknown>;
  timestamp: string;
};

export type TelemetryLine = {
  id: number;
  time: string;
  topic: string;
  value: string;
};
