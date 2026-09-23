"""Create a metre-scaled, editable Blender reference assembly from the STL export.

Run from the repository root:
  blender --background --python simulation/tools/prepare_snake_blender.py -- \
    "蛇形机器人stl" simulation/assets/snake_cad_reference.blend

The result deliberately keeps one Blender object per source STL.  Do not merge
the objects before deciding which parts are rigidly fixed to each simulated link.
"""

import re
import sys
from pathlib import Path

import bpy

MM_TO_M = 0.001


def arguments():
    if "--" not in sys.argv:
        raise SystemExit("Pass input STL directory and output .blend path after '--'.")
    values = sys.argv[sys.argv.index("--") + 1 :]
    if len(values) != 2:
        raise SystemExit("Expected: <stl-directory> <output-blend>")
    return Path(values[0]).resolve(), Path(values[1]).resolve()


def number_in(name):
    match = re.search(r"\.-(\d+)\.STL$", name, re.IGNORECASE)
    return int(match.group(1)) if match else None


def component_identity(filename):
    number = number_in(filename)
    suffix = f"_{number:02d}" if number is not None else ""
    if "双头舵机" in filename:
        return f"servo{suffix}", "servo", number
    if "窄U支架" in filename:
        return f"u_bracket{suffix}", "u_bracket", number
    if "舵机圆支架" in filename:
        return f"round_bracket{suffix}", "round_bracket", number
    return filename.rsplit(".", 1)[0], "auxiliary", number


source, output = arguments()
files = sorted(source.glob("*.STL")) + sorted(source.glob("*.stl"))
if not files:
    raise SystemExit(f"No STL files found in: {source}")

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for collection in list(bpy.data.collections):
    bpy.data.collections.remove(collection)

assembly = bpy.data.collections.new("CAD_reference_parts")
bpy.context.scene.collection.children.link(assembly)

for path in files:
    bpy.ops.object.select_all(action="DESELECT")
    bpy.ops.wm.stl_import(filepath=str(path))
    imported = list(bpy.context.selected_objects)
    if len(imported) != 1:
        raise RuntimeError(f"Expected exactly one object from {path.name}")
    obj = imported[0]

    # STL data is in millimetres and its vertices encode the CAD assembly pose.
    obj.scale = (MM_TO_M, MM_TO_M, MM_TO_M)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")

    name, category, number = component_identity(path.name)
    obj.name = name
    obj.data.name = f"{name}_mesh"
    obj["source_stl"] = path.name
    obj["category"] = category
    if number is not None:
        obj["assembly_index"] = number
        if category in {"servo", "u_bracket"}:
            obj["candidate_joint"] = f"joint_{number:02d}"

    for collection in list(obj.users_collection):
        collection.objects.unlink(obj)
    assembly.objects.link(obj)

scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.length_unit = "MILLIMETERS"
scene["source_directory"] = str(source)
scene["mesh_units"] = "metres (source STL converted from millimetres)"
scene["next_step"] = (
    "Create one collection per rigid link; move only parts rigidly fixed together "
    "into it, then place each link origin at its physical servo output axis."
)

output.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(output))
print(f"Wrote {output} with {len(files)} individually named CAD parts.")
