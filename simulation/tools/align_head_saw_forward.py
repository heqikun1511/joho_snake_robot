"""Attach the head while keeping its chainsaw pointing along the body +Y axis.

The body-side connector has zero CAD rotation, so its local X is the roll axis.
The head 2006 flange is likewise X-axis aligned.  Keeping source orientation
unchanged maps both roll axes together and preserves the head saw direction
(source +Y) as the robot forward direction (world +Y).
"""

from pathlib import Path

import bpy
from mathutils import Matrix, Vector

INPUT = Path("simulation/assets/snake_cad_reference.blend").resolve()
OUTPUT = Path("simulation/assets/snake_cad_head_saw_forward.blend").resolve()

# Metres: centre of body-side "仿生蛇头部连接打印件.-1" and centre of the
# head-side 2006 connection flange.
TARGET_CENTRE = Vector((0.026195, 2.160300, 0.982680))
HEAD_FLANGE_CENTRE = Vector((0.081698120, 0.158284538, 1.055000061))

bpy.ops.wm.open_mainfile(filepath=str(INPUT))
head_objects = [
    obj
    for obj in bpy.context.scene.objects
    if obj.get("source_stl", "").startswith("大创蛇头总装2 - ")
]
if not head_objects:
    raise RuntimeError("No head-assembly objects found in the reference blend.")

# Pure translation: source X -> target X (roll), source Y -> world Y (saw
# forward).  It maps the source flange centre exactly onto the target centre.
mapping = Matrix.Translation(TARGET_CENTRE - HEAD_FLANGE_CENTRE)
for obj in head_objects:
    obj.matrix_world = mapping @ obj.matrix_world

joint = bpy.data.objects.new("head_roll_joint", None)
joint.empty_display_type = "ARROWS"
joint.empty_display_size = 0.12
joint.location = TARGET_CENTRE
joint["joint_type"] = "continuous"
joint["axis_local"] = "+X (body connector roll axis)"
joint["head_forward"] = "+Y (chainsaw direction)"
bpy.context.scene.collection.objects.link(joint)
bpy.context.view_layer.update()

for obj in head_objects:
    world_matrix = obj.matrix_world.copy()
    obj.parent = joint
    # Store the mesh pose explicitly in the joint frame.  Using Blender's
    # parent-inverse here would apply the joint translation a second time.
    obj.matrix_parent_inverse = Matrix.Identity(4)
    obj.matrix_basis = joint.matrix_world.inverted() @ world_matrix

bpy.context.view_layer.update()
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT))
print(f"Aligned {len(head_objects)} components; chainsaw points +Y; wrote {OUTPUT}")
