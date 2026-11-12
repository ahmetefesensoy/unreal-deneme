import unreal
P='/Game/SoulCityGame/Weapons/Materials'
tools=unreal.AssetToolsHelpers.get_asset_tools()
def material(name,color,rough=.3,metal=.7,emissive=0):
    path=P+'/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path): return unreal.load_asset(path)
    m=tools.create_asset(name,P,unreal.Material,unreal.MaterialFactoryNew())
    c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-300,0)
    c.set_editor_property('constant',unreal.LinearColor(*color,1))
    unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
    for prop,value,y in [(unreal.MaterialProperty.MP_ROUGHNESS,rough,150),(unreal.MaterialProperty.MP_METALLIC,metal,250)]:
        n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,-300,y)
        n.set_editor_property('r',value)
        unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
    if emissive:
        e=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-300,350)
        e.set_editor_property('constant',unreal.LinearColor(*(v*emissive for v in color),1))
        unreal.MaterialEditingLibrary.connect_material_property(e,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m,only_if_is_dirty=False)
material('M_OpticHousing',(.028,.036,.043),.26,.85)
material('M_OpticGlass',(.015,.22,.3),.07,.65,.4)
material('M_ParkourEdge',(.9,.28,.035),.4,.35,.12)
material('M_TracerBlue',(.12,.65,1),.2,.1,10)
material('M_TracerRed',(1,.13,.025),.2,.1,10)
unreal.log('CITY_OPTIC_MATERIALS_READY')
