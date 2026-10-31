"""Render the original character maps, without interpreting their geometry."""
from pathlib import Path
from collections import Counter
import json
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ram = (ROOT / 'reference/extracted/game-ram.bin').read_bytes()
PALETTE = [(0,0,0),(255,255,255),(175,60,88),(126,243,214),(170,64,245),(98,213,50),(44,61,236),(255,255,70),(183,99,30),(119,83,0),(238,123,149),(98,98,98),(148,148,148),(183,255,134),(115,133,255),(205,205,205)]


def tile(code, color):
    im = Image.new('RGB', (8, 8), PALETTE[13])
    glyph = ram[0x3000 + code*8:0x3000 + code*8 + 8]
    for y, byte in enumerate(glyph):
        for x in range(8):
            if color & 8:
                value = (byte >> (6 - (x//2)*2)) & 3
                palette = [13,12,5,color & 7]
                im.putpixel((x,y), PALETTE[palette[value]])
            elif byte & (128 >> x):
                im.putpixel((x,y), PALETTE[color & 15])
    return im


def screen(base):
    image = Image.new('RGB', (320,200))
    for i in range(1000):
        image.paste(tile(ram[base+i],ram[base+1024+i]),((i%40)*8,(i//40)*8))
    return image


if __name__ == '__main__':
    output = ROOT / 'reference/extracted'
    atlas = Image.new('RGB',(16*80,16*70),(30,30,30))
    draw = ImageDraw.Draw(atlas)
    for c in range(256):
        x,y=(c%16)*80,(c//16)*70
        atlas.paste(tile(c,15).resize((48,48),Image.Resampling.NEAREST),(x,y))
        draw.text((x,y+49),f'{c:02x}',fill='white')
    atlas.save(output/'character-atlas.png')
    sheets = Image.new('RGB',(640,800))
    bases=[]
    for i in range(8):
        base=(ram[0x5780+i]+256*ram[0x5790+i])-960
        bases.append(base)
        image=screen(base)
        image.save(output/f'screen-{i}.png')
        sheets.paste(image,((i%2)*320,(i//2)*200))
        print('screen',i,hex(base),'codes',dict(Counter(ram[base:base+1000])))
    sheets.resize((1280,1600),Image.Resampling.NEAREST).save(output/'all-map-screens.png')
    order=list(ram[0x5700:0x5708])
    route=Image.new('RGB',(320,8*200))
    for i,index in enumerate(order):route.paste(screen(bases[index]),(0,(7-i)*200))
    route.save(output/'course1-original-map.png')
    print('Course One screen sequence:',order)
