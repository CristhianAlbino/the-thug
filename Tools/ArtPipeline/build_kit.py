#!/usr/bin/env python3
"""Deterministic authored procedural prototypes; no external meshes or textures.

Source coordinates: X right, -Y front, Z up, meters. Export: glTF Y-up.
Separate overlapping garment parts are intentional prototype construction,
not a welded production topology. No face rig, IK or cloth simulation.
"""
from __future__ import annotations
import argparse, hashlib, io, json, math, struct
from pathlib import Path
import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'05_ART'/'PrototypeKit_v01'
TAU=math.tau
CONVERT=np.array([[1,0,0],[0,0,1],[0,-1,0]],dtype=float)

def smooth(t):
    t=np.clip(t,0,1); return t*t*(3-2*t)

class Mesh:
    def __init__(self,name,material):
        self.name=name; self.material=material
        self.p=[]; self.uv=[]; self.faces=[]; self.j=[]; self.w=[]
    def vertex(self,p,uv,weights):
        self.p.append(p); self.uv.append(uv)
        w=sorted([(int(j),float(v)) for j,v in weights.items() if v>1e-9],key=lambda v:-v[1])[:4]
        total=sum(v for _,v in w)
        if not w: raise ValueError('unweighted vertex')
        self.j.append([j for j,_ in w]+[0]*(4-len(w)))
        self.w.append([v/total for _,v in w]+[0.]*(4-len(w)))
        return len(self.p)-1
    def finish(self):
        p=np.asarray(self.p,dtype=float); f=np.asarray(self.faces,dtype=int)
        cross=np.cross(p[f[:,1]]-p[f[:,0]],p[f[:,2]]-p[f[:,0]])
        ok=np.linalg.norm(cross,axis=1)>1e-12
        f=f[ok]; cross=cross[ok]
        n=np.zeros_like(p)
        for k in range(3): np.add.at(n,f[:,k],cross)
        lengths=np.linalg.norm(n,axis=1)
        n[lengths<1e-12]=[0,0,1]; lengths=np.linalg.norm(n,axis=1)
        n=n/lengths[:,None]
        uv=np.asarray(self.uv,dtype=float)
        tan=np.zeros_like(p); bit=np.zeros_like(p)
        a,b,c=p[f[:,0]],p[f[:,1]],p[f[:,2]]
        du=uv[f[:,1]]-uv[f[:,0]]; dv=uv[f[:,2]]-uv[f[:,0]]
        den=du[:,0]*dv[:,1]-dv[:,0]*du[:,1]
        good=np.abs(den)>1e-12
        dt=np.zeros_like(a); db=np.zeros_like(a)
        dt[good]=((b-a)[good]*dv[good,1,None]-(c-a)[good]*du[good,1,None])/den[good,None]
        db[good]=((c-a)[good]*du[good,0,None]-(b-a)[good]*dv[good,0,None])/den[good,None]
        for k in range(3):
            np.add.at(tan,f[:,k],dt); np.add.at(bit,f[:,k],db)
        tan-=n*np.sum(n*tan,axis=1)[:,None]
        length=np.linalg.norm(tan,axis=1)
        fallback=np.cross(n,np.tile([0,0,1.],(len(n),1)))
        fallback[np.linalg.norm(fallback,axis=1)<1e-6]=[1,0,0]
        tan[length<1e-8]=fallback[length<1e-8]
        tan/=np.linalg.norm(tan,axis=1)[:,None]
        sign=np.where(np.sum(np.cross(n,tan)*bit,axis=1)<0,-1.,1.)
        self.P=(p@CONVERT.T).astype('<f4'); self.N=(n@CONVERT.T).astype('<f4')
        self.T=np.column_stack((tan@CONVERT.T,sign)).astype('<f4')
        self.UV=uv.astype('<f4'); self.F=f.astype('<u4')
        self.J=np.asarray(self.j,dtype='<u2'); self.W=np.asarray(self.w,dtype='<f4')
        return self

class Rig:
    def __init__(self): self.names=[]; self.parents=[]; self.heads=[]; self.ends=[]
    def add(self,name,head,end,parent=None):
        self.names.append(name); self.parents.append(self.id(parent) if parent else -1)
        self.heads.append(np.array(head,dtype=float)); self.ends.append(np.array(end,dtype=float))
        return len(self.names)-1
    def id(self,name): return self.names.index(name)
    def weight(self,name): return {self.id(name):1.}
    def body_weight(self,z):
        points=[(.96,'pelvis'),(1.12,'spine_01'),(1.30,'spine_02'),(1.45,'spine_03'),(1.53,'neck_01')]
        if z<=points[0][0]: return self.weight(points[0][1])
        if z>=points[-1][0]: return self.weight(points[-1][1])
        for (a,na),(b,nb) in zip(points,points[1:]):
            if a<=z<=b:
                t=float(smooth((z-a)/(b-a))); return {self.id(na):1-t,self.id(nb):t}
    def limb_weight(self,z,side,arm=False):
        # Longitudinal interpolation around a joint, not nearest-object rigid skinning.
        if arm:
            if z>1.23: return self.weight('upperarm_'+side)
            if z<1.15: return self.weight('lowerarm_'+side)
            t=float(smooth((1.23-z)/.08)); return {self.id('upperarm_'+side):1-t,self.id('lowerarm_'+side):t}
        if z>.55: return self.weight('thigh_'+side)
        if z<.43: return self.weight('calf_'+side)
        t=float(smooth((.55-z)/.12)); return {self.id('thigh_'+side):1-t,self.id('calf_'+side):t}

