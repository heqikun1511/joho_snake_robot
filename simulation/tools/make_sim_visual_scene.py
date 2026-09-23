"""Remove pure electronic-detail meshes from the aligned simulation visual scene.

The source STL directory and the CAD reference blend are intentionally left
untouched.  Mechanical head parts, the depth-camera envelope, and chainsaw
remain in the resulting file.
"""

from pathlib import Path

import bpy

INPUT = Path("simulation/assets/snake_cad_head_saw_forward.blend").resolve()
OUTPUT = Path("simulation/assets/snake_sim_visual.blend").resolve()

bpy.ops.wm.open_mainfile(filepath=str(INPUT))
removed = []
for obj in list(bpy.context.scene.objects):
    source = obj.get("source_stl", "")
    if "Raspberry Pi 4 Model B" in source:
        removed.append(source)
        bpy.data.objects.remove(obj, do_unlink=True)

bpy.context.scene["simulation_visual_note"] = (
    "Raspberry Pi PCB, ICs, connectors, and ports removed; source CAD remains "
    "in 蛇形机器人stl.  This is visual geometry only, not a collision mesh."
)
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT))
print(f"Removed {len(removed)} electronic-detail meshes; wrote {OUTPUT}")
