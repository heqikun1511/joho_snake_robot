# Simulation assets

## Blender outputs

- `assets/snake_cad_reference.blend`: all source STL parts at their CAD poses.
- `assets/snake_cad_head_saw_forward.blend`: head connected to the body-side
  roll-axis centre, with the chainsaw pointing in the body's `+Y` direction.
- `assets/snake_sim_visual.blend`: electronics-detail meshes removed.
- `assets/snake_sim_lowpoly_collision.blend`: Gazebo staging asset. Visual
  geometry is reduced from 1,101,851 to 220,293 triangles and includes the
  `collision_proxies` collection (20 body proxies and one head proxy).

Source STL remains under `蛇形机器人stl/` and is tracked with Git LFS.

## Gazebo workflow

Use the low-poly scene only for `<visual>` meshes. Do not export detailed STL
as a Gazebo `<collision>` mesh. Export each confirmed rigid link separately
and express its collision geometry as its matching box / capsule proxy.

The source export does not encode the fixed-part membership, joint-axis
location, joint limits, mass, or inertia. These must be confirmed before an
articulated SDF is generated. In particular, the servo STL contains both the
servo case and horn, which belong to opposite sides of a revolute joint.

Once `link_00` ... `link_20` membership and axes are confirmed, create a
`model.sdf` containing one `<link>` per rigid group and one `<joint>` per
servo. Gazebo Sim's installed `JointPositionController` plugin can accept a
`gz.msgs.Double` command topic for each joint:

```xml
<plugin filename="gz-sim-joint-position-controller-system"
        name="gz::sim::systems::JointPositionController">
  <joint_name>joint_01</joint_name>
  <topic>joint_01/cmd</topic>
  <p_gain>8</p_gain>
  <d_gain>0.3</d_gain>
</plugin>
```

Launch an empty Gazebo world with:

```bash
source /opt/ros/jazzy/setup.bash
ros2 launch ros_gz_sim gz_sim.launch.py gz_args:="empty.sdf -r"
```