def make_rig(width):
    r=Rig()
    r.add('root',(0,0,0),(0,0,.1))
    r.add('pelvis',(0,0,.96),(0,0,1.08),'root')
    r.add('spine_01',(0,0,1.08),(0,0,1.24),'pelvis')
    r.add('spine_02',(0,0,1.24),(0,0,1.40),'spine_01')
    r.add('spine_03',(0,0,1.40),(0,0,1.50),'spine_02')
    r.add('neck_01',(0,0,1.50),(0,0,1.58),'spine_03')
    r.add('head',(0,0,1.58),(0,0,1.75),'neck_01')
    r.add('jaw',(0,-.045,1.62),(0,-.07,1.59),'head')
    for side,sg in [('l',1),('r',-1)]:
        sh=.215*width; el=sh+.15; wr=el+.135
        r.add('clavicle_'+side,(sg*.025,0,1.45),(sg*sh,0,1.46),'spine_03')
        r.add('upperarm_'+side,(sg*sh,0,1.46),(sg*el,0,1.19),'clavicle_'+side)
        r.add('lowerarm_'+side,(sg*el,0,1.19),(sg*wr,-.01,.98),'upperarm_'+side)
        r.add('hand_'+side,(sg*wr,-.01,.98),(sg*(wr+.04),-.01,.86),'lowerarm_'+side)
        for f in range(5):
            y=-.043+f*.017
            start=np.array([sg*(wr+.042),y,.905])
            if f==0: start=np.array([sg*(wr+.010),-.058,.942])
            parent='hand_'+side
            for k in range(3):
                end=start+np.array([sg*.013,0,-(.026 if f==0 else .030)])
                name=f'finger{f+1}_{k+1}_{side}'
                r.add(name,start,end,parent); parent=name; start=end
        hip=sg*.105*width
        r.add('thigh_'+side,(hip,0,.96),(hip,0,.49),'pelvis')
        r.add('calf_'+side,(hip,0,.49),(hip,0,.13),'thigh_'+side)
        r.add('foot_'+side,(hip,0,.13),(hip,-.14,.065),'calf_'+side)
        r.add('ball_'+side,(hip,-.14,.065),(hip,-.21,.06),'foot_'+side)
    return r

def loft(name,material,rings,weight,segs=24,gap=0.,caps=True):
    """Z-axis elliptical loft; rings=(z, halfwidth, halfdepth, centerX, centerY)."""
    m=Mesh(name,material)
    for k,(z,rx,ry,x,y) in enumerate(rings):
        for i in range(segs+1):
            a=gap+(TAU-2*gap)*i/segs
            m.vertex([x+rx*math.sin(a),y-ry*math.cos(a),z],[i/segs,1-k/(len(rings)-1)],weight(z))
    row=segs+1
    for k in range(len(rings)-1):
        for i in range(segs):
            a=k*row+i; b=a+1; c=a+row; d=c+1
            m.faces.extend([(a,b,c),(b,d,c)])
    if caps and gap==0:
        for k,flip in [(0,True),(len(rings)-1,False)]:
            z,rx,ry,x,y=rings[k]; center=m.vertex([x,y,z],[.5,.5],weight(z))
            for i in range(segs):
                a=k*row+i; b=a+1
                m.faces.append((center,b,a) if flip else (center,a,b))
    return m

def tube(name,material,points,radii,weights,segs=12):
    m=Mesh(name,material); points=np.asarray(points,dtype=float)
    for k,p in enumerate(points):
        direction=points[min(k+1,len(points)-1)]-points[max(0,k-1)]
        direction/=np.linalg.norm(direction)
        ref=np.array([0.,1.,0.]) if abs(direction[1])<.95 else np.array([1.,0.,0.])
        u=np.cross(direction,ref); u/=np.linalg.norm(u); v=np.cross(direction,u)
        radius=radii[k] if isinstance(radii,(list,tuple,np.ndarray)) else radii
        rr=radius if isinstance(radius,(tuple,list,np.ndarray)) else (radius,radius)
        for i in range(segs+1):
            a=TAU*i/segs; q=p+u*rr[0]*math.cos(a)+v*rr[1]*math.sin(a)
            m.vertex(q,[i/segs,k/(len(points)-1)],weights(k))
    row=segs+1
    for k in range(len(points)-1):
        for i in range(segs):
            a=k*row+i; b=a+1; c=a+row; d=c+1
            m.faces.extend([(a,b,c),(b,d,c)])
    for k,flip in [(0,True),(len(points)-1,False)]:
        center=m.vertex(points[k],[.5,.5],weights(k))
        for i in range(segs):
            a=k*row+i; b=a+1
            m.faces.append((center,b,a) if flip else (center,a,b))
    return m

def ellipsoid(name,material,center,radii,weight,segs=24,rows=14):
    rings=[]
    for a in np.linspace(-math.pi/2+.04,math.pi/2-.04,rows):
        rings.append((center[2]+radii[2]*math.sin(a),radii[0]*math.cos(a),radii[1]*math.cos(a),center[0],center[1]))
    return loft(name,material,rings,lambda z:weight,segs)

def box(name,material,center,size,weight):
    m=Mesh(name,material); cx,cy,cz=center; sx,sy,sz=np.asarray(size)/2
    corners=np.array([[cx+x*sx,cy+y*sy,cz+z*sz] for x,y,z in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]])
    for inds in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(3,7,6,2),(0,4,7,3),(1,2,6,5)]:
        offset=len(m.p)
        for i,uv in zip(inds,[(0,0),(1,0),(1,1),(0,1)]):m.vertex(corners[i],uv,weight)
        m.faces.extend([(offset,offset+1,offset+2),(offset,offset+2,offset+3)])
    return m

def patch(name,material,verts,weight):
    m=Mesh(name,material)
    for p,uv in zip(verts,[(0,0),(1,0),(1,1),(0,1)]):m.vertex(p,uv,weight)
    m.faces=[(0,1,2),(0,2,3)]
    return m

