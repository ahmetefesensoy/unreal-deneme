import unreal
import json
from pathlib import Path

tools = unreal.AssetToolsHelpers.get_asset_tools()
folder = '/Game/SoulCityGame/Blueprints'
source = '/Game/ParagonMurdock/Characters/Heroes/Murdock/MurdockPlayerCharacter'
base_class = unreal.EditorAssetLibrary.load_blueprint_class(source)
assert base_class, 'Murdock player class did not load'

def blueprint(name, parent):
    path = folder + '/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', parent)
    return tools.create_asset(name, folder, unreal.Blueprint, factory)

player = blueprint('BP_CityMurdock', base_class)
unreal.BlueprintEditorLibrary.compile_blueprint(player)
player_class = unreal.EditorAssetLibrary.load_blueprint_class(folder + '/BP_CityMurdock')
player_defaults = unreal.get_default_object(player_class)
movement = player_defaults.get_editor_property('character_movement')
movement.set_editor_property('max_walk_speed', 450.0)
movement.set_editor_property('jump_z_velocity', 550.0)
movement.set_editor_property('air_control', 0.25)
unreal.EditorAssetLibrary.save_loaded_asset(player, only_if_is_dirty=False)

mode = blueprint('BP_CityGameMode', unreal.GameModeBase)
unreal.BlueprintEditorLibrary.compile_blueprint(mode)
mode_class = unreal.EditorAssetLibrary.load_blueprint_class(folder + '/BP_CityGameMode')
mode_defaults = unreal.get_default_object(mode_class)
mode_defaults.set_editor_property('default_pawn_class', player_class)
unreal.EditorAssetLibrary.save_loaded_asset(mode, only_if_is_dirty=False)

world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/SoulCityGame/Maps/L_CityPrototype')
world.get_world_settings().set_editor_property('default_game_mode', mode_class)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/SoulCityGame/Maps/L_CityPrototype')
report = {
    'player_class': player_class.get_path_name(),
    'mesh': str(player_defaults.get_editor_property('mesh').get_editor_property('skeletal_mesh_asset')),
    'anim_class': str(player_defaults.get_editor_property('mesh').get_editor_property('anim_class')),
    'game_mode': mode_class.get_path_name(),
    'max_walk_speed': movement.get_editor_property('max_walk_speed'),
}
Path(unreal.Paths.project_dir(), 'Saved', 'murdock_setup.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('MURDOCK_SETUP_COMPLETE')
