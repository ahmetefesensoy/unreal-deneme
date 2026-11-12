"""Recreate local maps after the user has imported both licensed Epic packs."""
from pathlib import Path
import sys
import unreal

root = Path(unreal.Paths.project_dir())
sys.path.insert(0, str(root / "Scripts"))
for asset in (
    "/Game/SoulCity/Maps/LV_Soul_Slum",
    "/Game/ParagonMurdock/Characters/Heroes/Murdock/Meshes/Murdock",
):
    if not unreal.EditorAssetLibrary.does_asset_exist(asset):
        raise RuntimeError("Import the required licensed pack first: " + asset)

for name in (
    "setup_city.py", "setup_combat.py", "build_combat_audio.py",
    "build_weapon.py", "build_open_city.py", "build_city_combat.py",
):
    script = root / "Scripts" / name
    unreal.log("BOOTSTRAP: " + name)
    exec(compile(script.read_text(encoding="utf-8-sig"), str(script), "exec"),
         {"__file__": str(script), "__name__": "__main__"})
unreal.log("CITY_BOOTSTRAP_COMPLETE")
