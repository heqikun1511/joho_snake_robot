# Foxglove monitoring stack

This directory keeps the Foxglove-related runtime configuration and dashboard assets that belong to the monitoring stack.

## Purpose
- Store Foxglove dashboards, layout files, and launch configuration.
- Keep runtime visualization configuration separate from ROS package code.
- Preserve the prior Foxglove tooling while integrating it into the ROS-based architecture.

## Recommended layout
- `configs/` - dashboard JSON and layout definitions
- `extensions/` - custom Foxglove extensions or imported plugin sources
- `docker/` - container files or compose snippets if visualization runs in Docker

## Integration note
The monitoring stack remains a ROS-adjacent component. The ROS package should expose the topics and services required by the dashboard, while the Foxglove-side assets live here for runtime consumption.