class Materials:
    def __init__(self): self.spec={}; self.paths={}
    def add(self,name,color,kind='cloth',rough=.8,metal=0):
        self.spec.setdefault(name,dict(color=color,kind=kind,rough=rough,metal=metal))
        return name
    def produce(self):
        target=OUT/'Textures'; target.mkdir(parents=True,exist_ok=True)
        for name,s in self.spec.items():
            n=256; y,x=np.mgrid[0:n,0:n]/n
            seed=int(hashlib.sha256(name.encode()).hexdigest()[:8],16)
            rng=np.random.default_rng(seed); noise=rng.normal(0,1,(n,n))
            weave=np.sin(TAU*x*64)*np.cos(TAU*y*64)
            broad=(np.sin(TAU*x*3+.4)*np.sin(TAU*y*2)+np.cos(TAU*(x+y)*5))*.5
            grain=.035*noise+.028*weave+.055*broad
            if s['kind']=='skin': grain=.008*noise+.012*broad
            if s['kind']=='hair': grain=.10*np.sin(TAU*x*37)+.02*noise
            if s['kind']=='metal': grain=.015*noise+.025*broad
            base=np.clip(np.array(s['color'])[None,None,:]*(1+grain[:,:,None]),0,1)
            if s['kind']=='tattoo':
                # Prototype geometry motifs authored here, not copied from reference artwork.
                ink=(np.abs(np.sin(TAU*x*4)+np.sin(TAU*y*4))<.16)|((np.abs(y-.5)<.01)|(np.abs(y-.25)<.008))
                base[ink]*=.35
            color=(base*255+.5).astype('uint8')
            rough=np.clip(s['rough']+.055*noise+.045*broad,.05,1)
            orm=np.zeros((n,n,3),dtype='uint8'); orm[:,:,0]=255; orm[:,:,1]=(rough*255).astype('uint8'); orm[:,:,2]=int(s['metal']*255)
            height=grain*.018
            dx=(np.roll(height,-1,1)-np.roll(height,1,1))*6
            dy=(np.roll(height,-1,0)-np.roll(height,1,0))*6
            normal=np.stack((-dx,-dy,np.ones_like(dx)),axis=-1)
            normal/=np.linalg.norm(normal,axis=-1)[:,:,None]
            normal=((normal*.5+.5)*255).astype('uint8')
            paths=[]
            for suffix,arr in [('BaseColor',color),('ORM',orm),('NormalGL',normal)]:
                path=target/f'T_{name}_{suffix}.png'
                encoded=io.BytesIO();Image.fromarray(arr).save(encoded,format='PNG')
                data=encoded.getvalue();assert len(data)>8 and data.startswith(b'\x89PNG\r\n\x1a\n')
                temp=path.with_suffix('.png.tmp');temp.write_bytes(data);temp.replace(path);paths.append(path)
            self.paths[name]=paths

