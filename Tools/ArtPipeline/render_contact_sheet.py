"""Compare actual exported character heights at a shared pixels/meters scale."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from glb_io import Asset
from render_preview import render,FONT,OUT

def main():
    font=ImageFont.truetype(FONT,22);small=ImageFont.truetype(FONT,14)
    order=[('Joe','Joe'),('Harley','Harley'),('Jack_Standard','Jack · padrão'),('Jack_Punitive','Jack · Punitivo'),('Thomas','Thomas'),('Bartolomeu','Bartolomeu')]
    canvas=Image.new('RGB',(1200,1540),(209,206,199));d=ImageDraw.Draw(canvas)
    d.text((24,22),'THE THUG · BASES 3D DE PROTÓTIPO v01',font=font,fill=(35,40,35))
    d.text((24,56),'GLBs reais · mesma escala entre personagens · aparência simplificada',font=small,fill=(60,65,60))
    for i,(name,label) in enumerate(order):
        a=Asset(OUT/'Characters'/f'SK_{name}_v01.glb');img=render(a,-.22,400,630,fixed_scale=240)
        x=(i%3)*400;y=100+(i//3)*710;canvas.paste(img,(x,y))
        h=a.j['asset']['extras']['target_height_m'];source=a.j['asset']['extras']['height_source']
        d.text((x+24,y+646),label+' · '+f'{h:.2f}'.replace('.',',')+' m',font=font,fill=(35,40,35))
        if source!='author_confirmed':d.text((x+24,y+676),'Altura provisória deste protótipo',font=small,fill=(60,65,60))
    canvas.save(OUT/'Previews'/'TheThug_Elenco3D_v01.png')
    props=Image.new('RGB',(1200,620),(209,206,199));d=ImageDraw.Draw(props)
    d.text((22,20),'THE THUG · OBJETOS 3D DE PROTÓTIPO v01',font=font,fill=(35,40,35))
    for i,(name,label) in enumerate([('Hospital_Door','Porta'),('Electrical_Cabinet','Quadro elétrico'),('Hospital_Bed','Cama hospitalar')]):
        a=Asset(OUT/'Props'/f'SM_{name}_v01.glb');props.paste(render(a,-.52,400,460),(400*i,70))
        d.text((i*400+30,560),label,font=font,fill=(35,40,35))
    props.save(OUT/'Previews'/'TheThug_Props3D_v01.png');print('GLB contact sheets rendered')

if __name__=='__main__':main()
