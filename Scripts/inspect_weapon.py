import unreal
m=unreal.load_asset('/Game/ParagonMurdock/Characters/Heroes/Murdock/Meshes/Murdock')
c=unreal.new_object(unreal.SkeletalMeshComponent)
c.set_skinned_asset_and_update(m)
for name in c.get_all_socket_names():
    s=str(name)
    if any(x in s.lower() for x in ['gun','weapon']):
        unreal.log('CITY_BONE '+s+' parent='+str(c.get_parent_bone(name)))
for i in range(c.get_num_materials()):
    unreal.log('CITY_MATERIAL '+str(i)+' '+str(c.get_material(i)))