def character(spec,mats):
    style=spec['style']; build=spec['build']; r=make_rig(build); meshes=[]
    def add(m):
        base=m.name;count=sum(x.name==base or x.name.startswith(base+'__') for x in meshes)
        if count:m.name=base+'__'+str(count+1)
        meshes.append(m)
    sk=mats.add('Skin_'+spec['id'].split('_')[0],spec['skin'],'skin',.65)
    hair=mats.add('Hair_'+spec['id'].split('_')[0],spec['hair'],'hair',.88)
    dark=mats.add('Cloth_Charcoal',[.065,.062,.056],'cloth',.88)
    leather=mats.add('Leather_Black',[.040,.032,.025],'cloth',.68)
    brown=mats.add('Coat_Brown',[.115,.088,.065],'cloth',.90)
    olive=mats.add('Cloth_Olive',[.12,.135,.095],'cloth',.91)
    wine=mats.add('Cloth_Wine',[.22,.045,.054],'cloth',.9)
    ivory=mats.add('Coat_Ivory',[.61,.57,.48],'cloth',.91)
    brass=mats.add('Metal_Brass',[.36,.26,.095],'metal',.43,.8)
    eyes=mats.add('Eyes_Ivory',[.66,.63,.56],'skin',.25)
    iris=mats.add('Eyes_Brown',[.055,.040,.025],'skin',.35)
    pink=mats.add('Hair_DustyPink',[.39,.12,.15],'hair',.92)
    lips=mats.add('Lips_'+spec['id'].split('_')[0],np.array(spec['skin'])*[.80,.64,.61],'skin',.65)
    torso=[(.94,.14*build,.085,0,0),(1.03,.15*build,.09,0,0),(1.13,.147*build,.096,0,0),(1.30,.20*build,.108,0,0),(1.43,.22*build,.095,0,0),(1.48,.15*build,.074,0,0)]
    add(loft('Torso_Shirt',olive if style in ['thomas','bart'] else dark,torso,r.body_weight,segs=32))
    add(loft('Neck',sk,[(1.46,.048,.044,0,0),(1.54,.041,.038,0,0),(1.60,.048,.039,0,0)],lambda z:r.weight('neck_01'),segs=24))
    hscale=1.06 if style=='thomas' else .97 if style=='harley' else 1
    # Custom head profile rather than a single sphere; cheeks, jaw and brow have distinct rings.
    head=[(1.562,.022,.028,0,-.012),(1.584,.043,.050,0,-.007),(1.613,.064,.060,0,0),(1.65,.074,.064,0,0),(1.688,.071,.063,0,.005),(1.726,.065,.060,0,.005),(1.751,.044,.045,0,.005),(1.765,.008,.010,0,.005)]
    head=[(1.65+(z-1.65)*hscale,w*hscale,d*hscale,x,y) for z,w,d,x,y in head]
    add(loft('Head',sk,head,lambda z:r.weight('head'),segs=40))
    for side,sg in [('l',1),('r',-1)]:
        add(ellipsoid('Ear_'+side,sk,(sg*.074*hscale,.003,1.657),(.015,.018,.027),r.weight('head'),16,10))
        add(ellipsoid('Eye_'+side,eyes,(sg*.028,-.063,1.681),(.020,.007,.009 if style!='bart' else .007),r.weight('head'),20,10))
        add(ellipsoid('Iris_'+side,iris,(sg*.028,-.0701,1.682),(.007,.0018,.007),r.weight('head'),16,8))
        add(tube('Brow_'+side,hair,[(sg*.009,-.065,1.699),(sg*.027,-.067,1.702),(sg*.047,-.063,1.699)],[.003,.004,.002],lambda k:r.weight('head'),8))
    add(ellipsoid('Nose_Bridge',sk,(0,-.063,1.667),(.010,.013,.027),r.weight('head'),20,10))
    add(ellipsoid('Nose_Tip',sk,(0,-.081,1.647),(.017,.016,.010),r.weight('head'),20,10))
    add(tube('Upper_Lip',lips,[(-.024,-.062,1.617),(0,-.073,1.619),(.024,-.062,1.617)],.0025,lambda k:r.weight('head'),8))
    add(tube('Lower_Lip',lips,[(-.022,-.062,1.615),(0,-.069,1.613),(.022,-.062,1.615)],.002,lambda k:r.weight('jaw'),8))
    hairrings=[(1.694,.073,.064,0,.004),(1.718,.072,.066,0,.004),(1.751,.052,.052,0,.004),(1.778,.008,.010,0,.004)]
    add(loft('Hair_Cap',hair,hairrings,lambda z:r.weight('head'),32))
    for side,sg in [('l',1),('r',-1)]:
        x=sg*.105*build
        add(loft('Pants_'+side,dark,[(.12,.052,.049,x,0),(.22,.055,.050,x,0),(.43,.069,.055,x,0),(.49,.072,.066,x,0),(.68,.089*build,.082,x,0),(.91,.095*build,.087,x,0),(1.00,.070*build,.070,x,0)],lambda z,s=side:r.limb_weight(z,s),24))
        add(ellipsoid('Boot_'+side,leather,(x,-.052,.081),(.068,.145,.072),r.weight('foot_'+side),24,12))
        add(box('Sole_'+side,dark,(x,-.051,.025),(.14,.286,.029),r.weight('foot_'+side)))
        armstart=r.heads[r.id('upperarm_'+side)]; elbow=r.heads[r.id('lowerarm_'+side)]; wrist=r.heads[r.id('hand_'+side)]
        armpts=[armstart,armstart*.65+elbow*.35,elbow,elbow*.65+wrist*.35,wrist]
        sleeve=sk if style=='thomas' else brown if style in ['joe','bart'] else ivory if style=='jack_punitive' else leather if style=='harley' else dark
        radii=[.071*build,.078*build,.058*build,.062*build,.033*build]
        add(ellipsoid('Shoulder_'+side,sleeve,armstart,(.082*build,.070*build,.079*build),r.weight('upperarm_'+side),24,14))
        add(tube('Arm_'+side,sleeve,armpts,radii,lambda k,s=side,p=armpts:r.limb_weight(p[k][2],s,True),20))
        if style=='thomas':
            tat=mats.add('Skin_Thomas_Tattoo',spec['skin'],'tattoo',.69)
            add(tube('Tattoo_Arm_'+side,tat,armpts[1:],np.array(radii[1:])*1.015,lambda k,s=side,p=armpts:r.limb_weight(p[k+1][2],s,True),20))
            add(tube('Tshirt_Sleeve_'+side,olive,armpts[:2],[.078*build,.084*build],lambda k,s=side:r.weight('upperarm_'+s),20))
        if style=='harley':
            cuff=[np.array(armpts[-1])+[0,0,.043],armpts[-1]]
            add(tube('Wine_Cuff_'+side,wine,cuff,[.041*build,.035*build],lambda k,s=side:r.weight('lowerarm_'+s),20))
        add(ellipsoid('Palm_'+side,sk,(wrist[0]+sg*.030,-.01,.924),(.035,.046,.057),r.weight('hand_'+side),20,12))
        for f in range(5):
            for k in range(3):
                bn=f'finger{f+1}_{k+1}_{side}'; b=r.id(bn)
                add(tube(bn,sk,[r.heads[b],r.ends[b]],[.008-k*.001,.007-k*.001],lambda i,b=b:{b:1},8))
        # Authored belt/garment detail; not an atlas or a production skin topology.
        add(box('Trouser_Pocket_'+side,dark,(x+sg*.045,.084,.785),(.10,.012,.14),r.weight('thigh_'+side)))
    add(loft('Belt',leather,[(.972,.153*build,.09,0,0),(.997,.153*build,.09,0,0)],lambda z:r.weight('pelvis'),32))
    add(box('Buckle',brass,(0,-.099,.985),(.033,.012,.028),r.weight('pelvis')))
    if style in ['joe','jack_punitive']:
        coat= brown if style=='joe' else ivory
        rings=[(.60,.205*build,.13,0,0),(.80,.18*build,.115,0,0),(1.0,.167*build,.104,0,0),(1.18,.175*build,.110,0,0),(1.32,.214*build,.122,0,0),(1.45,.234*build,.103,0,0),(1.48,.158*build,.082,0,0)]
        add(loft('Long_Coat',coat,rings,r.body_weight,40,gap=.35,caps=False))
        for sg in [-1,1]:
            add(patch('Lapel_'+str(sg),coat,[(sg*.04,-.11,1.42),(sg*.15*build,-.09,1.46),(sg*.18*build,-.115,1.29),(sg*.09,-.129,1.23)],r.weight('spine_03')))
            add(box('Coat_Pocket_'+str(sg),coat,(sg*.15*build,-.086,.88),(.075,.013,.085),r.weight('pelvis')))
            for z in [.91,1.07,1.23]:
                add(ellipsoid('Button',brass,(sg*.060,-.115,z),(.005,.003,.005),r.body_weight(z),12,6))
        if style=='joe':
            add(loft('Turtleneck',dark,[(1.46,.059,.053,0,0),(1.55,.051,.048,0,0)],lambda z:r.weight('neck_01'),24))
            add(ellipsoid('Newsboy_Cap',brown,(0,.008,1.752),(.084,.082,.036),r.weight('head'),32,12))
            add(ellipsoid('Cap_Brim',brown,(0,-.072,1.733),(.076,.052,.007),r.weight('head'),32,8))
            for sg in [-1,1]:
                add(loft('Nape_Hair',hair,[(1.60,.038,.022,sg*.043,.05),(1.64,.044,.029,sg*.043,.05),(1.70,.038,.031,sg*.043,.04)],lambda z:r.weight('head'),16))
        else:
            add(loft('Wine_Waistcoat',wine,[(1.00,.156*build,.103,0,0),(1.18,.164*build,.113,0,0),(1.38,.192*build,.116,0,0),(1.45,.127*build,.09,0,0)],r.body_weight,32,gap=.035,caps=False))
            add(patch('Tie',olive,[(-.013,-.126,1.44),(.013,-.126,1.44),(.019,-.126,1.20),(0,-.13,1.17)],r.weight('spine_03')))
            chain=[(.09+.07*math.sin(t),-.126,1.045-.07*math.sin(t/2)) for t in np.linspace(0,math.pi,14)]
            add(tube('Pocket_Chain',brass,chain,.002,lambda k:r.weight('pelvis'),6))
    elif style=='thomas':
        add(loft('Utility_Vest',leather,[(1.0,.175*build,.104,0,0),(1.18,.168*build,.108,0,0),(1.32,.214*build,.119,0,0),(1.43,.226*build,.104,0,0),(1.46,.17*build,.075,0,0)],r.body_weight,32,gap=.4,caps=False))
        for sg in [-1,1]:
            for z in [1.10,1.29]:add(box('Vest_Pocket',leather,(sg*.14*build,-.116,z),(.09,.025,.09),r.body_weight(z)))
        beard=[(1.36,.010,.010,0,-.071),(1.43,.04,.026,0,-.078),(1.53,.078,.031,0,-.076),(1.59,.070,.027,0,-.067),(1.63,.05,.014,0,-.068)]
        add(loft('Viking_Beard',hair,beard,lambda z:r.weight('head'),32))
        for t in range(18):
            a=math.pi*(t/17-.5)
            sx=.066*math.sin(a); sy=.057*math.cos(a)
            pts=[(sx,sy,1.75),(sx*1.12,sy*1.18,1.68),(sx*1.22,sy*1.35+.025,1.57),(sx*1.35,sy*1.4+.045,1.43)]
            add(tube('Dread_'+str(t),hair,pts,[.010,.011,.010,.007],lambda k:r.weight('head'),8))
        for t in range(2):
            pts=[(.009*math.sin(k*.9+t*math.pi),-.107,1.55-k*.028) for k in range(7)]
            add(tube('Beard_Braid',hair,pts,.009,lambda k:r.weight('head'),8))
        add(box('Watch',leather,(r.heads[r.id('hand_l')][0],-.015,1.018),(.048,.069,.025),r.weight('lowerarm_l')))
    else:
        coat=leather if style=='harley' else brown if style=='bart' else dark
        add(loft('Short_Jacket',coat,[(.975,.16*build,.105,0,0),(1.13,.164*build,.11,0,0),(1.32,.211*build,.12,0,0),(1.45,.231*build,.101,0,0),(1.475,.153*build,.075,0,0)],r.body_weight,32,gap=.30,caps=False))
        for sg in [-1,1]:
            add(patch('Collar',coat,[(sg*.036,-.075,1.48),(sg*.12*build,-.082,1.46),(sg*.15*build,-.118,1.33),(sg*.07,-.13,1.36)],r.weight('spine_03')))
            add(box('Jacket_Pocket',coat,(sg*.14*build,-.088,1.23),(.075,.016,.083),r.body_weight(1.23)))
        if style in ['harley','bart']:
            hoodmat=wine if style=='harley' else coat
            add(loft('Hood',hoodmat,[(1.40,.072,.055,0,.063),(1.47,.087,.060,0,.067),(1.51,.067,.046,0,.062)],r.body_weight,24,gap=1.65,caps=False))
        if style=='harley':
            add(loft('Wine_Lining',wine,[(1.0,.145*build,.103,0,0),(1.33,.178*build,.114,0,0),(1.43,.169*build,.097,0,0)],r.body_weight,24,gap=.15,caps=False))
            add(loft('Choker',leather,[(1.523,.044,.041,0,0),(1.535,.044,.041,0,0)],lambda z:r.weight('neck_01'),24))
            for side,sg in [('l',1),('r',-1)]:
                for t in range(6):
                    a=t*.5
                    pts=[(sg*.055,.02,1.75),(sg*(.09+.006*t),.05,1.72),(sg*(.092+.008*t),.07,1.61),(sg*(.095+.008*t),.08,1.49)]
                    add(tube('Loose_Hair_'+side+str(t),hair,pts,[.009,.009,.007,.005],lambda k:r.weight('head'),8))
                    add(tube('Pink_Tip_'+side+str(t),pink,pts[-2:],[.0075,.0055],lambda k:r.weight('head'),8))
        else:
            # Reproducible tuft layout preserves Jack's head across his two outfits.
            rng=np.random.default_rng(314 if style.startswith('jack') else 719)
            for t in range(16):
                x=rng.uniform(-.055,.055); y=rng.uniform(-.045,.025)
                pts=[(x,y,1.741),(x-.015,y-.012,1.777),(x-.033,y-.025,1.761)]
                add(tube('Hair_Tuft_'+str(t),hair,pts,[.009,.008,.002],lambda k:r.weight('head'),8))
        if style=='bart':
            for sg in [-1,1]:
                pts=[(sg*(.030+.026*math.cos(a)),-.076,1.681+.018*math.sin(a)) for a in np.linspace(0,TAU,21)]
                add(tube('Glasses_Frame',leather,pts,.0024,lambda k:r.weight('head'),6))
                add(tube('Glasses_Temple',leather,[(sg*.055,-.074,1.684),(sg*.071,-.01,1.683),(sg*.074,.02,1.675)],.002,lambda k:r.weight('head'),6))
            add(tube('Glasses_Bridge',leather,[(-.007,-.078,1.685),(0,-.081,1.687),(.007,-.078,1.685)],.002,lambda k:r.weight('head'),6))
    if style=='jack_punitive':
        # Same dark wavy hair as standard Jack, with identical RNG and geometry.
        rng=np.random.default_rng(314)
        for t in range(16):
            x=rng.uniform(-.055,.055); y=rng.uniform(-.045,.025)
            pts=[(x,y,1.741),(x-.015,y-.012,1.777),(x-.033,y-.025,1.761)]
            add(tube('Hair_Tuft_'+str(t),hair,pts,[.009,.008,.002],lambda k:r.weight('head'),8))
    # Anatomical base height is 1.785m including hair/cap; scale actual geometry
    # so requested heights are achieved, rather than merely stored as labels.
    top=max(max(p[2] for p in m.p) for m in meshes)
    bottom=min(min(p[2] for p in m.p) for m in meshes)
    scale=spec['height']/(top-bottom)
    for m in meshes:
        m.p=[(np.array(p)-[0,0,bottom])*scale for p in m.p]
        m.finish()
    for attr in ['heads','ends']:
        setattr(r,attr,[(p-[0,0,bottom])*scale for p in getattr(r,attr)])
    r.heads[0]=np.zeros(3)
    return meshes,r

