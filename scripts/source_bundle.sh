#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p out
test -f vendor/parallel-n64/Makefile
python3 - <<'PY'
import json
from pathlib import Path
for entry in json.loads(Path('sources.lock.json').read_text()).values():
 if 'revision' in entry:
  (Path(entry['path'])/'.nps64-revision').write_text(entry['revision']+'\n')
PY
python3 scripts/source_bundle.py
