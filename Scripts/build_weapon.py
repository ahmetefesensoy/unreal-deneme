import unreal
rifle=unreal.CityAssetTools.build_rifle_mesh()
assert rifle
assert unreal.EditorAssetLibrary.save_loaded_asset(rifle,only_if_is_dirty=False)
unreal.log('CITY_PRIMARY_RIFLE_READY '+rifle.get_path_name())
