"""Audit the captured first split-pipeline fill batch without executing shaders.

This checks host-visible post-hang data, not PS4 GPU execution or coherency.
Captured bytes remain outside git.
"""
import collections
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

capture = Path(sys.argv[1] if len(sys.argv) > 1 else 'logs')
out = Path(sys.argv[2] if len(sys.argv) > 2 else 'build/parallel-blend-analysis')
out.mkdir(parents=True, exist_ok=True)
report = (capture/'gpu-hang.txt').read_text()
assert 'memory 0xb done' in report, 'Not the expected split blend-stage capture'
assert 'memory 0xc done' not in report, 'Blend wait completed; review subsequent stage'

def words(address):
    data = (capture/f'gpu-hang-data-{address}.bin').read_bytes()
    assert len(data) % 4 == 0
    return struct.unpack(f'<{len(data)//4}I', data)

coarse = words('43f8321000')
fine = words('43f8640000')
offsets = words('43f8720000')
coverage = (capture/'gpu-hang-data-43f87a0000.bin').read_bytes()
indirect = words('43f8331000')
expected_tiles = {y*128+x for y in range(30) for x in range(40)}
active = {i for i, mask in enumerate(coarse) if mask}
checks = {
    'active_tiles_match_40x30': active == expected_tiles,
    'coarse_masks_single_primitive': all(coarse[i] == 1 for i in active),
    'fine_masks_match_coarse': all(fine[i*8] == coarse[i] and
        all(fine[i*8+j] == 0 for j in range(1, 8)) for i in range(len(coarse))),
    'tile_offsets_unique_and_in_range': sorted(offsets[i*8] for i in active) == list(range(1200)),
    'indirect_work_count_matches': indirect[0] == 1200 and indirect[3] == 1200,
    'coverage_reads_in_bounds': all((offsets[i*8]+1)*64 <= len(coverage) for i in active),
}
counts = collections.Counter()
for i in active:
    start = offsets[i*8]*64
    counts.update(coverage[start:start+64])
checks['coverage_is_fill_or_rejected'] = set(counts) <= {64, 255}
checks['coverage_total_matches_tiles'] = sum(counts.values()) == 1200*64
result = {
    'kind': 'post-hang host-visible first fill-batch audit; no GPU execution',
    'waiting': re.search(r'waiting for (\d+), GPU completed (\d+), submitted (\d+)', report).groups(),
    'last_marker': 11, 'active_tiles': len(active),
    'coverage_bytes': {str(k): v for k, v in sorted(counts.items())},
    'checks': checks,
    'capture_hashes': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
        for p in sorted(capture.glob('gpu-hang*'))},
}
(out/'report.json').write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps({k: v for k, v in result.items() if k != 'capture_hashes'}, indent=2))
if not all(checks.values()):
    raise SystemExit('FAIL captured fill-batch invariants; inspect report.json')
print('PASS captured masks, offsets, indirect counts and coverage bounds; GPU hang remains unresolved')
