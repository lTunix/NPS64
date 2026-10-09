"""Add only new lane functions; preserve every CPU46 function's counters."""
from pathlib import Path
import subprocess,sys,tempfile
base,output,*raw=sys.argv[1:]
assert raw, 'Pass instrumentation profiles'
names={'nps64_lane_remainder','nps64_edgewalker_fast','nps64_edgewalker_dispatch','nps64_core_game_profile'}
with tempfile.TemporaryDirectory(dir='/work/build') as directory:
    directory=Path(directory); all_text=directory/'all.txt'; selected=directory/'lanes.txt'
    subprocess.run(['llvm-profdata','merge','--text',*raw,'-o',str(all_text)],check=True)
    blocks=[b for b in all_text.read_text().split('\n\n') if b and b.splitlines()[0].split(':')[-1] in names]
    found={b.splitlines()[0].split(':')[-1] for b in blocks}
    assert names==found, f'Missing lane functions: {names-found}'
    selected.write_text('\n\n'.join(blocks)+'\n\n')
    subprocess.run(['llvm-profdata','merge',base,str(selected),'-o',output],check=True)
print('PASS: added four lane functions, CPU46 function counters preserved')
