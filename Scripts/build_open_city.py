"""Preserve the Soul City streets and connect a broad western parkour district."""
import unreal,math,json,sys
from pathlib import Path
sys.path.insert(0,str(Path(unreal.Paths.project_dir(),'Scripts')))
from parkour_geometry import fence,drain_escape

MAP='/Game/SoulCityGame/Maps/L_CityOpen'
world=unreal.EditorLoadingAndSavingUtils.load_map('/Game/SoulCityGame/Maps/L_CityPrototype')
assert world
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    assert unreal.EditorAssetLibrary.delete_asset(MAP)
assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
world=unreal.EditorLoadingAndSavingUtils.load_map(MAP)
assert world
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
removed=0
for actor in list(actors.get_all_level_actors()):
    if isinstance(actor,(unreal.PlayerStart,unreal.NavMeshBoundsVolume,unreal.BlockingVolume)):
        actors.destroy_actor(actor)
    elif isinstance(actor,unreal.StaticMeshActor):
        c,e=actor.get_actor_bounds(False)
        # Clear the new connector and district only; original city streets east of it remain.
        if actor.get_actor_label()=='SM_Cave_Rock_Large406' or (c.x < -3150 and -3500<c.y<4000 and -500<c.z<3500 and e.x<5000):
            actors.destroy_actor(actor); removed+=1

cube=unreal.load_asset('/Engine/BasicShapes/Cube')
concrete=unreal.load_asset('/Game/SoulCity/Environment/Materials/Building/MI_Slums_Concrete_Wet')
metal=unreal.load_asset('/Game/SoulCity/Environment/Materials/Building/MI_Slums_Metalwork')
edge=unreal.load_asset('/Game/SoulCityGame/Weapons/Materials/M_ParkourEdge')

def mesh(label,asset,center,size=None,yaw=0,pitch=0,collision=True,material=None):
    obj=unreal.load_asset(asset) if isinstance(asset,str) else asset
    assert obj,label
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector())
    a.set_actor_label(label)
    comp=a.static_mesh_component
    comp.set_static_mesh(obj); comp.set_mobility(unreal.ComponentMobility.STATIC)
    if material: comp.set_material(0,material)
    if size:
        e=obj.get_bounds().box_extent
        a.set_actor_scale3d(unreal.Vector(size[0]/max(2*e.x,1),size[1]/max(2*e.y,1),size[2]/max(2*e.z,1)))
    a.set_actor_rotation(unreal.Rotator(pitch=pitch,yaw=yaw,roll=0),False)
    c,_=a.get_actor_bounds(False)
    a.set_actor_location(unreal.Vector(*center)-c,False,False)
    comp.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    return a

# 45 x 60 metre neighbourhood with several routes around the buildings.
for ix in range(5):
    for iy in range(6):
        mesh(f'West_District_Street_{ix}_{iy}',cube,(-10300+ix*900,-1900+iy*1000,740),(900,1000,120),material=concrete)

# A six-metre-wide promenade descends into the original Soul City entrance.
length=math.hypot(3200,770)
mesh('Soul_City_Entry_Promenade',cube,(-4700,568,415),(length,600,40),pitch=-math.degrees(math.atan2(770,3200)),material=metal)
mesh('Soul_City_Entry_Landing',cube,(-3090,568,15),(320,540,40),material=metal)
for side in [-1,1]:
    mesh('Promenade_Edge_'+str(side),cube,(-4700,568+side*280,440),(length,12,12),pitch=-math.degrees(math.atan2(770,3200)),material=edge,collision=False)

house='/Game/SoulCity/Environment/Meshes/Building_Slum/SM_Slums_Distance_01b_OP'
vent='/Game/SoulCity/Environment/Meshes/Building_Slum/SM_Slums_Vent_01a'
for i,(x,y,top) in enumerate([(-9800,2700,1400),(-7700,2700,1500),(-9800,-1400,1400),(-7700,-1400,1500)]):
    h=top-800
    mesh(f'West_House_{i}',house,(x,y,800+h/2),(1100,900,h),yaw=90 if i%2 else 0,collision=False)
    body=mesh(f'West_House_Solid_{i}',cube,(x,y,800+h/2),(1060,860,h),material=concrete)
    body.set_actor_hidden_in_game(True)
    mesh(f'Parkour_Roof_{i}',cube,(x,y,top-10),(1140,940,20),material=metal)
    mesh(f'Roof_Vent_{i}',vent,(x+300,y+220,top+80),(160,120,160),collision=True)
    mesh(f'Roof_Edge_Marker_{i}',cube,(x,y-460,top+3),(1100,12,6),material=edge,collision=False)

