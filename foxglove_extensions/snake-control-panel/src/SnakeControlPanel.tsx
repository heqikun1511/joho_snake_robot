import type { PanelExtensionContext } from "@foxglove/studio";
import { useCallback, useEffect, useMemo, useRef, useState } from "react";

import { initializeRos, publishCommand, renderStateLines, TOPICS } from "./ros";
import type { ConnectionState, ControlMode, TelemetryLine } from "./types";

type Props = { context: PanelExtensionContext };
type NumberMap = Record<string, number>;

const modeNames: Record<ControlMode, string> = {
  general: "通用控制", puppet: "木偶式控制", guided: "导向式控制", target: "目标式控制",
};
const joints = ["pitch_joint_1", "yaw_joint_1", "pitch_joint_2", "yaw_joint_2", "pitch_joint_3", "yaw_joint_3", "pitch_joint_4", "yaw_joint_4"];
const gaitFields = ["alpha_y", "omega_y", "beta_y", "gamma_y", "alpha_p", "omega_p", "beta_p", "gamma_p"];
const imageSources = ["无", "RGB 图像", "深度图", "三维重建图"];

function elapsedText(seconds: number): string {
  return [Math.floor(seconds / 3600), Math.floor((seconds % 3600) / 60), seconds % 60]
    .map((value) => String(value).padStart(2, "0")).join(":");
}

function ControlField({ label, value, onChange, step = 0.1 }: { label: string; value: number; step?: number; onChange: (value: number) => void }): JSX.Element {
  return <label className="control-field"><span>{label}</span><input type="number" step={step} value={value} onChange={(event) => onChange(Number(event.target.value))} /></label>;
}

function PuppetControls({ jointValues, gait, onJoint, onGait }: { jointValues: NumberMap; gait: NumberMap; onJoint: (name: string, value: number) => void; onGait: (name: string, value: number) => void }): JSX.Element {
  const [inputType, setInputType] = useState<"joint" | "gait">("joint");
  return <div className="mode-content">
    <div className="segmented"><button className={inputType === "joint" ? "active" : ""} onClick={() => setInputType("joint")}>关节角度</button><button className={inputType === "gait" ? "active" : ""} onClick={() => setInputType("gait")}>步态参数</button></div>
    {inputType === "joint" ? <div className="joint-grid">{joints.map((joint) => <label className="joint-row" key={joint}><span>{joint}</span><input type="range" min={-90} max={90} value={jointValues[joint]} onChange={(event) => onJoint(joint, Number(event.target.value))} /><input type="number" min={-90} max={90} value={jointValues[joint]} onChange={(event) => onJoint(joint, Number(event.target.value))} /><em>°</em></label>)}</div>
      : <div className="form-grid">{gaitFields.map((field) => <ControlField key={field} label={field} value={gait[field]} onChange={(value) => onGait(field, value)} />)}</div>}
  </div>;
}

function SpineVisualizer({ jointValues }: { jointValues: NumberMap }): JSX.Element {
  const [selected, setSelected] = useState(0);
  const points = useMemo(() => {
    const result = [{ x: 46, y: 104 }];
    let heading = 0;
    for (let index = 0; index < 8; index++) {
      const pair = Math.floor(index / 2) + 1;
      const yaw = jointValues[`yaw_joint_${pair}`] ?? 0;
      const pitch = jointValues[`pitch_joint_${pair}`] ?? 0;
      heading += (yaw * Math.PI) / 720;
      const previous = result[result.length - 1]!;
      result.push({ x: previous.x + 84 * Math.cos(heading), y: previous.y + 84 * Math.sin(heading) + Math.sin((pitch * Math.PI) / 180) * 8 });
    }
    return result;
  }, [jointValues]);
  const pair = Math.min(4, Math.floor(selected / 2) + 1);

  return <section className="spine-panel">
    <header><div><span className="eyebrow">BIOMIMETIC SPINE</span><h2>仿生脊柱姿态</h2></div><div className="spine-legend"><span><i className="pitch-dot" />PITCH</span><span><i className="yaw-dot" />YAW</span><b>8 JOINTS ONLINE</b></div></header>
    <div className="spine-stage"><svg viewBox="0 0 790 210" role="img" aria-label="蛇形机器人脊柱姿态"><defs><filter id="spineGlow"><feGaussianBlur stdDeviation="4" result="blur" /><feMerge><feMergeNode in="blur" /><feMergeNode in="SourceGraphic" /></feMerge></filter><linearGradient id="spineGradient"><stop stopColor="#34d5c5" /><stop offset=".55" stopColor="#38bdf8" /><stop offset="1" stopColor="#a78bfa" /></linearGradient></defs><polyline className="spine-shadow" points={points.map((point) => `${point.x},${point.y}`).join(" ")} /><polyline className="spine-line" points={points.map((point) => `${point.x},${point.y}`).join(" ")} />{points.map((point, index) => <g className={selected === index ? "spine-joint selected" : "spine-joint"} key={index} onClick={() => setSelected(index)} transform={`translate(${point.x} ${point.y})`}><circle className="joint-ring" r="13" /><circle className={index % 2 === 0 ? "joint-core pitch" : "joint-core yaw"} r="6" /><text y="30">{String(index + 1).padStart(2, "0")}</text></g>)}<path className="spine-head" d={`M ${points[8]!.x + 2} ${points[8]!.y - 12} l 25 12 -25 12 z`} /></svg><div className="joint-readout"><span>SELECTED</span><strong>JOINT {String(selected + 1).padStart(2, "0")}</strong><div><em>P</em>{(jointValues[`pitch_joint_${pair}`] ?? 0).toFixed(1)}°</div><div><em>Y</em>{(jointValues[`yaw_joint_${pair}`] ?? 0).toFixed(1)}°</div></div></div>
  </section>;
}

