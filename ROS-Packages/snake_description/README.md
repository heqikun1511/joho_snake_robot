# snake_description

Contains the four-module, eight-joint snake robot Xacro description.

Inspect the kinematic model without ros2_control:

```bash
ros2 launch snake_description view_model.launch.py
```

Headless inspection:

```bash
ros2 launch snake_description view_model.launch.py use_gui:=false use_rviz:=false
```
