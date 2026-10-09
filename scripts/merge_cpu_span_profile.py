"""Train new span specializations without changing established PGO counters."""
from pathlib import Path
import subprocess
import sys
import tempfile

base, output, *raw = sys.argv[1:]
assert raw, 'Pass instrumentation profiles'
names = {'nps64_fbread2_16', 'nps64_fbwrite_16',
         'nps64_tcdiv_persp', 'nps64_tcdiv_nopersp'}
for span in ('complete', 'notexelnext', 'notexel1'):
    for prefix in ('fast', 'flat', 'dispatch'):
        names.add('nps64_' + prefix + '_render_spans_2cycle_' + span)
with tempfile.TemporaryDirectory(dir='/work/build') as directory:
    directory = Path(directory)
    all_text = directory / 'all.txt'
    selected = directory / 'spans.txt'
    subprocess.run(['llvm-profdata', 'merge', '--text', *raw,
                    '-o', str(all_text)], check=True)
    blocks = [block for block in all_text.read_text().split('\n\n')
              if block and block.splitlines()[0].split(':')[-1] in names]
    found = {block.splitlines()[0].split(':')[-1] for block in blocks}
    assert names == found, f'Missing span functions: {names - found}'
    selected.write_text('\n\n'.join(blocks) + '\n\n')
    subprocess.run(['llvm-profdata', 'merge', base, str(selected),
                    '-o', output], check=True)
    def records(profile):
        target = directory / 'check.txt'
        subprocess.run(['llvm-profdata', 'merge', '--text', profile,
                        '-o', str(target)], check=True)
        return {block.splitlines()[0]: block for block in
                target.read_text().split('\n\n') if block}
    previous, merged = records(base), records(output)
    assert all(merged.get(name) == block for name, block in previous.items()), \
        'Established profile counters changed'
print('PASS: 13 new span functions added; previous function counters preserved')
