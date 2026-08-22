import { createElement } from "react";
import type { ExtensionContext, PanelExtensionContext } from "@foxglove/studio";
import { createRoot } from "react-dom/client";

import { SnakeControlPanel } from "./SnakeControlPanel";
import "./styles.css";

function initPanel(context: PanelExtensionContext): () => void {
  const root = createRoot(context.panelElement);
  root.render(createElement(SnakeControlPanel, { context }));
  return () => root.unmount();
}

export function activate(context: ExtensionContext): void {
  context.registerPanel({
    name: "蛇形机器人",
    initPanel,
  });
}
