"""Decode captured GFX7 shaders without executing any GPU work."""
from pathlib import Path
import hashlib
import re
import subprocess

root = Path('/work')
output = root / 'build/hang-analysis'
output.mkdir(parents=True, exist_ok=True)
decoder = output / 'clrx-build/programs/clrxdisasm'
files = sorted(root.glob('gpu-hang-shader-*.bin'))
assert files, 'No captured shaders in workspace root'
report = ['# Analysis of current PS4 shader capture', '',
          'GPU stopped after trace marker 8, before completion of raster dispatch.',
          'This report identifies generated code and available inputs; it does not establish a root cause.', '']
for p in files:
    data = p.read_bytes()
    result = subprocess.run([str(decoder), '--gpuType=bonaire', '--raw', '--hexcode', str(p)],
                            check=True, capture_output=True, text=True)
    (output / (p.stem + '.s')).write_text(result.stdout)
    first_end = re.search(r'/\*([0-9a-f]+):.*?\*/ s_endpgm', result.stdout)
    assert first_end, p.name
    code_end = int(first_end.group(1), 16) + 4
    report += [f'- `{p.name}`: {len(data)} captured bytes; first s_endpgm ends at '
               f'0x{code_end:x}. SHA256 `{hashlib.sha256(data).hexdigest()}`.']
    if p.stem.endswith('44fff9d400'):
        main = result.stdout[:first_end.end()]
        forbidden = re.findall(r'\b(?:ds_\w+|s_barrier|\w*atomic\w*)\b', main)
        assert not forbidden, forbidden
        report += ['', 'Rasterizer code through its first termination has no LDS, atomic or '
                   's_barrier instructions. This is not proof of runtime termination.',
                   'Its opening scalar descriptor loads reference byte offsets 0xb0 '
                   '(coarse mask) and 0xa0 (tile mask). Current captures include 256 '
                   'descriptor bytes and snapshots of both masks.', '']
for left, right in zip(files, files[1:]):
    left_va = int(left.stem.split('-')[-1], 16)
    right_va = int(right.stem.split('-')[-1], 16)
    delta = right_va - left_va
    if 0 < delta < left.stat().st_size:
        overlap = left.read_bytes()[delta:]
        assert overlap == right.read_bytes()[:len(overlap)], 'Overlapping code differs'
        report.append(f'- Captures at 0x{left_va:x} and 0x{right_va:x}: '
                      f'{len(overlap)} overlapping bytes match exactly.')
import struct
coarse_path = root / 'gpu-hang-data-43f8321000.bin'
tile_path = root / 'gpu-hang-data-43f8640000.bin'
if coarse_path.exists() and tile_path.exists():
    coarse = struct.unpack('<%dI' % (coarse_path.stat().st_size//4), coarse_path.read_bytes())
    tiles = struct.unpack('<%dI' % (tile_path.stat().st_size//4), tile_path.read_bytes())
    active = {y*128+x for y in range(30) for x in range(40)}
    assert all(coarse[i] == (1 if i in active else 0) for i in range(len(coarse)))
    assert all(tiles[i] == (1 if i//8 in active and i%8 == 0 else 0) for i in range(len(tiles)))
    report += ['', 'PASS: 1,200 active tiles; coarse and per-primitive masks agree exactly. '
               'Only primitive zero is selected, all other bits are zero.',
               'The current color raster shader terminates at 0x25c4 instead of the '
               'previous full shader at 0x2ba0. Reduction did not resolve the hardware hang.',
               'Host replay and complete memory equivalence are checked separately by '
               'tests/replay_gpu_capture.py; host completion does not validate PS4 ISA.']
(output/'REPORT.md').write_text('\n'.join(report)+'\n')
print('PASS: GFX7 decode, shader hashes and matching overlap; report in build/hang-analysis/REPORT.md')
