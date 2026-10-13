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

# Proportional grayscale glyphs at their native display sizes (HD library).
import struct
sizes=(16,22,32,44)
ui=bytearray(b'UIF1')
for size in sizes:
 face=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',size)
 for code in range(32,128):
  glyph=Image.new('L',(48,56))
  ImageDraw.Draw(glyph).text((0,0),chr(code),font=face,fill=255)
  ui.append(min(48,round(face.getlength(chr(code)))))
  ui.extend(glyph.tobytes())
(stage/'ui-font.bin').write_bytes(ui)
(stage/'cover-catalog.txt').write_bytes((root/'assets/cover-catalog.txt').read_bytes())
