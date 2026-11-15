#!/usr/bin/env python3
"""Generate small, self-contained README buttons and an original pixel animation."""
from pathlib import Path
from html import escape
import math
from PIL import Image, ImageDraw
OUT=Path(__file__).resolve().parents[1]/'assets/readme'
OUT.mkdir(parents=True,exist_ok=True)
buttons={
 'download-en':('DOWNLOAD PKG','#68caff',196),'download-es':('DESCARGAR PKG','#68caff',196),
 'notes-en':('WHAT IS NEW','#a6a1ff',176),'notes-es':('NOVEDADES','#a6a1ff',176),
 'issue-en':('REPORT AN ISSUE','#70ddb0',212),'issue-es':('REPORTAR ERROR','#70ddb0',212),
 'donate-en':('DONATE · PAYPAL','#f4ce72',212),'donate-es':('DONAR · PAYPAL','#f4ce72',212),
 'english':('ENGLISH','#68caff',132),'spanish':('ESPANOL','#a6a1ff',132),
}
for name,(label,color,width) in buttons.items():
 svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="44" viewBox="0 0 {width} 44" role="img" aria-label="{escape(label)}">
<title>{escape(label)}</title><rect x="1" y="1" width="{width-2}" height="42" rx="7" fill="#111c30" stroke="#30425e"/>
<path d="M10 14h4v4h4v4h-4v4h-4z" fill="{color}"/><path d="M23 8h{width-34}" stroke="{color}" opacity=".55"/>
<text x="{(width+12)/2}" y="28" text-anchor="middle" fill="#f1f5fc" font-family="monospace" font-size="12" font-weight="bold" letter-spacing=".8">{escape(label)}</text></svg>'''
 (OUT/(name+'.svg')).write_text(svg+'\n')
(OUT/'features.svg').write_text('''<svg xmlns="http://www.w3.org/2000/svg" width="672" height="40" viewBox="0 0 672 40" role="img" aria-label="PS4, Vulkan, Native / 2x, English / Espanol"><title>PS4 · Vulkan · Native / 2x · English / Espanol</title><rect width="672" height="40" rx="7" fill="#101a2b"/>
<g fill="#9fb5cf" font-family="monospace" font-size="12" font-weight="bold" text-anchor="middle"><text x="65" y="25">PS4</text><text x="210" y="25">VULKAN</text><text x="395" y="25">NATIVE / 2x</text><text x="577" y="25">ENGLISH / ESPANOL</text></g><g fill="#60c8ff"><rect x="124" y="16" width="7" height="7"/><rect x="292" y="16" width="7" height="7"/><rect x="480" y="16" width="7" height="7"/></g></svg>\n''')
(OUT/'divider.svg').write_text('''<svg xmlns="http://www.w3.org/2000/svg" width="880" height="12" viewBox="0 0 880 12"><path d="M0 6h394m92 0h394" stroke="#344b68"/><path d="M408 3h6v6h-6zm16 0h6v6h-6zm16 0h6v6h-6zm16 0h6v6h-6z" fill="#68caff"/></svg>\n''')
# A small original controller, a moving signal and pixel stars; no game footage.
frames=[]
for frame in range(32):
 im=Image.new('RGB',(440,42),'#0b1220');d=ImageDraw.Draw(im)
 for x in range(0,440,8):d.point((x,8),fill='#1b2b42');d.point((x,34),fill='#1b2b42')
 d.line((66,22,378,22),fill='#20354e')
 for x,y in [(14,16),(18,12),(22,10),(38,10),(42,12),(46,16),(46,28),(42,30),(38,26),(22,26),(18,30),(14,28)]:
  d.rectangle((x,y,x+3,y+3),fill='#69caff')
 d.rectangle((18,14,45,26),fill='#243d59');d.rectangle((21,16,24,24),fill='#d5e7fa');d.rectangle((18,19,27,21),fill='#d5e7fa')
 for x,y,c in [(36,16,'#70ddb0'),(40,20,'#a6a1ff'),(32,20,'#f4ce72')]:d.rectangle((x,y,x+2,y+2),fill=c)
 runner=70+int(frame/31*300)
 for offset,color in [(12,'#20354e'),(6,'#3c789e'),(0,'#68caff')]:d.rectangle((runner-offset,20,runner-offset+3,23),fill=color)
 for i,(x,y) in enumerate([(104,12),(174,29),(254,12),(329,29),(402,19)]):
  col='#f4ce72' if (frame//8+i)%3==0 else '#39536d'
  d.rectangle((x-3,y-1,x+3,y+1),fill=col);d.rectangle((x-1,y-3,x+1,y+3),fill=col)
 frames.append(im.resize((880,84),Image.Resampling.NEAREST))
frames[0].save(OUT/'pixel-signal.gif',save_all=True,append_images=frames[1:],duration=100,loop=0,optimize=True,disposal=2)
print('Generated',len(buttons)+3,'README assets in',OUT)
