import unreal,json
from pathlib import Path
w=unreal.EditorLoadingAndSavingUtils.load_map('/Game/SoulCityGame/Maps/L_CityOpen')
unreal.log('PROBE_TRACE_DOC '+str(unreal.SystemLibrary.line_trace_single.__doc__))
unreal.log('PROBE_HIT_DOC '+str(unreal.HitResult.__doc__))
unreal.log('PROBE_NAV_DOC '+str(unreal.NavigationSystemV1.project_point_to_navigation.__doc__))
r=unreal.SystemLibrary.line_trace_single(w,unreal.Vector(-3088,568,400),unreal.Vector(-3088,568,-500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[],unreal.DrawDebugTrace.NONE,True)
unreal.log('PROBE_SAMPLE '+str(r))
if r is not None:
    unreal.log('PROBE_FIELDS '+str(dir(r)))
    unreal.log('PROBE_TUPLE '+str(r.to_tuple()))
