"""Create a thumbnail from authentic before-and-after footage and the game logo."""
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'.tools/python'))
import cv2
from PIL import Image,ImageDraw,ImageFont,ImageEnhance,ImageFilter
src=root/'release-work/trailer-1.2/sources';out=root/'releases/1.2.0/media'
def frame(name,t):
 c=cv2.VideoCapture(str(src/(name+'.mp4')));c.set(cv2.CAP_PROP_POS_MSEC,t*1000);ok,f=c.read();c.release();assert ok
 return Image.fromarray(cv2.cvtColor(f,cv2.COLOR_BGR2RGB))
modern=frame('course1',3.95);retro=frame('original-c64',5.35)
bg=ImageEnhance.Brightness(modern.resize((1280,720)).filter(ImageFilter.GaussianBlur(10))).enhance(.30).convert('RGBA')
logo=Image.open(root/'assets/source/branding/bmx-cruisers-logo.png').convert('RGBA');logo.thumbnail((485,225));bg.alpha_composite(logo,(65,-8))
d=ImageDraw.Draw(bg);font=lambda n:ImageFont.truetype(str(root/'Content/UI/Fonts/Roboto-BoldCondensedItalic.ttf'),n)
d.text((565,35),'THE CLASSIC RIDE.',font=font(61),fill='#fff7e8',stroke_width=1)
d.text((565,105),'REBUILT.',font=font(84),fill='#fff12d',stroke_width=1)
d.text((38,228),'COMMODORE 64',font=font(30),fill='#fff12d');d.text((540,228),'UNREAL ENGINE 5',font=font(30),fill='#fff12d')
bg.paste(retro.resize((466,350),Image.Resampling.NEAREST),(35,270));bg.paste(modern.resize((720,405),Image.Resampling.LANCZOS),(525,270))
d=ImageDraw.Draw(bg);d.rectangle((33,268,503,622),outline='#fff12d',width=3);d.rectangle((523,268,1247,677),outline='#fff12d',width=3)
d.polygon([(477,411),(540,449),(477,487)],fill='#ff492f',outline='#fff12d')
d.rectangle((25,635,510,704),fill='#151c25');d.text((43,645),'VERSION 1.2  /  WINDOWS',font=font(33),fill='#ffffff')
bg.convert('RGB').save(out/'BMX-Cruisers-1.2-YouTube-Thumbnail.jpg',quality=96)
print('Thumbnail ready')