def quat(axis,angle):
    v=np.asarray(axis,dtype=float); v/=np.linalg.norm(v)
    return [*(v*math.sin(angle/2)),math.cos(angle/2)]

def animations(r):
    clips=[]
    for name,duration in [('Idle_Breath',3.),('Walk_InPlace',1.2),('Crouch_Hold',2.),('Look_Over_Shoulder',2.4),('Help_Reach',2.4),('Stop_Signal',2.)]:
        times=np.linspace(0,duration,int(duration*24)+1,dtype='<f4'); channels=[]
        def rot(bone,axis,fn):
            channels.append((r.id(bone),'rotation',np.array([quat(axis,fn(float(t))) for t in times],dtype='<f4')))
        def trans(bone,fn):
            k=r.id(bone); parent=r.parents[k]
            base=CONVERT@(r.heads[k]-(r.heads[parent] if parent>=0 else np.zeros(3)))
            channels.append((k,'translation',np.array([base+fn(float(t)) for t in times],dtype='<f4')))
        phase=lambda t:TAU*t/duration
        if name=='Idle_Breath':
            rot('spine_02',(1,0,0),lambda t:.012*math.sin(phase(t)))
            rot('head',(0,1,0),lambda t:.015*math.sin(phase(t)))
        elif name=='Walk_InPlace':
            for side,sg in [('l',1),('r',-1)]:
                rot('thigh_'+side,(1,0,0),lambda t,s=sg:s*.32*math.sin(phase(t)))
                rot('calf_'+side,(1,0,0),lambda t,s=sg:-.45*max(0,s*math.sin(phase(t))))
                rot('upperarm_'+side,(1,0,0),lambda t,s=sg:-s*.18*math.sin(phase(t)))
                rot('foot_'+side,(1,0,0),lambda t,s=sg:.10*max(0,s*math.sin(phase(t))))
            trans('pelvis',lambda t:np.array([0,.012*math.sin(phase(t)*2),0]))
        elif name=='Crouch_Hold':
            ease=lambda t:float(smooth(min(t/.45,(duration-t)/.45)))
            upper=float(r.heads[r.id('thigh_l')][2]-r.heads[r.id('calf_l')][2])
            lower=float(r.heads[r.id('calf_l')][2]-r.heads[r.id('foot_l')][2])
            def crouch_offset(t):
                e=ease(t);a=.65*e;b=-.55*e
                return np.array([0,-upper*(1-math.cos(a))-lower*(1-math.cos(b)),upper*math.sin(a)+lower*math.sin(b)])
            trans('pelvis',crouch_offset)
            for side in ['l','r']:
                rot('thigh_'+side,(1,0,0),lambda t:.65*ease(t))
                rot('calf_'+side,(1,0,0),lambda t:-1.20*ease(t))
                rot('foot_'+side,(1,0,0),lambda t:.55*ease(t))
            rot('spine_01',(1,0,0),lambda t:-.20*ease(t))
        elif name=='Look_Over_Shoulder':
            rot('head',(0,1,0),lambda t:.8*math.sin(math.pi*t/duration)**2)
            rot('spine_03',(0,1,0),lambda t:.15*math.sin(math.pi*t/duration)**2)
        elif name=='Help_Reach':
            e=lambda t:math.sin(math.pi*t/duration)**2
            rot('spine_01',(1,0,0),lambda t:-.25*e(t))
            rot('upperarm_l',(1,0,0),lambda t:-.85*e(t))
            rot('lowerarm_l',(1,0,0),lambda t:-.35*e(t))
        elif name=='Stop_Signal':
            e=lambda t:math.sin(math.pi*t/duration)**2
            rot('upperarm_r',(1,0,0),lambda t:-.8*e(t))
            rot('lowerarm_r',(1,0,0),lambda t:-.5*e(t))
        clips.append((name,times,channels))
    return clips

