// Test the parser and transform functions actually used by the offline viewer.
// This is not a WebGL/browser rendering test.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),cp=require('node:child_process');
const root=path.resolve(__dirname,'../..');
const template=fs.readFileSync(path.join(__dirname,'viewer_template.html'),'utf8');
const whole=template.split('<script>')[1].split('</script>')[0].replace('__MODELS__','[]');
new vm.Script(whole); // Syntax for the complete application, including event handlers.
const pure=whole.split('try{if(!gl)')[0];
const context=vm.createContext({document:{getElementById:()=>({getContext:()=>null})},console,TextDecoder,Uint8Array,Uint16Array,Uint32Array,Float32Array,DataView,ArrayBuffer});
vm.runInContext(pure,context);
const python=`import sys,json\nfrom pathlib import Path\nsys.path.insert(0,'Tools/ArtPipeline')\nfrom glb_io import Asset\nout=[]\nfor p in sorted(Path('05_ART/PrototypeKit_v01').rglob('*.glb')):\n a=Asset(p);samples=[(None,0)]\n for k,an in enumerate(a.j.get('animations',[])):\n  duration=max(float(a.acc(s['input'])[-1,0]) for s in an['samplers'])\n  samples.extend((k,duration*t) for t in [0,.17,.41,.69,1])\n out.append({'path':str(p),'samples':[{'clip':k,'time':t,'globals':[m.T.ravel().tolist() for m in a.globals(k,t)]} for k,t in samples]})\nprint(json.dumps(out))`;
const fixtures=JSON.parse(cp.execFileSync('python3',['-c',python],{cwd:root,maxBuffer:10*1024*1024}).toString());
let poses=0,nodes=0,maxError=0;
for(const asset of fixtures){
 const buffer=fs.readFileSync(path.join(root,asset.path));context.raw=buffer.buffer.slice(buffer.byteOffset,buffer.byteOffset+buffer.byteLength);
 vm.runInContext('state=parse(raw)',context);
 const count=vm.runInContext('state.j.nodes.length',context);
 for(const sample of asset.samples){
  context.clipArg=sample.clip===null?-1:sample.clip;context.timeArg=sample.time;
  const actual=vm.runInContext('clip=clipArg; matrices(state,timeArg)',context);
  if(actual.length!==count||actual.length!==sample.globals.length)throw Error('node count');
  for(let i=0;i<actual.length;i++)for(let k=0;k<16;k++){
   const delta=Math.abs(actual[i][k]-sample.globals[i][k]);maxError=Math.max(maxError,delta);
   if(!Number.isFinite(actual[i][k])||delta>3e-6)throw Error(asset.path+' matrix mismatch node '+i+' at '+sample.time);
  }
  poses++;nodes+=actual.length;
 }
}
const report={passed:true,assets:fixtures.length,pose_samples:poses,node_matrix_comparisons:nodes,max_absolute_matrix_error:maxError,scope:'complete_JS_syntax_and_actual_viewer_GLTF_parser_transform_math',browser_WebGL_rendering_tested:false};
fs.writeFileSync(path.join(root,'05_ART/PrototypeKit_v01/viewer_validation.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report,null,2));
