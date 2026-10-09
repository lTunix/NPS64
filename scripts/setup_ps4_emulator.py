"""Download the pinned official shadPS4 AppImage; run with Python 3."""
import hashlib, urllib.request, zipfile
from pathlib import Path
root=Path("build/shadps4")
root.mkdir(parents=True,exist_ok=True)
archive=root/"release.zip"
url="https://github.com/shadps4-emu/shadPS4/releases/download/v.0.19.0/shadps4-linux-sdl-0.19.0.zip"
expected="a6e3b83fd6c9ea7b29c37e77e1aa687802d4ecd98c2ea794ca9e1d97b24a0cf9"
if not archive.exists(): urllib.request.urlretrieve(url,archive)
if hashlib.sha256(archive.read_bytes()).hexdigest()!=expected:
    raise SystemExit("Release checksum mismatch; refusing extraction")
with zipfile.ZipFile(archive) as z:
    data=z.read("Shadps4-sdl.AppImage")
(root/"Shadps4-sdl.AppImage").write_bytes(data)
print("PASS pinned shadPS4 v0.19.0 archive checksum")
