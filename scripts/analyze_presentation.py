#!/usr/bin/env python3
"""Compare presentation costs and slow-frame duration, never infer console FPS from host runs."""
import argparse,json,re
from pathlib import Path
from analyze_stutter import analyze
p=argparse.ArgumentParser(description=__doc__);p.add_argument('logs',nargs='+',type=Path);a=p.parse_args()
reports=[]
for path in a.logs:
 text=path.read_text(errors='replace');runs=list(re.finditer(r'NPS64 Vulkan Game Test [0-9.]+;',text))
 if runs:text=text[runs[-1].start():]
 groups={}
 for tag,weight in [('vulkan-times','loops'),('gpu-present-times','loops'),('scanout','samples')]:
  rows=[dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in text.splitlines() if line.startswith('['+tag+']')]
  total=sum(int(r.get(weight,0)) for r in rows)
  means={}
  for key in sorted({key for r in rows for key in r if key.endswith('_ms')}):
   if total:means[key]=sum(float(r.get(key,0))*int(r.get(weight,0)) for r in rows)/total
  groups[tag]={'samples':total,'weighted_ms':means}
 reports.append({'file':str(path),'costs':groups,'cadence':analyze(text),'policies':[line for line in text.splitlines() if line.startswith(('[rdp-policy]','[gpu-present] enabled='))]})
print(json.dumps({'reports':reports,'notes':['Match ROM, scene, resolution, filter and duration for hardware A/B.','Full-frame cadence and longest slow episodes matter more than average FPS.','GPU fence times include preceding queued work; do not add inclusive timers.','Host software Vulkan tests establish correctness, not PS4 performance.']},indent=2,ensure_ascii=False))
