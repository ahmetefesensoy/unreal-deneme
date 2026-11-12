import unreal,json
from pathlib import Path
world=unreal.EditorLoadingAndSavingUtils.load_map('/Game/SoulCityGame/Maps/L_CityPrototype')
a=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
r={'actors':[],'starts':[]}
for o in a:
    if isinstance(o,unreal.PlayerStart): r['starts'].append({'pos':str(o.get_actor_location()),'rot':str(o.get_actor_rotation())})
    if isinstance(o,unreal.StaticMeshActor):
        p,e=o.get_actor_bounds(False)
        m=o.static_mesh_component.static_mesh
        r['actors'].append({'name':o.get_actor_label(),'mesh':m.get_path_name() if m else '', 'center':[p.x,p.y,p.z],'extent':[e.x,e.y,e.z],'collision':str(o.static_mesh_component.get_collision_enabled())})
Path(unreal.Paths.project_dir(),'Saved','city_layout.json').write_text(json.dumps(r),encoding='utf-8')
unreal.log('CITY_LAYOUT_READY '+str(len(r['actors'])))