class GLB:
    def __init__(self):
        self.j={'asset':{'version':'2.0','generator':'The Thug authored procedural ArtPipeline v01'},'scene':0,'scenes':[{'nodes':[]}],'nodes':[],'meshes':[],'buffers':[{'byteLength':0}],'bufferViews':[],'accessors':[],'materials':[],'textures':[],'images':[],'samplers':[{'magFilter':9729,'minFilter':9987,'wrapS':10497,'wrapT':10497}]}
        self.bin=bytearray()
    def view(self,data,target=None):
        while len(self.bin)%4:self.bin+=b'\0'
        idx=len(self.j['bufferViews']); v={'buffer':0,'byteOffset':len(self.bin),'byteLength':len(data)}
        if target:v['target']=target
        self.j['bufferViews'].append(v); self.bin+=data; return idx
    def acc(self,data,kind,component=5126,target=None,bounds=False):
        a=np.asarray(data); idx=len(self.j['accessors']); v=self.view(a.tobytes(),target)
        count=len(a) if kind!='SCALAR' else a.size
        desc={'bufferView':v,'componentType':component,'count':count,'type':kind}
        if bounds:
            desc['min']=a.reshape(count,-1).min(0).astype(float).tolist(); desc['max']=a.reshape(count,-1).max(0).astype(float).tolist()
        self.j['accessors'].append(desc); return idx
    def image(self,path):
        data=path.read_bytes();assert len(data)>8 and data.startswith(b'\x89PNG\r\n\x1a\n'),path
        view=self.view(data); idx=len(self.j['images']); self.j['images'].append({'bufferView':view,'mimeType':'image/png','name':path.name})
        ti=len(self.j['textures']); self.j['textures'].append({'sampler':0,'source':idx}); return ti
    def material(self,name,s,paths):
        tex=[self.image(p) for p in paths]
        self.j['materials'].append({'name':name,'pbrMetallicRoughness':{'baseColorTexture':{'index':tex[0]},'metallicRoughnessTexture':{'index':tex[1]},'metallicFactor':1,'roughnessFactor':1},'normalTexture':{'index':tex[2],'scale':.35},'doubleSided':True,'extras':{'stage':'prototype','procedural_source':'build_kit.py'}})
        return len(self.j['materials'])-1
    def mesh(self,m,material,skin=True):
        attrs={'POSITION':self.acc(m.P,'VEC3',target=34962,bounds=True),'NORMAL':self.acc(m.N,'VEC3',target=34962),'TANGENT':self.acc(m.T,'VEC4',target=34962),'TEXCOORD_0':self.acc(m.UV,'VEC2',target=34962)}
        if skin:
            attrs['JOINTS_0']=self.acc(m.J,'VEC4',5123,34962); attrs['WEIGHTS_0']=self.acc(m.W,'VEC4',target=34962)
        primitive={'attributes':attrs,'indices':self.acc(m.F.ravel(),'SCALAR',5125,34963),'material':material,'mode':4}
        self.j['meshes'].append({'name':m.name,'primitives':[primitive]})
        return len(self.j['meshes'])-1
    def save(self,path):
        while len(self.bin)%4:self.bin+=b'\0'
        self.j['buffers'][0]['byteLength']=len(self.bin)
        raw=json.dumps(self.j,separators=(',',':'),ensure_ascii=False).encode()
        while len(raw)%4:raw+=b' '
        blob=struct.pack('<III',0x46546c67,2,12+8+len(raw)+8+len(self.bin))+struct.pack('<II',len(raw),0x4e4f534a)+raw+struct.pack('<II',len(self.bin),0x004e4942)+self.bin
        path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(blob)

