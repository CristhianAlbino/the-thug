import base64,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'05_ART'/'PrototypeKit_v01'
config=json.loads((Path(__file__).parent/'characters.json').read_text())
labels={s['id']:s['label'] for s in config['characters']}
labels.update({'Hospital_Door':'Porta do hospital','Electrical_Cabinet':'Quadro elétrico','Hospital_Bed':'Cama hospitalar'})
models=[]
for path in sorted(OUT.rglob('*.glb')):
    key=path.stem.removeprefix('SK_').removeprefix('SM_').removesuffix('_v01')
    models.append({'label':labels[key],'filename':path.name,'data':base64.b64encode(path.read_bytes()).decode()})
# Default to protagonist rather than alphabetical order.
models.sort(key=lambda m:(m['label']!='Joe',m['filename']))
template=(Path(__file__).parent/'viewer_template.html').read_text()
assert template.count('__MODELS__')==1
html=template.replace('__MODELS__',json.dumps(models,ensure_ascii=False,separators=(',',':')))
dest=OUT/'TheThug_Kit3D_v01.html';dest.write_text(html)
print(dest)
