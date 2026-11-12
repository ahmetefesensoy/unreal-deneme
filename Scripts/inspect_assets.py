import unreal
import json
from pathlib import Path

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
report = {}
for root in ['/Game/ParagonMurdock', '/Game/SoulCity/Maps', '/Game/ThirdPerson/Blueprints']:
    report[root] = [{'path': str(a.package_name), 'class': str(a.asset_class_path.asset_name)} for a in registry.get_assets_by_path(root, recursive=True)]
world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/SoulCity/Maps/LV_Soul_Slum')
report['actors'] = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(a, (unreal.PlayerStart, unreal.CameraActor)):
        report['actors'].append({'name': a.get_name(), 'class': a.get_class().get_name(), 'location': str(a.get_actor_location()), 'rotation': str(a.get_actor_rotation())})
report['world'] = str(world)
report['world_settings'] = str(world.get_world_settings().get_editor_property('default_game_mode'))
Path(unreal.Paths.project_dir(), 'Saved', 'asset_report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('ASSET_INSPECTION_COMPLETE')
