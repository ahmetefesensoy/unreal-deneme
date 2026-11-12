"""Place the arena inside Soul City, with distant enemy sectors and pit exits."""
import unreal,sys,json,math
from pathlib import Path
root=Path(unreal.Paths.project_dir())
sys.path.insert(0,str(root/'Scripts'))
from parkour_geometry import drain_escape
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
OPEN='/Game/SoulCityGame/Maps/L_CityOpen'
COMBAT='/Game/SoulCityGame/Maps/L_CityCombat'
world=unreal.EditorLoadingAndSavingUtils.load_map(OPEN)
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
metal=unreal.load_asset('/Game/SoulCity/Environment/Materials/Building/MI_Slums_Metalwork')
edge=unreal.load_asset('/Game/SoulCityGame/Weapons/Materials/M_ParkourEdge')
def mesh(label,obj,center,size,yaw=0,pitch=0,material=None,collision=True):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector())
    a.set_actor_label(label);c=a.static_mesh_component;c.set_static_mesh(obj)
    c.set_mobility(unreal.ComponentMobility.STATIC)
    e=obj.get_bounds().box_extent
    a.set_actor_scale3d(unreal.Vector(size[0]/max(e.x*2,1),size[1]/max(e.y*2,1),size[2]/max(e.z*2,1)))
    a.set_actor_rotation(unreal.Rotator(pitch=pitch,yaw=yaw,roll=0),False)
    p,_=a.get_actor_bounds(False);a.set_actor_location(unreal.Vector(*center)-p,False,False)
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    if material:c.set_material(0,material)
    return a
for a in list(actors.get_all_level_actors()):
    if a.get_actor_label().startswith('Drain_Escape_') or isinstance(a,unreal.NavMeshBoundsVolume):actors.destroy_actor(a)
drain_escape(mesh,cube,metal,edge)
assert unreal.CityAssetTools.build_arena_navigation(world,unreal.Vector(-800,0,1200),unreal.Vector(26000,16000,6500))
assert unreal.EditorLoadingAndSavingUtils.save_map(world,OPEN)
if unreal.EditorAssetLibrary.does_asset_exist(COMBAT):assert unreal.EditorAssetLibrary.delete_asset(COMBAT)
assert unreal.EditorLoadingAndSavingUtils.save_map(world,COMBAT)
world=unreal.EditorLoadingAndSavingUtils.load_map(COMBAT)
for a in list(actors.get_all_level_actors()):
    if isinstance(a,unreal.PlayerStart) or a.actor_has_tag('CityEnemySpawn'):actors.destroy_actor(a)
for i,y in enumerate([468,668]):
    a=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(-3088,y,130),unreal.Rotator())
    a.set_actor_label('Soul_City_Combat_Start_'+str(i+1))

def floor(x,y):
    hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x,y,450),unreal.Vector(x,y,-300),unreal.TraceTypeQuery.ECC_VISIBILITY,True,[],unreal.DrawDebugTrace.NONE,True)
    if hit is None:return None
    data=hit.to_tuple()
    if not data[0] or data[1] or data[7].z<.7:return None
    p=data[5]
    if p.z<-50 or p.z>400:return None
    center=p+unreal.Vector(0,0,95)
    blocked=unreal.SystemLibrary.capsule_trace_single(world,center,center+unreal.Vector(0,0,1),35,88,unreal.TraceTypeQuery.ECC_VISIBILITY,False,[],unreal.DrawDebugTrace.NONE,True)
    if blocked is not None:return None
    return p

start=unreal.Vector(-3088,568,180)
candidates=[]
for x in range(-200,6501,350):
    for y in range(-2000,2201,350):
        p=floor(x,y)
        if p is None:continue
        distance=math.hypot(p.x-start.x,p.y-start.y)
        if distance<2800:continue
        sight=unreal.SystemLibrary.line_trace_single(world,start,p+unreal.Vector(0,0,150),unreal.TraceTypeQuery.ECC_VISIBILITY,True,[],unreal.DrawDebugTrace.NONE,True)
        nav=unreal.NavigationSystemV1.project_point_to_navigation(world,p,None,None,unreal.Vector(150,150,150))
        candidates.append((0 if sight is None else 1,0 if nav is not None else 1,distance,p))
candidates.sort(key=lambda v:v[:3])
selected=[]
for visible,navigable,distance,p in candidates:
    if any(math.hypot(p.x-q.x,p.y-q.y)<340 for q in selected):continue
    selected.append(p)
    a=actors.spawn_actor_from_class(unreal.TargetPoint,p+unreal.Vector(0,0,95))
    a.set_actor_label('Soul_Enemy_Sector_'+str(len(selected)))
    a.set_editor_property('tags',[unreal.Name('CityEnemySpawn')])
    if len(selected)>=28:break
assert len(selected)>=3,'No safe distant enemy sectors found'
world.get_world_settings().set_editor_property('default_game_mode',unreal.CityGameMode)
world.get_world_settings().set_editor_property('kill_z',-700.0)
assert unreal.EditorLoadingAndSavingUtils.save_map(world,COMBAT)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
(root/'Saved'/'city_combat_setup.json').write_text(json.dumps({'map':COMBAT,'enemy_sectors':len(selected),'floor_candidates':len(candidates),'minimum_spawn_distance_cm':2600,'spawn_points':[[p.x,p.y,p.z] for p in selected],'pit_stair_steps':16},indent=2),encoding='utf-8')
unreal.log('CITY_SOUL_COMBAT_CREATED sectors='+str(len(selected)))
