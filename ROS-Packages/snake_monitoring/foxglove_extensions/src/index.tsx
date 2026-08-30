import { ExtensionContext } from '@foxglove/extension';
import { SnakeMonitoringPanel } from './panel';

export function activate(extensionContext: ExtensionContext): void {
  extensionContext.registerPanel({
    name: 'Snake Monitoring',
    id: 'snake.monitoring.panel',
    initPanel: (panelContext) => new SnakeMonitoringPanel(panelContext),
  });
}
