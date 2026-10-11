"""Summarize a case-6 repeat-10 capture without executing captured code."""
from pathlib import Path
import argparse,json,struct
parser=argparse.ArgumentParser()
parser.add_argument('capture',type=Path)
parser.add_argument('output',type=Path)
args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
sizes=[8192,4096,1024,524288,65536]
buffers=[(args.capture/f'tile-fail-case5-buffer{i}.bin').read_bytes() for i in range(5)]
assert all(len(b)==1048576 for b in buffers)
words=[struct.unpack('<262144I',b) for b in buffers]
assert words[1][:4]==(0,0,640,480), 'Expected upper-left quadrant fixture'
assert words[0][7]&128, 'Expected captured FILL/COPY primitive'
mask=words[3];coarse=words[4];rows=[];mismatches=0;coarse_mismatches=0
for y in range(32):
    active=[];unexpected=[]
    for x in range(40):
        tile=y*128+x;expected=int(y<15 and x<=20)
        if mask[tile*8]:active.append(x)
        if mask[tile*8]!=expected:unexpected.append(x)
        mismatches+=sum(mask[tile*8+group]!=(expected if group==0 else 0) for group in range(8))
        coarse_mismatches+=coarse[tile]!=expected
    rows.append({'y':y,'active_x':active,'unexpected_x':unexpected})
report={'fixture':'case6 repeat10, one FILL/COPY primitive, upper-left quadrant',
 'scissor':words[1][:4],'mask_mismatches_visible_region':mismatches,
 'coarse_mismatches_visible_region':coarse_mismatches,
 'allocation_guards_intact':[all(w==0xcdcdcdcd for w in words[i][sizes[i]//4:]) for i in range(5)],
 'rows':rows}
(args.output/'capture-analysis.json').write_text(json.dumps(report,indent=2)+'\n')
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="790" height="240" viewBox="0 0 790 240">',
 '<rect width="790" height="240" fill="#eef1f5"/>']
for panel,label in enumerate(('Expected mask','PS4 captured mask','Differences')):
    left=12+panel*262
    svg.append(f'<text x="{left}" y="20" font-family="sans-serif" font-size="14">{label}</text>')
    for y in range(32):
        for x in range(40):
            expected=int(y<15 and x<=20);actual=bool(mask[(y*128+x)*8])
            active=(bool(expected),actual,actual!=bool(expected))[panel]
            color=('#1976d2' if panel<2 else '#d32f2f') if active else '#ffffff'
            svg.append(f'<rect x="{left+x*6}" y="{32+y*6}" width="5" height="5" fill="{color}"/>')
svg.append('</svg>')
(args.output/'tile-mask-comparison.svg').write_text('\n'.join(svg))
print(json.dumps({k:v for k,v in report.items() if k!='rows'},indent=2))
