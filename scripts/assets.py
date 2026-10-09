from pathlib import Path
import os
from PIL import Image, ImageDraw, ImageFont

root = Path('/work')
stage = Path(os.environ.get('NPS64_STAGE_DIR', '/work/build/stage'))
font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'
font = ImageFont.truetype(font_path, 14)
data = bytearray()
for code in range(128):
    im = Image.new('L', (8, 16))
    ImageDraw.Draw(im).text((0, -1), chr(code) if code >= 32 else ' ', font=font, fill=255)
    data.extend(im.tobytes())
(root / 'assets/font.bin').write_bytes(data)
im = Image.new('RGB', (512, 512), '#101e30')
d = ImageDraw.Draw(im)
d.rounded_rectangle((46, 46, 466, 466), 48, outline='#48dac1', width=8)
d.text((68, 177), 'NPS64', font=ImageFont.truetype(font_path, 98), fill='#f5f7ff')
d.text((80, 315), 'NINTENDO 64 / PS4', font=ImageFont.truetype(font_path, 26), fill='#48dac1')
d.text((139, 387), 'EXPERIMENTAL', font=ImageFont.truetype(font_path, 24), fill='#9baabd')
im.save(stage / 'sce_sys/icon0.png')
