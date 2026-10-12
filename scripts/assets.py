from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path('/work')
stage = root / 'build/stage'
font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'
font = ImageFont.truetype(font_path, 14)
data = bytearray()
for code in range(128):
    im = Image.new('L', (8, 16))
    ImageDraw.Draw(im).text((0, -1), chr(code) if code >= 32 else ' ', font=font, fill=255)
    data.extend(im.tobytes())
(root / 'assets/font.bin').write_bytes(data)
# Package the supplied artwork, trimming only its empty outer margins.
logo = Image.open(root / 'icon.png').convert('RGBA')
bounds = logo.convert('RGB').getbbox()
if bounds:
    logo = logo.crop(bounds)
logo.thumbnail((480, 480), Image.LANCZOS)
icon = Image.new('RGBA', (512, 512), (0, 0, 0, 255))
icon.alpha_composite(logo, ((512-logo.width)//2, (512-logo.height)//2))
icon.convert('RGB').save(stage / 'sce_sys/icon0.png')
