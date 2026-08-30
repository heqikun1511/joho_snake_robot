# Foxglove extensions for snake_monitoring

This folder keeps the custom Foxglove extension sources associated with the ROS monitoring package.

## Purpose
- Preserve Foxglove custom UI logic and plugins.
- Keep extension development close to the ROS monitoring package.
- Make the visualization layer a first-class part of the ROS architecture.

## Suggested source layout
- `src/` - TypeScript/React source for the custom panel
- `package.json` - extension package metadata
- `tsconfig.json` - TypeScript configuration
- `dist/` - built output for runtime usage

## Migration principle
The Foxglove extension should not live as a standalone repository island. Instead, it sits under the ROS monitoring package and is consumed by the monitoring stack from here.
