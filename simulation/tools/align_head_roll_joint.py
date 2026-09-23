"""Align the independently exported head assembly to the body's roll joint.

The CAD exports use different assembly origins.  This script maps the 2006
flange centre/axis of the head onto the centre and longitudinal (+Y) axis of
the body-side printed connector, then parents all head components to an empty
named ``head_roll_joint``.
"""

import math
from pathlib import Path

import bpy
from mathutils import Matrix, Vector

INPUT = Path("simulation/assets/snake_cad_reference.blend").resolve()
OUTPUT = Path("simulation/assets/snake_cad_head_roll_aligned.blend").resolve()

# Coordinates are metres.  The body target is the measured origin of
# "仿生蛇头部连接打印件.-1"; its longitudinal roll axis is +Y.
TARGET_CENTRE = Vector((0.026195, 2.160300, 0.982680))
# Centre of the 2006 connection flange in the separate head CAD assembly.
HEAD_FLANGE_CENTRE = Vector((0.081698120, 0.158284538, 1.055000061))

bpy.ops.wm.open_mainfile(filepath=str(INPUT))

existing = bpy.data.objects.get("head_roll_joint")
if existing:
    bpy.data.objects.remove(existing, do_unlink=True)

head_objects = [
    obj
    for obj in bpy.context.scene.objects
    if obj.get("source_stl", "").startswith("大创蛇头总装2 - ")
]
if not head_objects:
    raise RuntimeError("No head-assembly objects found in the reference blend.")

# Maps source +X (flange/roll axis) to body +Y, then maps flange centre to
# the body connector's target centre.
rotation = Matrix.Rotation(math.radians(90.0), 4, "Z")
mapping = (
    Matrix.Translation(TARGET_CENTRE)
    @ rotation
    @ Matrix.Translation(-HEAD_FLANGE_CENTRE)
)

for obj in head_objects:
    obj.matrix_world = mapping @ obj.matrix_world

joint = bpy.data.objects.new("head_roll_joint", None)
joint.empty_display_type = "ARROWS"
joint.empty_display_size = 0.12
joint.matrix_world = Matrix.Translation(TARGET_CENTRE) @ rotation
joint["joint_type"] = "continuous"
joint["axis_local"] = "+X (world +Y at zero pose)"
joint["source_axis"] = "2006连接法兰 centre axis"
bpy.context.scene.collection.objects.link(joint)

for obj in head_objects:
    world_matrix = obj.matrix_world.copy()
    obj.parent = joint
    obj.matrix_parent_inverse = joint.matrix_world.inverted()
    obj.matrix_world = world_matrix

bpy.context.view_layer.update()
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT))
print(f"Aligned {len(head_objects)} head components and saved {OUTPUT}")
