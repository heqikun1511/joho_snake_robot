# snake_bringup

Starts the snake robot description, ros2_control mock hardware, joint state
broadcaster and group position controller.

```bash
ros2 launch snake_bringup mock_control.launch.py
```

For a headless test:

```bash
ros2 launch snake_bringup mock_control.launch.py use_rviz:=false
```
