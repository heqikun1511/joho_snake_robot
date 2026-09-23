"""Print the world-space bounds of every STL in a CAD assembly export.

Run from the repository root:
  blender --background --python simulation/tools/inspect_stl_assembly.py -- \
    "蛇形机器人stl"
"""

import sys
from pathlib import Path

import bpy
from mathutils import Vector


def source_directory():
    arguments = sys.argv
    if "--" not in arguments or len(arguments) <= arguments.index("--") + 1:
        raise SystemExit("Pass the STL directory after '--'.")
    return Path(arguments[arguments.index("--") + 1]).resolve()


def bounds_for(obj):
    corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    return tuple(min(c[i] for c in corners) for i in range(3)), tuple(
        max(c[i] for c in corners) for i in range(3)
    )


directory = source_directory()
files = sorted(directory.glob("*.STL")) + sorted(directory.glob("*.stl"))
if not files:
    raise SystemExit(f"No STL files found in {directory}")

for path in files:
    bpy.ops.object.select_all(action="DESELECT")
    bpy.ops.wm.stl_import(filepath=str(path))
    imported = list(bpy.context.selected_objects)
    if len(imported) != 1:
        raise RuntimeError(f"Expected one object from {path}, got {len(imported)}")
    obj = imported[0]
    lower, upper = bounds_for(obj)
    dimensions = tuple(upper[i] - lower[i] for i in range(3))
    print(
        "STL_BOUNDS|{}|min={:.6f},{:.6f},{:.6f}|max={:.6f},{:.6f},{:.6f}"
        "|size={:.6f},{:.6f},{:.6f}".format(
            path.name, *lower, *upper, *dimensions
        )
    )
    bpy.data.objects.remove(obj, do_unlink=True)