function ImageViewport({ index, onRecord }: { index: number; onRecord: (index: number, source: string, enabled: boolean) => void }): JSX.Element {
  const [source, setSource] = useState(index === 0 ? "RGB 图像" : "无");
  const [expanded, setExpanded] = useState(false);
  const [recording, setRecording] = useState(false);
  const toggleRecord = () => { const next = !recording; setRecording(next); onRecord(index, source, next); };
  return <section className={expanded ? "image-card expanded" : "image-card"}>
    <header><div><span className="eyebrow">VIDEO 0{index + 1}</span><strong>图像窗口 {index + 1}</strong></div><button className="icon-button" onClick={() => setExpanded((value) => !value)}>{expanded ? "恢复" : "放大"}</button></header>
    <div className="image-stage"><div className="grid-overlay" /><div className="scan-line" /><span>{source === "无" ? "未选择图源" : `${source} / 等待数据`}</span></div>
    <footer><select value={source} onChange={(event) => setSource(event.target.value)}>{imageSources.map((item) => <option key={item}>{item}</option>)}</select><button className={recording ? "button danger" : "button"} disabled={source === "无"} onClick={toggleRecord}>{recording ? "停止保存" : "保存图像"}</button></footer>
  </section>;
}

export function SnakeControlPanel({ context }: Props): JSX.Element {
  const [robotId, setRobotId] = useState("snake-001");
  const [connection, setConnection] = useState<ConnectionState>("disconnected");
  const [elapsed, setElapsed] = useState(0);
  const [ledOn, setLedOn] = useState(false);
  const [mode, setMode] = useState<ControlMode>("general");
  const [command, setCommand] = useState("sensor imu rate=100 filter=lowpass");
  const [recording, setRecording] = useState(false);
  const [telemetry, setTelemetry] = useState<TelemetryLine[]>([]);
  const [jointValues, setJointValues] = useState<NumberMap>(() => Object.fromEntries(joints.map((name) => [name, 0])));
  const [gait, setGait] = useState<NumberMap>(() => Object.fromEntries(gaitFields.map((name) => [name, 0])));
  const [guided, setGuided] = useState({ speed: 0.2, turn: 0.3 });
  const [target, setTarget] = useState({ x: 0, y: 0, z: 0, yaw: 0, tolerance: 0.05 });
  const nextId = useRef(1);

  const appendLocal = useCallback((value: string) => setTelemetry((previous) => [...previous, { id: nextId.current++, time: new Date().toLocaleTimeString("zh-CN", { hour12: false }), topic: "LOCAL", value }].slice(-250)), []);

  useEffect(() => {
    initializeRos(context);
    context.onRender = (state, done) => {
      const lines = renderStateLines(state, nextId.current); nextId.current += lines.length;
      if (lines.length > 0) setTelemetry((previous) => [...previous, ...lines].slice(-250));
      done();
    };
    return () => { context.onRender = undefined; context.unsubscribeAll(); context.unadvertise?.(TOPICS.command); };
  }, [context]);

  useEffect(() => {
    if (connection !== "connected") return;
    const timer = window.setInterval(() => setElapsed((value) => value + 1), 1000);
    return () => window.clearInterval(timer);
  }, [connection]);

  const send = useCallback((action: string, parameters: Record<string, unknown>) => {
    const ok = publishCommand(context, { robot_id: robotId, mode, action, parameters, timestamp: new Date().toISOString() });
    appendLocal(`${ok ? "已发送" : "数据源不支持发布"}: ${action}`);
  }, [appendLocal, context, mode, robotId]);

  const toggleConnection = () => {
    if (connection === "connected") { send("disconnect", {}); setConnection("disconnected"); setElapsed(0); return; }
    setConnection("connecting"); send("connect", {}); window.setTimeout(() => setConnection("connected"), 600);
  };
  const toggleLed = () => { const next = !ledOn; setLedOn(next); send("set_led", { enabled: next }); };
  const statusName = useMemo(() => ({ disconnected: "未连接", connecting: "连接中", connected: "已连接" })[connection], [connection]);

  const modePanel = () => {
    if (mode === "general") return <div className="mode-content"><div className="form-grid"><ControlField label="传感器采样率 / Hz" value={100} onChange={() => undefined} step={1} /><ControlField label="滤波截止频率 / Hz" value={20} onChange={() => undefined} step={1} /><ControlField label="PID Kp" value={1.2} onChange={() => undefined} /><ControlField label="PID Ki" value={0.1} onChange={() => undefined} /><ControlField label="PID Kd" value={0.03} onChange={() => undefined} /><label className="control-field"><span>滤波算法</span><select><option>低通滤波</option><option>均值滤波</option><option>卡尔曼滤波</option></select></label></div></div>;
    if (mode === "puppet") return <PuppetControls jointValues={jointValues} gait={gait} onJoint={(name, value) => setJointValues((old) => ({ ...old, [name]: value }))} onGait={(name, value) => setGait((old) => ({ ...old, [name]: value }))} />;
    if (mode === "guided") return <div className="mode-content guided"><div className="direction-pad"><button onMouseDown={() => send("velocity", { direction: "forward", ...guided })} onMouseUp={() => send("stop", {})}>↑</button><button onMouseDown={() => send("velocity", { direction: "left", ...guided })} onMouseUp={() => send("stop", {})}>←</button><button className="stop" onClick={() => send("emergency_stop", {})}>STOP</button><button onMouseDown={() => send("velocity", { direction: "right", ...guided })} onMouseUp={() => send("stop", {})}>→</button><button onMouseDown={() => send("velocity", { direction: "backward", ...guided })} onMouseUp={() => send("stop", {})}>↓</button></div><div className="form-grid"><ControlField label="移动速度" value={guided.speed} onChange={(speed) => setGuided((old) => ({ ...old, speed }))} /><ControlField label="转向速度" value={guided.turn} onChange={(turn) => setGuided((old) => ({ ...old, turn }))} /></div></div>;
    return <div className="mode-content"><div className="notice">目标导航后端算法待接入，当前可发布目标参数。</div><div className="form-grid">{Object.entries(target).map(([name, value]) => <ControlField key={name} label={name.toUpperCase()} value={value} onChange={(next) => setTarget((old) => ({ ...old, [name]: next }))} />)}</div><button className="button primary" onClick={() => send("navigate", target)}>发送目标位置</button></div>;
  };

  return <main className="snake-console">
    <header className="topbar"><div className="brand"><div className="brand-mark">蛇</div><div><strong>蛇形机器人</strong><small>GENERATION 03 / ROS 2</small></div></div><label className="robot-id"><span>机器人标识码</span><input list="robot-history" value={robotId} onChange={(event) => setRobotId(event.target.value)} /><datalist id="robot-history"><option value="snake-001" /><option value="snake-002" /></datalist></label><button className={connection === "connected" ? "button active" : "button"} onClick={toggleConnection}>{connection === "connected" ? "断开连接" : "连接机器人"}</button><div className="connection"><span className={`status-light ${connection}`} /><div><strong>{statusName}</strong><small>{elapsedText(elapsed)}</small></div></div><button className={ledOn ? "led on" : "led"} onClick={toggleLed}>{ledOn ? "● LED 亮" : "○ LED 灭"}</button></header>
    <nav className="tabs">{(Object.keys(modeNames) as ControlMode[]).map((item) => <button key={item} className={item === mode ? "active" : ""} onClick={() => setMode(item)}><span>{modeNames[item]}</span><small>{item.toUpperCase()}</small></button>)}</nav>
    <SpineVisualizer jointValues={jointValues} />
    <section className="control-layout"><article className="panel controls"><header><div><span className="eyebrow">CONTROL MODE</span><h2>{modeNames[mode]}</h2></div><span className="mode-tag">{mode.toUpperCase()}</span></header>{modePanel()}</article><article className="panel command"><header><div><span className="eyebrow">COMMAND DOWNLOAD</span><h2>控制命令</h2></div></header><textarea value={command} onChange={(event) => setCommand(event.target.value)} /><div className="action-row"><code>{TOPICS.command}</code><button className="button primary" onClick={() => send("execute", { command })}>发送命令</button></div></article><article className="panel telemetry"><header><div><span className="eyebrow">TELEMETRY</span><h2>数据回传</h2></div><div><button className="text-button" onClick={() => setTelemetry([])}>清空</button><button className={recording ? "button danger" : "button"} onClick={() => { const next = !recording; setRecording(next); send("record_telemetry", { enabled: next }); }}>{recording ? "停止保存" : "保存数据"}</button></div></header><div className="telemetry-list">{telemetry.length === 0 ? <div className="empty"><b>∿</b><span>等待传感器和关节数据</span></div> : telemetry.map((line) => <div className="telemetry-row" key={line.id}><time>{line.time}</time><strong>{line.topic}</strong><span>{line.value}</span></div>)}</div></article></section>
    <section className="image-grid">{[0, 1, 2].map((index) => <ImageViewport key={index} index={index} onRecord={(channel, source, enabled) => send("record_image", { channel, source, enabled })} />)}</section>
  </main>;
}
