"""Read back exported GLB bytes and evaluate actual linear blend skinning."""
import io,json,struct
import numpy as np
from PIL import Image

SIZES={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
TYPES={5121:'u1',5123:'<u2',5125:'<u4',5126:'<f4'}

class Asset:
    def __init__(self,path):
        self.path=path;self.raw=path.read_bytes()
        magic,version,length=struct.unpack_from('<III',self.raw)
        assert magic==0x46546c67 and version==2 and length==len(self.raw)
        n,kind=struct.unpack_from('<II',self.raw,12); assert kind==0x4e4f534a
        self.j=json.loads(self.raw[20:20+n]); o=20+n
        count,kind=struct.unpack_from('<II',self.raw,o);assert kind==0x004e4942
        self.bin=self.raw[o+8:o+8+count];assert o+8+count==length
        self.parents=[-1]*len(self.j['nodes'])
        for k,node in enumerate(self.j['nodes']):
            for ch in node.get('children',[]):
                assert self.parents[ch]==-1;self.parents[ch]=k
        self.textures=[]
        for image in self.j['images']:
            v=self.j['bufferViews'][image['bufferView']];start=v.get('byteOffset',0)
            self.textures.append(np.array(Image.open(io.BytesIO(self.bin[start:start+v['byteLength']])).convert('RGB'))/255)
    def acc(self,index):
        a=self.j['accessors'][index];v=self.j['bufferViews'][a['bufferView']]
        dtype=np.dtype(TYPES[a['componentType']]);cols=SIZES[a['type']]
        start=v.get('byteOffset',0)+a.get('byteOffset',0)
        stride=v.get('byteStride',cols*dtype.itemsize)
        last=start+(a['count']-1)*stride+cols*dtype.itemsize
        assert last<=v.get('byteOffset',0)+v['byteLength'] and last<=len(self.bin)
        return np.ndarray((a['count'],cols),dtype=dtype,buffer=self.bin,offset=start,strides=(stride,dtype.itemsize)).copy()
    def globals(self,clip=None,t=0):
        ts=[np.array(n.get('translation',[0,0,0]),float) for n in self.j['nodes']]
        qs=[np.array(n.get('rotation',[0,0,0,1]),float) for n in self.j['nodes']]
        if clip is not None:
            anim=self.j['animations'][clip]
            for c in anim['channels']:
                s=anim['samplers'][c['sampler']]; times=self.acc(s['input'])[:,0]; vals=self.acc(s['output'])
                k=min(max(0,int(np.searchsorted(times,t,side='right')-1)),len(times)-2)
                f=np.clip((t-times[k])/(times[k+1]-times[k]),0,1)
                a=vals[k].astype(float);b=vals[k+1].astype(float);target=c['target']
                if target['path']=='rotation':
                    if np.dot(a,b)<0:b=-b
                    q=a*(1-f)+b*f;qs[target['node']]=q/np.linalg.norm(q)
                elif target['path']=='translation':ts[target['node']]=a*(1-f)+b*f
        result=[None]*len(ts)
        def calc(k):
            if result[k] is None:
                x,y,z,w=qs[k];m=np.eye(4)
                m[:3,:3]=[[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]]
                m[:3,:3]*=np.array(self.j['nodes'][k].get('scale',[1,1,1]))[None,:]
                m[:3,3]=ts[k];p=self.parents[k]
                result[k]=calc(p)@m if p>=0 else m
            return result[k]
        for k in range(len(ts)):calc(k)
        return result
    def geometry(self,clip=None,t=0):
        globals=self.globals(clip,t);result=[]
        for k,node in enumerate(self.j['nodes']):
            if 'mesh' not in node:continue
            for pr in self.j['meshes'][node['mesh']]['primitives']:
                attrs=pr['attributes'];p=self.acc(attrs['POSITION']);n=self.acc(attrs['NORMAL']);uv=self.acc(attrs['TEXCOORD_0'])
                f=self.acc(pr['indices']).reshape(-1,3)
                if 'skin' in node:
                    skin=self.j['skins'][node['skin']];ibm=self.acc(skin['inverseBindMatrices']).reshape(-1,4,4).transpose(0,2,1)
                    mats=np.array([globals[j] for j in skin['joints']])@ibm
                    jj=self.acc(attrs['JOINTS_0']);ww=self.acc(attrs['WEIGHTS_0'])
                    pm=np.sum(mats[jj]*ww[:,:,None,None],axis=1)
                    p=np.einsum('nij,nj->ni',pm,np.column_stack((p,np.ones(len(p)))))[:,:3]
                    n=np.einsum('nij,nj->ni',pm[:,:3,:3],n)
                else:
                    p=(globals[k]@np.column_stack((p,np.ones(len(p)))).T).T[:,:3]
                    n=(globals[k][:3,:3]@n.T).T
                n/=np.linalg.norm(n,axis=1)[:,None]
                result.append((node['name'],p,n,uv,f,pr['material']))
        return result
