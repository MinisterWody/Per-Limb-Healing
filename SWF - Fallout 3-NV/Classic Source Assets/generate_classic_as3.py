from pathlib import Path
import re, xml.etree.ElementTree as ET
ROOT = Path(__file__).resolve().parent
SRC = ROOT / 'svg'
OUT = ROOT.parent / 'Source/reconstructed-as/ClassicPaperDoll.as' 
# FNV/VUI+ paperdoll hierarchy from the supplied paperdoll.xml.
# The user-supplied SVGs preserve the original paper-doll image canvases at 8x
# resolution.  Do NOT stretch each SVG to the XML width/height: those traits are
# menu layout/interaction metrics while the image itself is independently zoomed.
# Preserve the source registration by converting SVG coordinates uniformly 1/8,
# then apply only the XML parent-relative origins below.
ORIGINS={
    'head': (0.0,0.0),
    'face': (14.5,30.0), # y = (92-64)/2 + 16 from the XML
    'torso': (-14.0,86.0),
    'left_arm': (76.0,86.5),   # torso + (90, 0.5)
    'right_arm': (-102.0,81.0),# torso + (-88, -5)
    'left_leg': (33.5,172.0),  # torso + (47.5, 86)
    'right_leg': (-50.5,168.5),# torso + (-36.5, 82.5)
}
SVG_TO_NATIVE=0.125

def tokenize(d):
    return re.findall(r'[MLCZ]|[-+]?(?:\d*\.\d+|\d+\.?)(?:[eE][-+]?\d+)?', d)

def parse_path(d):
    toks=tokenize(d); i=0; cmd=None; out=[]; start=None; cur=None
    while i<len(toks):
        if toks[i] in 'MLCZ': cmd=toks[i]; i+=1
        if cmd=='M':
            x=float(toks[i]); y=float(toks[i+1]); i+=2
            out.append(('M',(x,y))); cur=(x,y); start=(x,y)
            # SVG: subsequent coordinate pairs after M are implicit L, but these files use explicit commands.
            cmd=None
        elif cmd=='L':
            x=float(toks[i]); y=float(toks[i+1]); i+=2
            out.append(('L',(x,y))); cur=(x,y); cmd=None
        elif cmd=='C':
            vals=tuple(float(toks[i+j]) for j in range(6)); i+=6
            out.append(('C',vals)); cur=(vals[4],vals[5]); cmd=None
        elif cmd=='Z':
            out.append(('Z',())); cur=start; cmd=None
        elif cmd is None:
            if i<len(toks) and toks[i] not in 'MLCZ':
                raise ValueError('unexpected implicit data')
        else: raise ValueError(cmd)
    return out

def asset(name, metric_key):
    e=ET.fromstring((SRC/(name+'.svg')).read_text())
    vb=list(map(float,e.attrib['viewBox'].split()))
    path=next(iter(e)).attrib['d']
    ops=parse_path(path)
    x,y=ORIGINS[metric_key]
    sx=SVG_TO_NATIVE; sy=SVG_TO_NATIVE
    commands=[]; data=[]; sx0=sy0=None; cur=None
    for op,v in ops:
        if op=='M':
            px=x+(v[0]-vb[0])*sx; py=y+(v[1]-vb[1])*sy
            commands.append(1); data += [px,py]; sx0,sy0=px,py; cur=(v[0],v[1])
        elif op=='L':
            px=x+(v[0]-vb[0])*sx; py=y+(v[1]-vb[1])*sy
            commands.append(2); data += [px,py]; cur=(v[0],v[1])
        elif op=='C':
            # Autodesk Scaleform GFx 4.x supports GraphicsPath CURVE_TO
            # (quadratic) but not Flash's later CUBIC_CURVE_TO command.
            # Approximate each SVG cubic with three midpoint-matched quadratic
            # segments; at the final ~0.8x paper-doll size the error is
            # sub-pixel while remaining on the well-supported GFx path API.
            p0=cur
            p1=(v[0],v[1]); p2=(v[2],v[3]); p3=(v[4],v[5])
            def cubic(t):
                u=1.0-t
                return (u*u*u*p0[0]+3*u*u*t*p1[0]+3*u*t*t*p2[0]+t*t*t*p3[0],
                        u*u*u*p0[1]+3*u*u*t*p1[1]+3*u*t*t*p2[1]+t*t*t*p3[1])
            for seg in range(3):
                t0=seg/3.0; t1=(seg+1)/3.0; tm=(t0+t1)/2.0
                a=cubic(t0); m=cubic(tm); dpt=cubic(t1)
                q=(2*m[0]-0.5*(a[0]+dpt[0]), 2*m[1]-0.5*(a[1]+dpt[1]))
                vals=[x+(q[0]-vb[0])*sx,y+(q[1]-vb[1])*sy,
                      x+(dpt[0]-vb[0])*sx,y+(dpt[1]-vb[1])*sy]
                commands.append(3); data += vals
            cur=p3
        elif op=='Z':
            # drawPath has no CLOSE command; explicitly return to the subpath start.
            commands.append(2); data += [sx0,sy0]
    return commands,data

def fmt_num(n):
    s=f'{n:.6f}'.rstrip('0').rstrip('.')
    if s=='-0': s='0'
    return s

def wrap(vals, per=18, indent='         '):
    chunks=[]
    for i in range(0,len(vals),per): chunks.append(indent+','.join(vals[i:i+per]))
    return ',\n'.join(chunks)
assets=[
 ('HeadNormal','head','head'),('HeadBroken','head_broken','head'),
 ('TorsoNormal','torso','torso'),('TorsoBroken','torso_broken','torso'),
 ('LeftArmNormal','left_arm','left_arm'),('LeftArmBroken','left_arm_broken','left_arm'),
 ('RightArmNormal','right_arm','right_arm'),('RightArmBroken','right_arm_broken','right_arm'),
 ('LeftLegNormal','left_leg','left_leg'),('LeftLegBroken','left_leg_broken','left_leg'),
 ('RightLegNormal','right_leg','right_leg'),('RightLegBroken','right_leg_broken','right_leg'),
 ('Face00','face_00','face'),('Face01','face_01','face'),('Face02','face_02','face'),
 ('Face03','face_03','face'),('Face04','face_04','face'),('Face10','face_10','face'),
]
methods=[]
for m,n,k in assets:
    c,d=asset(n,k)
    cs=[str(x) for x in c]; ds=[fmt_num(x) for x in d]
    methods.append(f'''      private static function Draw{m}(target:Sprite) : void\n      {{\n         var commands:Vector.<int> = new <int>[\n{wrap(cs)}\n         ];\n         var data:Vector.<Number> = new <Number>[\n{wrap(ds)}\n         ];\n         target.graphics.beginFill(16777215,1);\n         target.graphics.drawPath(commands,data,GraphicsPathWinding.EVEN_ODD);\n         target.graphics.endFill();\n      }}\n''')
# The current ClassicPaperDoll contains hand-maintained interaction and face
# behavior in addition to generated vector geometry. Regenerate only the Draw*
# methods so running this tool cannot replace current behavior with an older
# template.
existing = OUT.read_text()
marker = '      private static function DrawHeadNormal'
try:
    draw_start = existing.index(marker)
except ValueError as exc:
    raise RuntimeError('ClassicPaperDoll.as is missing the generated Draw* section') from exc

class_close = '   }\n}\n'
if not existing.endswith(class_close):
    raise RuntimeError('Unexpected ClassicPaperDoll.as class ending')

text = existing[:draw_start] + ''.join(methods) + class_close
OUT.write_text(text)
print('updated generated geometry in', OUT, 'bytes', OUT.stat().st_size)
