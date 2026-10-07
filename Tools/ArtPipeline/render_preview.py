"""Preview rendered from actual exported GLB geometry, not AI concept art.
Software painter preview: diffuse lighting, sampled base color; not a PBR bake.
"""
import argparse,math
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from glb_io import Asset
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'05_ART'/'PrototypeKit_v01'
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'

def render(a,yaw=0,width=420,height=760,clip=None,time=0,zoom=1,fixed_scale=None):
    image=Image.new('RGB',(width,height),(209,206,199));draw=ImageDraw.Draw(image)
    geom=a.geometry(clip,time);bounds=np.concatenate([g[1] for g in a.geometry()])
    h=bounds[:,1].max()-bounds[:,1].min();span=max(np.ptp(bounds[:,0]),np.ptp(bounds[:,2]))
    scale=min(height*.84/h,width*.88/span)*zoom
    if fixed_scale is not None:scale=fixed_scale
    c=math.cos(yaw);s=math.sin(yaw);r=np.array([[c,0,s],[0,1,0],[-s,0,c]])
    light=np.array([-.35,.75,.6]);light/=np.linalg.norm(light)
    baseline=height*.87 if fixed_scale is not None else (height+h*scale)/2
    draw.ellipse((width*.21,baseline-6,width*.79,baseline+18),fill=(185,183,177))
    pixels=np.array(image);zbuffer=np.full((height,width),-np.inf)
    for name,p,n,uv,f,mi in geom:
        p=p@r.T;n=n@r.T
        coords=np.column_stack((p[:,0]*scale+width/2,baseline-p[:,1]*scale))
        material=a.j['materials'][mi];ti=material['pbrMetallicRoughness']['baseColorTexture']['index']
        tex=a.textures[a.j['textures'][ti]['source']];u=uv%1
        col=tex[(u[:,1]*tex.shape[0]).astype(int)%tex.shape[0],(u[:,0]*tex.shape[1]).astype(int)%tex.shape[1]]
        intensity=.48+.52*np.maximum(n@light,0)
        col=np.clip(col*intensity[:,None],0,1)**(1/1.8)
        for inds in f:
            tri=coords[inds];x0=max(0,int(np.floor(tri[:,0].min())));x1=min(width-1,int(np.ceil(tri[:,0].max())))
            y0=max(0,int(np.floor(tri[:,1].min())));y1=min(height-1,int(np.ceil(tri[:,1].max())))
            if x1<x0 or y1<y0:continue
            a0,b,c=tri;den=(b[1]-c[1])*(a0[0]-c[0])+(c[0]-b[0])*(a0[1]-c[1])
            if abs(den)<1e-7:continue
            yy,xx=np.mgrid[y0:y1+1,x0:x1+1];xx=xx+.5;yy=yy+.5
            w0=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/den
            w1=((c[1]-a0[1])*(xx-c[0])+(a0[0]-c[0])*(yy-c[1]))/den;w2=1-w0-w1
            zz=w0*p[inds[0],2]+w1*p[inds[1],2]+w2*p[inds[2],2]
            mask=(w0>=-1e-6)&(w1>=-1e-6)&(w2>=-1e-6)&(zz>zbuffer[y0:y1+1,x0:x1+1])
            if not mask.any():continue
            rgb=w0[:,:,None]*col[inds[0]]+w1[:,:,None]*col[inds[1]]+w2[:,:,None]*col[inds[2]]
            pixels[y0:y1+1,x0:x1+1][mask]=(np.clip(rgb[mask],0,1)*255).astype('uint8')
            zbuffer[y0:y1+1,x0:x1+1][mask]=zz[mask]
    return Image.fromarray(pixels)

def sheet(path):
    a=Asset(path);canvas=Image.new('RGB',(1680,980),(209,206,199));draw=ImageDraw.Draw(canvas)
    label=path.stem.replace('SK_','').replace('_v01','').replace('_',' ')
    draw.text((28,20),label+' · THE THUG',font=ImageFont.truetype(FONT,29),fill=(35,37,35))
    draw.text((28,60),'MALHA 3D PROCEDURAL · PROTÓTIPO v01 · render do GLB exportado',font=ImageFont.truetype(FONT,16),fill=(70,71,68))
    for i,(yaw,label) in enumerate([(0,'FRENTE'),(math.pi/2,'PERFIL'),(math.pi,'COSTAS'),(-.45,'TRÊS QUARTOS')]):
        img=render(a,yaw,420,800);canvas.paste(img,(i*420,100));draw.text((i*420+140,922),label,font=ImageFont.truetype(FONT,17),fill=(50,52,49))
    dest=OUT/'Previews'/f'{path.stem}_turnaround.png';canvas.save(dest);return dest

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--only',default='');ap.add_argument('--animate',action='store_true');args=ap.parse_args()
    for path in sorted((OUT/'Characters').glob('*.glb')):
        if args.only and args.only not in path.name:continue
        print(sheet(path),flush=True)
        if args.animate:
            a=Asset(path);frames=[render(a,-.3,360,620,1,1.2*k/24) for k in range(24)]
            frames[0].save(OUT/'Previews'/f'{path.stem}_walk.gif',save_all=True,append_images=frames[1:],duration=50,loop=0)

if __name__=='__main__':main()
