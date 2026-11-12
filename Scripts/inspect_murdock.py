import unreal
import json
from pathlib import Path

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
items = []
for a in registry.get_assets_by_path('/Game/ParagonMurdock', recursive=True):
    kind = str(a.asset_class_path.asset_name)
    if kind in ['Blueprint', 'AnimBlueprint', 'SkeletalMesh', 'Skeleton', 'BlendSpace', 'BlendSpace1D']:
        entry = {'path': str(a.package_name), 'class': kind}
        items.append(entry)
Path(unreal.Paths.project_dir(), 'Saved', 'murdock_report.json').write_text(json.dumps(items, indent=2), encoding='utf-8')
unreal.log('MURDOCK_INSPECTION_COMPLETE')
