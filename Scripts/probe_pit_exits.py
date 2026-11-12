import unreal,json
from pathlib import Path
w=unreal.EditorLoadingAndSavingUtils.load_map('/Game/SoulCityGame/Maps/L_CityCombat')
r=[]
for x in [8800,9000,9200,9400,9600,9800,10000]:
    for y in [-3000,-2700,-2400,-2100]:
        h=unreal.SystemLibrary.line_trace_single(w,unreal.Vector(x,y,650),unreal.Vector(x,y,-650),unreal.TraceTypeQuery.ECC_VISIBILITY,False,[],unreal.DrawDebugTrace.NONE,True)
        if h is not None:
            d=h.to_tuple();r.append({'x':x,'y':y,'z':round(d[5].z,1),'nz':round(d[7].z,2),'actor':d[9].get_actor_label() if d[9] else ''})
Path(unreal.Paths.project_dir(),'Saved','pit_exits.json').write_text(json.dumps(r),encoding='utf-8')
unreal.log('PIT_EXIT_GRID_READY')
