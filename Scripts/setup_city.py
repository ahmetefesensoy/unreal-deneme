import unreal
from pathlib import Path

SOURCE_MAP = '/Game/SoulCity/Maps/LV_Soul_Slum'
PLAY_MAP = '/Game/SoulCityGame/Maps/L_CityPrototype'
world = unreal.EditorLoadingAndSavingUtils.load_map(SOURCE_MAP)
assert world, 'Soul: City map could not be loaded'
game_mode = unreal.CityGameMode
assert game_mode
world.get_world_settings().set_editor_property('default_game_mode', game_mode)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, PLAY_MAP)
unreal.log('CITY_PROTOTYPE_CREATED: ' + PLAY_MAP)
