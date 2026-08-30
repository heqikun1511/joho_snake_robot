import { PanelExtensionContext, RenderState } from '@foxglove/extension';

export class SnakeMonitoringPanel {
  constructor(private readonly context: PanelExtensionContext) {}

  public render(): JSX.Element {
    const state = this.context.getCurrentLayout();
    return (
      <div style={{ padding: 12, color: '#e5e7eb', background: '#111827', height: '100%' }}>
        <h3 style={{ margin: 0, marginBottom: 8 }}>Snake Monitoring</h3>
        <p style={{ margin: 0 }}>ROS topics and robot state are available via the connected Foxglove session.</p>
        <pre style={{ marginTop: 12, whiteSpace: 'pre-wrap' }}>{JSON.stringify(state, null, 2)}</pre>
      </div>
    );
  }

  public update(): void {
    // Panel update logic can be added here.
  }

  public destroy(): void {
    // Cleanup logic can be added here.
  }
}
