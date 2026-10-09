"""Compare real-ROM game pixels against saved upstream-binning captures."""
from pathlib import Path
from PIL import Image
import hashlib

lines=[]
for frame in (300,330):
    # Exclude changing FPS HUD and status bar; compare full-width game area.
    region=(0,64,1280,656)
    reference=Image.open(f'build/gpu-binning-reference/frame{frame}.ppm').crop(region).tobytes()
    direct=Image.open(f'build/bench-frame-{frame}.ppm').crop(region).tobytes()
    assert reference==direct, f'Game pixels differ at frame {frame}'
    lines.append(f'PASS frame {frame}: exact game pixels excluding HUD = {hashlib.sha256(direct).hexdigest()}')
Path('build/gpu-tile-equivalence/image-results.log').write_text('\n'.join(lines)+'\n')
print('\n'.join(lines))
