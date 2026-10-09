"""Compare paired same-VI CPU/Vulkan game areas. Diagnostic, not parity PASS."""
import argparse, json
from pathlib import Path
from PIL import Image, ImageChops
p=argparse.ArgumentParser()
p.add_argument("directory",type=Path)
a=p.parse_args()
rows=[]
for cpu in sorted(a.directory.glob("frame-*-cpu.ppm"),key=lambda p:int(p.name.split("-")[1])):
    gpu=cpu.with_name(cpu.name.replace("-cpu.ppm","-vulkan.ppm"))
    x=Image.open(cpu).convert("RGB"); y=Image.open(gpu).convert("RGB")
    if x.size!=y.size: raise ValueError("Different capture dimensions")
    diff=ImageChops.difference(x,y)
    values=list(diff.getdata())
    error=sum(sum(v) for v in values)/(len(values)*3)
    bad=sum(max(v)>16 for v in values)/len(values)
    stem=cpu.name.removesuffix("-cpu.ppm")
    view=Image.new("RGB",(x.width*3,x.height))
    view.paste(x,(0,0));view.paste(y,(x.width,0));view.paste(diff,(2*x.width,0))
    view.save(a.directory/(stem+"-full.png"))
    view.resize((1440,360)).save(a.directory/(stem+"-comparison.png"))
    metadata=cpu.with_name(stem+".json")
    rows.append(dict(frame=stem,mean_absolute_error=error,pixels_over_16_pct=bad*100,metadata=json.loads(metadata.read_text()) if metadata.exists() else None))
if not rows: raise SystemExit("No paired frames; capture did not run")
(a.directory/"report.json").write_text(json.dumps(rows,indent=2))
print(json.dumps(rows,indent=2))
