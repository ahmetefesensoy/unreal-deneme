"""Create a broad, navigable rooftop arena from the selected Soul: City assets."""
import unreal
import math
import json
import sys
from pathlib import Path
sys.path.insert(0,str(Path(unreal.Paths.project_dir(),'Scripts')))
from parkour_geometry import fence

ARENA='/Game/SoulCityGame/Maps/L_CityArena'
rifle=unreal.CityAssetTools.build_rifle_mesh()
assert rifle, 'Rifle extraction failed'
unreal.EditorAssetLibrary.save_loaded_asset(rifle, only_if_is_dirty=False)
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
concrete=unreal.load_asset('/Game/SoulCity/Environment/Materials/Building/MI_Slums_Concrete_Wet')
metal=unreal.load_asset('/Game/SoulCity/Environment/Materials/Building/MI_Slums_Metalwork')
edge=unreal.load_asset('/Game/SoulCityGame/Weapons/Materials/M_ParkourEdge')
world.get_world_settings().set_editor_property('default_game_mode',unreal.CityGameMode)
world.get_world_settings().set_editor_property('kill_z',-650.0)

def mesh(label, asset, center, size=None, yaw=0, pitch=0, collision=True, material=None):
    obj=unreal.load_asset(asset) if isinstance(asset,str) else asset
    assert obj, label
    actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector())
    actor.set_actor_label(label)
    comp=actor.static_mesh_component
    comp.set_static_mesh(obj)
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    if material: comp.set_material(0,material)
    if size:
        extent=obj.get_bounds().box_extent
        actor.set_actor_scale3d(unreal.Vector(size[0]/max(2*extent.x,1),size[1]/max(2*extent.y,1),size[2]/max(2*extent.z,1)))
    actor.set_actor_rotation(unreal.Rotator(pitch=pitch,yaw=yaw,roll=0),False)
    actual_center,_=actor.get_actor_bounds(False)
    actor.set_actor_location(unreal.Vector(*center)-actual_center,False,False)
    comp.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    return actor

# 90 x 70 metre main deck. A clear 25 metre spawn courtyard replaces the narrow alley.
for ix in range(9):
    for iy in range(7):
        mesh(f'Deck_{ix}_{iy}',cube,(-4000+ix*1000,-3000+iy*1000,-60),(1000,1000,120),material=concrete)
# Two outer decks and broad bridges; exposed edges lead to the void.
for sign in [-1,1]:
    mesh(f'Outer_Deck_{sign}',cube,(0,sign*4400,-60),(4200,1400,120),material=concrete)
    for bx in [-1300,1300]:
        mesh(f'Bridge_{sign}_{bx}',cube,(bx,sign*3600,-35),(900,1200,70),material=metal)

# Elevated position, reachable by gentle ramps instead of repeated jumping.
mesh('Upper_Combat_Deck',cube,(2000,0,250),(2800,2400,100),material=concrete)
mesh('West_Ramp',cube,(-100,0,140),(1460,1000,50),pitch=12.1,material=metal)
mesh('South_Ramp',cube,(2000,-1900,140),(1460,1000,50),yaw=90,pitch=12.1,material=metal)

for i,(x,y,h,w) in enumerate([
    (-900,-1700,110,650),(-900,1700,110,650),(200,-2350,130,800),(200,2350,130,800),
    (1300,-2500,110,550),(1300,2500,110,550),(3300,-2400,180,500),(3300,2400,180,500),
    (-2500,-1800,85,550),(-2500,1800,85,550),(0,-4400,120,800),(0,4400,120,800)]):
    mesh(f'Cover_{i}',cube,(x,y,h/2),(w,160,h),material=concrete)
    mesh(f'Cover_Metal_Cap_{i}',cube,(x,y,h+4),(w+15,175,8),material=metal)
# Low thresholds can be stepped over automatically; high cover remains a real obstacle.
for i,y in enumerate([-900,900]):
    mesh(f'Low_Service_Step_{i}',cube,(-1500,y,17.5),(600,300,35),material=metal)

building='/Game/SoulCity/Environment/Meshes/Building/SM_S_Soul_Building7'
building2='/Game/SoulCity/Environment/Meshes/Building/SM_S_Soul_Building11'
vent='/Game/SoulCity/Environment/Meshes/Building_Slum/SM_Slums_Vent_01a'
for side in [-1,1]:
    for i,x in enumerate([-5000,-3300,-1400,800,2800,4900]):
        height=2600+(i%3)*800
        mesh(f'Skyline_{side}_{i}',building if i%2 else building2,(x,side*6200,height/2-300),(1500,1200,height),yaw=180 if side==1 else 0,collision=False)
    for i,y in enumerate([-2600,0,2600]):
        mesh(f'West_Background_{side}_{i}',building2,(side*5500,y,1600),(1400,1600,3800),yaw=90,collision=False)
for i,(x,y) in enumerate([(-500,-2800),(-500,2800),(3600,-1400),(3600,1400)]):
    mesh(f'Industrial_Vent_{i}',vent,(x,y,150),(380,220,300),material=None)

# Two generous, separate spawn positions face the open centre.
fence(mesh,cube,metal,edge,'Arena_Vault_Lane',-2400,2000,width=500)
fence(mesh,cube,metal,edge,'Arena_South_Vault',-2100,-1900,width=500,height=100)
for i,(x,z) in enumerate([(-2100,70),(-1450,120),(-800,170)]):
    mesh(f'Arena_Jump_Island_{i}',cube,(x,-3000,z-25),(480,500,50),material=metal)
    mesh(f'Arena_Jump_Marker_{i}',cube,(x+230,-3000,z+3),(15,480,6),material=edge,collision=False)

for i,y in enumerate([-240,240]):
    start=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(-3000,y,100),unreal.Rotator(0,0,0))
    start.set_actor_label(f'Coop_Player_Start_{i+1}')

sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),unreal.Rotator(pitch=-32,yaw=-40,roll=0))
sun.light_component.set_editor_property('intensity',5.0)
sun.light_component.set_light_color(unreal.LinearColor(0.7,0.8,1.0))
sun.light_component.set_editor_property('atmosphere_sun_light',True)
actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1600))
sky.light_component.set_editor_property('intensity',0.8)
sky.light_component.set_editor_property('real_time_capture',True)
fog=actors.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,-150))
fog.component.set_editor_property('fog_density',0.008)
for i,(x,y) in enumerate([(-3400,-2100),(-3400,2100),(3200,-2300),(3200,2300),(0,-4400),(0,4400)]):
    light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(x,y,350))
    light.light_component.set_editor_property('intensity',900.0)
    light.light_component.set_editor_property('attenuation_radius',1100.0)
    light.light_component.set_light_color(unreal.LinearColor(0.12,0.55,1) if i%2 else unreal.LinearColor(1,0.35,0.1))
    light.light_component.set_editor_property('cast_shadows',False)

assert unreal.EditorLoadingAndSavingUtils.save_map(world,ARENA)
nav=unreal.CityAssetTools.build_arena_navigation(world)
assert nav, 'Navigation build did not start'
assert unreal.EditorLoadingAndSavingUtils.save_map(world,ARENA)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
Path(unreal.Paths.project_dir(),'Saved','arena_setup.json').write_text(json.dumps({'map':ARENA,'main_deck_metres':[90,70],'overall_depth_metres':102,'nav_build':nav,'rifle':rifle.get_path_name(),'actors':len(actors.get_all_level_actors())},indent=2),encoding='utf-8')
unreal.log('CITY_ARENA_CREATED')
