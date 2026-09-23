"""Generate a Gazebo-oriented visual scene and collision-proxy staging scene.

The detailed visual meshes are decimated to reduce renderer load.  Collision
objects are deliberately boxes, one around each servo/U-bracket joint module
and one around the head: these are staging proxies for SDF, not exported CAD.
"""

from pathlib import Path

import bpy
from mathutils import Vector

INPUT = Path("simulation/assets/snake_sim_visual.blend").resolve()
OUTPUT = Path("simulation/assets/snake_sim_lowpoly_collision.blend").resolve()
VISUAL_RATIO = 0.20
COLLISION_MARGIN_M = 0.006


def world_bounds(objects):
    corners = [obj.matrix_world @ Vector(corner) for obj in objects for corner in obj.bound_box]
    return (
        Vector(tuple(min(point[i] for point in corners) for i in range(3))),
        Vector(tuple(max(point[i] for point in corners) for i in range(3))),
    )


def proxy(collection, name, objects, link_name):
    lower, upper = world_bounds(objects)
    centre = (lower + upper) / 2
    dimensions = upper - lower + Vector((COLLISION_MARGIN_M,) * 3)
    bpy.ops.mesh.primitive_cube_add(location=centre)
    obj = bpy.context.active_object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for old_collection in list(obj.users_collection):
        old_collection.objects.unlink(obj)
    collection.objects.link(obj)
    obj.display_type = "WIRE"
    obj.hide_render = True
    obj["collision_proxy"] = True
    obj["link_name"] = link_name
    obj["geometry"] = "box"
    obj["source"] = "servo and U-bracket bounds"


bpy.ops.wm.open_mainfile(filepath=str(INPUT))
mesh_objects = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
before_triangles = sum(len(obj.data.polygons) for obj in mesh_objects)

for obj in mesh_objects:
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    modifier = obj.modifiers.new("gazebo_visual_decimate", "DECIMATE")
    modifier.ratio = VISUAL_RATIO
    modifier.decimate_type = "COLLAPSE"
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.select_set(False)

collision = bpy.data.collections.new("collision_proxies")
bpy.context.scene.collection.children.link(collision)
for index in range(1, 21):
    members = [
        obj
        for obj in mesh_objects
        if obj.get("assembly_index") == index
        and obj.get("category") in {"servo", "u_bracket"}
    ]
    if len(members) == 2:
        proxy(collision, f"collision_link_{index:02d}", members, f"link_{index:02d}")

head = [
    obj
    for obj in mesh_objects
    if obj.get("source_stl", "").startswith("大创蛇头总装2 - ")
]
if head:
    proxy(collision, "collision_head_link", head, "head_link")

after_triangles = sum(len(obj.data.polygons) for obj in mesh_objects)
scene = bpy.context.scene
scene["visual_triangles_before"] = before_triangles
scene["visual_triangles_after"] = after_triangles
scene["collision_proxy_count"] = len(collision.objects)
scene["collision_note"] = (
    "Use these boxes as a starting point for SDF collision geometry.  Refine "
    "dimensions after link membership and measured masses are verified."
)

OUTPUT.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT))
print(
    f"Wrote {OUTPUT}: visual triangles {before_triangles} -> {after_triangles}; "
    f"collision proxies={len(collision.objects)}"
)
