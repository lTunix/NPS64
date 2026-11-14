#!/usr/bin/env bash
# Reproduce exact pixels and compare against the validated pre-optimization scaler.
set -euo pipefail
cd /work
mkdir -p build/presentation-bench
git show 2d8aae5:src/bilinear_scaler.h > build/presentation-bench/baseline.h
clang -std=c11 -O2 -Wall -Wextra -Werror -Isrc -I. -DNPS64_BILINEAR_HEADER='"build/presentation-bench/baseline.h"' tests/bench_bilinear_scaler.c -o build/presentation-bench/baseline
clang -mssse3 -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/bench_bilinear_scaler.c -o build/presentation-bench/optimized
for n in 1 2 3; do
 build/presentation-bench/baseline > "build/presentation-bench/baseline-$n.txt"
 build/presentation-bench/optimized > "build/presentation-bench/optimized-$n.txt"
done
python3 - <<'PY'
from pathlib import Path
import json,re,statistics
root=Path('build/presentation-bench');results=[]
for i in range(8):
 a=[];b=[]
 for n in range(1,4):
  p=(root/f'baseline-{n}.txt').read_text().splitlines()[i];q=(root/f'optimized-{n}.txt').read_text().splitlines()[i]
  assert p.split(' ms=')[0]==q.split(' ms=')[0]
  assert p.split('hash=')[1]==q.split('hash=')[1],(p,q)
  a.append(float(re.search(r'ms=([\d.]+)',p)[1]));b.append(float(re.search(r'ms=([\d.]+)',q)[1]))
 results.append(dict(case=p.split(' ms=')[0],baseline_ms=statistics.median(a),optimized_ms=statistics.median(b),identical_pixels=True))
(root/'comparison.json').write_text(json.dumps(results,indent=2)+'\n')
print('PASS presentation: 8 identical output hashes; timings in build/presentation-bench/comparison.json')
PY
