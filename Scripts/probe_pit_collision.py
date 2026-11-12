import unreal,json
from pathlib import Path
w=unreal.EditorLoadingAndSavingUtils.load_map('/Game/SoulCityGame/Maps/L_CityCombat')
def describe(r):
    if r is None:return None
    d=r.to_tuple();a=d[9];c=d[10]
    return {'actor':a.get_actor_label() if a else '', 'component':c.get_name() if c else '', 'mesh':c.static_mesh.get_name() if isinstance(c,unreal.StaticMeshComponent) and c.static_mesh else '', 'point':[d[5].x,d[5].y,d[5].z],'normal':[d[7].x,d[7].y,d[7].z],'initial':d[1]}
result={'capsule':describe(unreal.SystemLibrary.capsule_trace_single(w,unreal.Vector(9681,-3102,-10),unreal.Vector(9681,-3002,-10),34,88,unreal.TraceTypeQuery.ECC_VISIBILITY,False,[],unreal.DrawDebugTrace.NONE,True)), 'up':[]}
for x in [9600,9700,9800]:
    for y in [-3150,-3050,-2950,-2850,-2750,-2650]:
        r=unreal.SystemLibrary.line_trace_single(w,unreal.Vector(x,y,-120),unreal.Vector(x,y,700),unreal.TraceTypeQuery.ECC_VISIBILITY,False,[],unreal.DrawDebugTrace.NONE,True)
        result['up'].append({'x':x,'y':y,'hit':describe(r)})
Path(unreal.Paths.project_dir(),'Saved','pit_collision.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
unreal.log('CITY_PIT_PROBE_READY')
