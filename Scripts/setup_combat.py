import unreal
import json
from pathlib import Path

world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/SoulCityGame/Maps/L_CityPrototype')
world.get_world_settings().set_editor_property('default_game_mode', unreal.CityGameMode)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/SoulCityGame/Maps/L_CityPrototype')
blend = unreal.load_asset('/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/BlendSpaces/Murdock_Locomotion_BS')
report = {'blend': str(blend), 'axes': str(blend.get_editor_property('blend_parameters')), 'samples': str(blend.get_editor_property('sample_data'))}
mesh = unreal.load_asset('/Game/ParagonMurdock/Characters/Heroes/Murdock/Meshes/Murdock')
report['mesh'] = str(mesh)
Path(unreal.Paths.project_dir(), 'Saved', 'combat_assets.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('COMBAT_MAP_READY')