def export_character(spec,meshes,r,mats):
    g=GLB(); used=sorted(set(m.material for m in meshes)); mi={n:g.material(n,mats.spec[n],mats.paths[n]) for n in used}
    for k,name in enumerate(r.names):
        parent=r.parents[k]; loc=CONVERT@(r.heads[k]-(r.heads[parent] if parent>=0 else np.zeros(3)))
        g.j['nodes'].append({'name':name,'translation':loc.tolist(),'rotation':[0,0,0,1],'extras':{'bone_end':(CONVERT@r.ends[k]).tolist()}})
    for k,p in enumerate(r.parents):
        if p>=0:g.j['nodes'][p].setdefault('children',[]).append(k)
    g.j['scenes'][0]['nodes'].append(0)
    bind=[]
    for h in r.heads:
        mat=np.eye(4); mat[:3,3]=-CONVERT@h; bind.append(mat.T.ravel())
    g.j['skins']=[{'name':'SK_TheThug_Prototype_v01','joints':list(range(len(r.names))),'skeleton':0,'inverseBindMatrices':g.acc(np.asarray(bind,dtype='<f4'),'MAT4')}]
    for m in meshes:
        mesh=g.mesh(m,mi[m.material]); node=len(g.j['nodes']); g.j['nodes'].append({'name':m.name,'mesh':mesh,'skin':0});g.j['scenes'][0]['nodes'].append(node)
    g.j['animations']=[]
    for name,times,channels in animations(r):
        a={'name':name,'samplers':[],'channels':[]}; ta=g.acc(times,'SCALAR',bounds=True)
        for bone,path,data in channels:
            oi=g.acc(data,'VEC4' if path=='rotation' else 'VEC3')
            a['samplers'].append({'input':ta,'output':oi,'interpolation':'LINEAR'})
            a['channels'].append({'sampler':len(a['samplers'])-1,'target':{'node':bone,'path':path}})
        g.j['animations'].append(a)
    g.j['asset']['extras']={'stage':'prototype_not_final','character':spec['id'],'target_height_m':spec['height'],'height_source':spec['height_source'],'forward':'+Z','up':'+Y','rig':'custom_not_unreal_mannequin','limitations':['separate_overlapping_meshes','no_IK','no_facial_rig','no_cloth_physics','no_LOD','animations_not_gameplay_timing_approved']}
    path=OUT/'Characters'/f'SK_{spec["id"]}_v01.glb'; g.save(path)
    return {'id':spec['id'],'file':str(path.relative_to(ROOT)),'height_m':spec['height'],'height_source':spec['height_source'],'bones':len(r.names),'meshes':len(meshes),'vertices':sum(len(m.P) for m in meshes),'triangles':sum(len(m.F) for m in meshes),'animations':[a['name'] for a in g.j['animations']],'status':'prototype_unreviewed','sha256':hashlib.sha256(path.read_bytes()).hexdigest()}

