import json
import subprocess
from pathlib import Path

root = Path('/work')
sources = json.loads((root / 'sources.lock.json').read_text())
for name, item in sources.items():
    if 'revision' not in item:
        continue  # Release tarballs are verified by bootstrap_gpu.sh.
    dst = root / item['path']
    if not dst.exists():
        subprocess.run(['git', 'init', str(dst)], check=True)
        subprocess.run(['git', '-C', str(dst), 'remote', 'add', 'origin', item['url']], check=True)
        subprocess.run(['git', '-C', str(dst), 'fetch', '--depth', '1', 'origin', item['revision']], check=True)
        subprocess.run(['git', '-C', str(dst), 'checkout', '--detach', 'FETCH_HEAD'], check=True)
    marker = dst / '.nps64-revision'
    if not (dst / '.git').exists() and marker.exists():
        head = marker.read_text().strip()
    else:
        head = subprocess.check_output(['git', '-c', 'safe.directory=*', '-C', str(dst), 'rev-parse', 'HEAD'], text=True).strip()
    if head != item['revision']:
        raise SystemExit(f'{name}: revision inesperada {head}; se requiere {item["revision"]}')
    print(f'{name}: {head}')
