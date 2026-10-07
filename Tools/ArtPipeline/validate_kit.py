"""Independent exported-file checks, including actual posed vertex movement."""
import hashlib,json
from pathlib import Path
import numpy as np
from glb_io import Asset
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'05_ART'/'PrototypeKit_v01'

def validate(path):
    a=Asset(path);j=a.j;checks=0;verts=0;triangles=0
    assert j['buffers'][0]['byteLength']==len(a.bin);checks+=1
    for v in j['bufferViews']:
        assert v.get('byteOffset',0)%4==0
        assert v.get('byteOffset',0)+v['byteLength']<=len(a.bin);checks+=2
    geom=a.geometry();mins=[];maxs=[]
    for name,p,n,uv,f,mat in geom:
        assert np.isfinite(p).all() and np.isfinite(n).all() and np.isfinite(uv).all()
        assert np.allclose(np.linalg.norm(n,axis=1),1,atol=1e-5)
        assert f.min()>=0 and f.max()<len(p)
        areas=np.linalg.norm(np.cross(p[f[:,1]]-p[f[:,0]],p[f[:,2]]-p[f[:,0]]),axis=1)
        assert areas.min()>1e-12,(name,'degenerate faces')
        assert len(np.unique(uv,axis=0))>3,(name,'missing actual UVs')
        assert 0<=mat<len(j['materials'])
        mins.append(p.min(0));maxs.append(p.max(0));checks+=6;verts+=len(p);triangles+=len(f)
    for node in j['nodes']:
        if 'skin' not in node:continue
        skin=j['skins'][node['skin']]
        for pr in j['meshes'][node['mesh']]['primitives']:
            attrs=pr['attributes'];pp=a.acc(attrs['POSITION']);ww=a.acc(attrs['WEIGHTS_0']);jj=a.acc(attrs['JOINTS_0'])
            assert len(pp)==len(ww)==len(jj)
            assert (ww>=0).all() and np.allclose(ww.sum(1),1,atol=2e-7)
            assert jj.max()<len(skin['joints'])
            # Rest-pose readback must equal exported positions, not translated twice.
            current=next(g[1] for g in geom if g[0]==node['name'])
            assert np.allclose(current,pp,atol=2e-6);checks+=4
    clips=[]
    for i,anim in enumerate(j.get('animations',[])):
        duration=0
        for s in anim['samplers']:
            t=a.acc(s['input'])[:,0];v=a.acc(s['output']);duration=max(duration,float(t[-1]))
            assert (np.diff(t)>0).all() and np.isfinite(v).all() and len(t)==len(v)
            if v.shape[1]==4:assert np.allclose(np.linalg.norm(v,axis=1),1,atol=2e-6)
            checks+=2
        maxmovement=0
        for time in [duration*.17,duration*.37,duration*.61]:
            posed=a.geometry(i,time)
            for base,pose in zip(geom,posed):
                assert np.isfinite(pose[1]).all()
                maxmovement=max(maxmovement,float(np.max(np.linalg.norm(base[1]-pose[1],axis=1))));checks+=1
        assert maxmovement>1e-5,(path.name,anim['name'],'no real vertex deformation')
        driftmax=0
        if anim['name']=='Crouch_Hold':
            rest=a.globals()
            for side in ['l','r']:
                index=next(k for k,n in enumerate(j['nodes']) if n['name']=='foot_'+side)
                for t in np.linspace(0,duration,81):
                    drift=float(np.linalg.norm(a.globals(i,float(t))[index][:3,3]-rest[index][:3,3]))
                    driftmax=max(driftmax,drift)
                    assert drift<.001 # <1mm sampled FK/interpolation error; no IK claim.
                    checks+=1
        record={'name':anim['name'],'duration_s':round(duration,4),'max_sampled_vertex_displacement_m':round(maxmovement,5)}
        if anim['name']=='Crouch_Hold':record['max_sampled_ankle_drift_m']=round(driftmax,7)
        clips.append(record)
    lo=np.min(mins,0);hi=np.max(maxs,0)
    h=j['asset'].get('extras',{}).get('target_height_m')
    if h is not None:assert abs(hi[1]-lo[1]-h)<2e-6;assert abs(lo[1])<2e-6;checks+=2
    return {'file':str(path.relative_to(ROOT)),'passed':True,'checks':checks,'vertices':verts,'triangles':triangles,'bounds_m':[lo.tolist(),hi.tolist()],'animations':clips,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}

def main():
    reports=[validate(p) for p in sorted(OUT.rglob('*.glb'))]
    # Jack identity invariants are compared on actual exported head/face/hair positions.
    std=OUT/'Characters'/'SK_Jack_Standard_v01.glb';pun=OUT/'Characters'/'SK_Jack_Punitive_v01.glb'
    comparisons=[]
    if std.exists() and pun.exists():
        g1=Asset(std).geometry();g2=Asset(pun).geometry()
        for prefix in ['Head','Ear_','Eye_','Iris_','Brow_','Nose_','Upper_Lip','Lower_Lip','Hair_']:
            left=[(n,p) for n,p,*_ in g1 if n.startswith(prefix)];right=[(n,p) for n,p,*_ in g2 if n.startswith(prefix)]
            assert len(left)==len(right)
            for (n,p),(n2,p2) in zip(left,right):assert n==n2 and np.array_equal(p,p2),n
            comparisons.append(prefix)
    result={'passed':True,'scope':'file_structure_numeric_skinning_and_animation; no Blender/Unreal import or official Khronos validator run','assets':reports,'jack_identity_parts_compared':comparisons,'not_validated':['artistic likeness to concept sheets','gameplay timing','foot IK or floor contacts','clothing collision','UE import','performance on target notebook','final retopology']}
    (OUT/'validation_report.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'passed':True,'assets':len(reports),'checks':sum(r['checks'] for r in reports),'jack_identity':bool(comparisons)},indent=2))

if __name__=='__main__':main()