def props(mats):
    steel=mats.add('Metal_Steel',[.30,.32,.30],'metal',.57,.85)
    painted=mats.add('Paint_Hospital',[.30,.37,.32],'cloth',.87)
    rubber=mats.add('Rubber_Dark',[.035,.038,.033],'cloth',.95)
    fabric=mats.add('Mattress_Grey',[.38,.37,.32],'cloth',.98)
    # Independent named moving pivots. No collisions, state machine or lock logic is fabricated.
    assets=[]; w={0:1}
    door=[box('Frame_Left',painted,(-.515,0,1.1),(.09,.17,2.2),w),box('Frame_Right',painted,(.515,0,1.1),(.09,.17,2.2),w),box('Frame_Top',painted,(0,0,2.175),(1.12,.17,.10),w)]
    leaf=[box('Door_Leaf',painted,(.45,0,1.055),(.9,.060,2.11),w),box('Handle',steel,(.80,-.065,1.03),(.10,.055,.018),w)]
    assets.append(('Hospital_Door',door,leaf,(-.45,0,0)))
    cabinet=[box('Cabinet',painted,(0,0,.35),(.48,.22,.70),w),box('Back',steel,(0,.07,.35),(.44,.02,.66),w)]
    panel=[box('Panel',steel,(.24,-.125,0),(.47,.026,.68),w)]
    assets.append(('Electrical_Cabinet',cabinet,panel,(-.235,0,.35)))
    bed=[box('Mattress',fabric,(0,0,.66),(.78,1.95,.12),w),box('Bed_Frame',steel,(0,0,.565),(.82,2,.065),w)]
    for x in [-.34,.34]:
        for y in [-.85,.85]:
            bed.append(box('Leg',steel,(x,y,.29),(.038,.038,.54),w))
            bed.append(ellipsoid('Caster',rubber,(x,y,.05),(.045,.026,.045),w,16,8))
    for y in [-.98,.98]:
        for x in [-.38,.38]:bed.append(box('Rail_Post',steel,(x,y,.76),(.025,.025,.40),w))
        bed.append(box('End_Rail',steel,(0,y,.945),(.78,.025,.025),w))
    assets.append(('Hospital_Bed',bed,[],(0,0,0)))
    return assets

def export_prop(name,static,moving,pivot,mats):
    g=GLB(); used=sorted(set(m.material for m in static+moving)); mi={n:g.material(n,mats.spec[n],mats.paths[n]) for n in used}
    root=len(g.j['nodes']); g.j['nodes'].append({'name':name,'children':[]});g.j['scenes'][0]['nodes']=[root]
    for m in static:
        m.finish(); node=len(g.j['nodes']); g.j['nodes'].append({'name':m.name,'mesh':g.mesh(m,mi[m.material],False)});g.j['nodes'][root]['children'].append(node)
    if moving:
        moving_root=len(g.j['nodes']); g.j['nodes'].append({'name':'Hinge_Pivot','translation':(CONVERT@np.array(pivot)).tolist(),'children':[]});g.j['nodes'][root]['children'].append(moving_root)
        for m in moving:
            m.finish(); node=len(g.j['nodes']);g.j['nodes'].append({'name':m.name,'mesh':g.mesh(m,mi[m.material],False)});g.j['nodes'][moving_root]['children'].append(node)
        time=g.acc(np.array([0,1],dtype='<f4'),'SCALAR',bounds=True)
        q=g.acc(np.array([quat((0,1,0),0),quat((0,1,0),math.pi/2)],dtype='<f4'),'VEC4')
        g.j['animations']=[{'name':'Open_90deg_Reference','samplers':[{'input':time,'output':q,'interpolation':'LINEAR'}],'channels':[{'sampler':0,'target':{'node':moving_root,'path':'rotation'}}]}]
    g.j['asset']['extras']={'stage':'prototype_not_final','units':'meters','origin':'floor_center' if name!='Electrical_Cabinet' else 'lower_center','logic':'visual_only_no_runtime_events'}
    path=OUT/'Props'/f'SM_{name}_v01.glb';g.save(path)
    return {'id':name,'file':str(path.relative_to(ROOT)),'status':'prototype_unreviewed','sha256':hashlib.sha256(path.read_bytes()).hexdigest()}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--only',default='');args=ap.parse_args()
    config=json.loads((Path(__file__).parent/'characters.json').read_text())
    mats=Materials();data=[]
    for spec in config['characters']:
        if not args.only or args.only==spec['id']:data.append((spec,*character(spec,mats)))
    propdata=props(mats) if not args.only else []
    mats.produce()
    chars=[export_character(s,meshes,r,mats) for s,meshes,r in data]
    propindex=[export_prop(*p,mats) for p in propdata]
    manifest={'version':1,'stage':'procedural_prototype_not_final','voices_produced':0,'characters':chars,'props':propindex,'material_count':len(mats.spec),'texture_maps':len(mats.spec)*3,'source':'Tools/ArtPipeline/build_kit.py','deterministic_seed':True,'author_visual_specs':'Tools/ArtPipeline/characters.json'}
    (OUT/'asset_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(manifest,ensure_ascii=False,indent=2))

if __name__=='__main__':main()