# Two staircase approaches make the roof circuit usable without vaulting.
for i in range(30):
    h=(i+1)*20
    mesh(f'North_Roof_Stair_{i}',cube,(-9800,1450+i*25,800+h/2),(350,25,h),material=metal)
mesh('North_Stair_Landing',cube,(-9800,2240,1390),(350,150,20),material=metal)
for i in range(35):
    h=(i+1)*20
    mesh(f'South_Roof_Stair_{i}',cube,(-7700,-100-i*25,800+h/2),(350,25,h),material=metal)

# A safe bridge on the south loop and a sprint-jump gap on the north loop.
mesh('South_Roof_Bridge',cube,(-8750,-1400,1450),(1200,400,40),pitch=4.8,material=metal)
for i,(x,z) in enumerate([(-9100,1400),(-8440,1450)]):
    mesh(f'North_Jump_Island_{i}',cube,(x,2700,z-35),(500,500,70),material=metal)
    mesh(f'North_Jump_Marker_{i}',cube,(x+240,2700,z+3),(15,480,6),material=edge,collision=False)
    mesh(f'North_Jump_Support_{i}',cube,(x,2700,(z+800)/2),(100,100,z-800),material=metal)

fence(mesh,cube,metal,edge,'West_Plaza_Vault',-9000,600,800,width=900)
fence(mesh,cube,metal,edge,'South_Alley_Vault',-8450,-1100,800,width=500)
fence(mesh,cube,metal,edge,'North_Roof_Vault',-9600,2700,1400,width=700,height=100)
mesh('High_Alley_Wall',cube,(-7000,1800,910),(60,600,220),material=concrete)
for y in [-50,1250]:
    mesh('Plaza_Route_Line_'+str(y),cube,(-8100,y,803),(3600,14,6),material=edge,collision=False)

for i,y in enumerate([360,840]):
    a=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(-9800,y,900),unreal.Rotator())
    a.set_actor_label('Open_City_Coop_Start_'+str(i+1))
for i,(x,y) in enumerate([(-9900,600),(-8700,1800),(-7100,600),(-8700,-500),(-6200,600)]):
    pole=mesh('West_Streetlamp_'+str(i),cube,(x,y,990),(18,18,380),material=metal)
    mesh('Streetlamp_Lens_'+str(i),cube,(x,y,1185),(70,70,15),material=edge,collision=False)
    light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(x,y,1150))
    light.light_component.set_editor_property('intensity',60.0)
    light.light_component.set_editor_property('attenuation_radius',1700.0)
    light.light_component.set_light_color(unreal.LinearColor(.35,.65,1))
    light.light_component.set_editor_property('cast_shadows',False)
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.SkyLight):
        a.light_component.set_editor_property('intensity',max(.75,a.light_component.get_editor_property('intensity')))
    elif isinstance(a,unreal.DirectionalLight):
        a.light_component.set_editor_property('intensity',max(1.,a.light_component.get_editor_property('intensity')))

world.get_world_settings().set_editor_property('default_game_mode',unreal.CityGameMode)
drain_escape(mesh,cube,metal,edge)
world.get_world_settings().set_editor_property('kill_z',-700.0)
assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
assert unreal.CityAssetTools.build_arena_navigation(world,unreal.Vector(-800,0,1200),unreal.Vector(26000,16000,6500))
assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
Path(unreal.Paths.project_dir(),'Saved','open_city_setup.json').write_text(json.dumps({'map':MAP,'new_district_metres':[45,60],'entry_ramp_width_metres':6,'new_roof_routes':2,'cleared_west_connector_actors':removed,'actors':len(actors.get_all_level_actors())},indent=2),encoding='utf-8')
unreal.log('CITY_OPEN_CREATED')
