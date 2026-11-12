"""Generate original plasma-rifle and mechanical reload samples, then import them."""
import unreal,wave,math,random,struct
from pathlib import Path
root=Path(unreal.Paths.project_dir())
folder=root/'SourceAssets'/'Audio'
folder.mkdir(parents=True,exist_ok=True)
rate=48000
def write(name,values):
    for i in range(len(values)):
        values[i]*=min(1.,max(0.,(len(values)-1-i)/(rate*.015)))
    peak=max(max(abs(v) for v in values),.001)
    pcm=[int(max(-1,min(1,v/peak*.9))*32767) for v in values]
    path=folder/(name+'.wav')
    with wave.open(str(path),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate)
        f.writeframes(struct.pack('<'+'h'*len(pcm),*pcm))
    task=unreal.AssetImportTask()
    task.set_editor_property('filename',str(path))
    task.set_editor_property('destination_path','/Game/SoulCityGame/Audio')
    task.set_editor_property('destination_name',name)
    task.set_editor_property('automated',True)
    task.set_editor_property('replace_existing',True)
    task.set_editor_property('save',True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    assert task.get_editor_property('imported_object_paths'),name
for variant in range(3):
    rng=random.Random(718+variant);values=[];low=0.;phase=0.
    for i in range(int(rate*.24)):
        t=i/rate;noise=rng.uniform(-1,1);low=.86*low+.14*noise
        phase+=2*math.pi*(80+450*math.exp(-t/0.012))/rate
        attack=min(1,t/.0007)
        crack=(noise-low)*.65*math.exp(-t/.017)
        body=math.sin(phase)*.42*math.exp(-t/.045)+low*.5*math.exp(-t/.085)
        metal=math.sin(2*math.pi*1950*t)*.08*math.exp(-abs(t-.025)/.012)
        values.append(attack*(crack+body+metal))
    write('S_RifleShot0'+str(variant+1),values)
rng=random.Random(443);values=[]
for i in range(int(rate*1.65)):
    t=i/rate;v=0.
    for when,strength in [(.07,.7),(.43,.55),(1.07,.7),(1.43,1.)]:
        d=t-when
        if 0<=d<.14:
            v+=strength*(rng.uniform(-1,1)*.7+math.sin(2*math.pi*380*d)*.3)*math.exp(-d/.022)
    if .72<t<.92: v+=rng.uniform(-1,1)*.075*math.sin((t-.72)/.2*math.pi)
    values.append(v)
write('S_RifleReload',values)
exec(compile((root/'Scripts'/'build_optic_materials.py').read_text(encoding='utf-8-sig'),'build_optic_materials.py','exec'))
unreal.log('CITY_COMBAT_AUDIO_READY')
