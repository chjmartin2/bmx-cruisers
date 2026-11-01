"""Execute the original row loader to export Course One's exact visible row stream."""
from pathlib import Path
from collections import deque
import sys, json, hashlib, shutil
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'.tools/python'))
from py65.devices.mpu6502 import MPU

ROOT=Path(__file__).resolve().parents[1]
RAM_PATH=ROOT/'reference/extracted/course1-initial-ram.bin'


def rectangles(mask):
    """Lossless horizontal runs, merged vertically when their extents agree."""
    result=[]; active={}
    for y,line in enumerate(mask):
        runs=[]; x=0
        while x<len(line):
            if not line[x]: x+=1; continue
            start=x
            while x<len(line) and line[x]: x+=1
            runs.append((start,x))
        next_active={}
        for run in runs:
            rect=active.pop(run,[run[0],y,run[1]-run[0],0])
            rect[3]+=1; next_active[run]=rect
        result.extend(active.values()); active=next_active
    result.extend(active.values())
    return result


def export(course=1,ram_path=None,io_path=None):
    source=(ram_path or RAM_PATH).read_bytes(); ram=list(source); cpu=MPU(memory=ram)
    rows=[list(source[0x400+r*40:0x400+(r+1)*40]) for r in range(24,0,-1)]
    colors=[list(source[0xd800+r*40:0xd800+(r+1)*40]) for r in range(24,0,-1)]
    # RAM-bank dumps exclude color RAM; initialize it from the I/O-bank capture.
    io=(io_path or ROOT/'reference/extracted/course1-initial-io.bin').read_bytes()
    ram[0xd000:0xe000]=io
    colors=[list(io[0x800+r*40:0x800+(r+1)*40]) for r in range(24,0,-1)]
    finish=None
    for tick in range(1,800):
        cpu.pc=0xc412;cpu.sp=0xfd;ram[0x1fe]=0xff;ram[0x1ff]=0x0f
        for instruction in range(50000):
            if cpu.pc==0x5500:finish=tick;break
            if cpu.pc==0x1000:break
            cpu.step()
        else:raise RuntimeError('Original row loader failed to return')
        if finish:break
        rows.append(list(ram[0x428:0x450]));colors.append(list(ram[0xd828:0xd850]))
    assert finish is not None, 'No original finish trigger'
    h=len(rows)*8;w=320
    terrain=np.zeros((h,w),np.uint8);solid=np.zeros((h,w),np.uint8)
    finish_black=np.zeros((h,w),bool);finish_white=np.zeros((h,w),bool)
    fence=[];plants=set()
    plant_codes={84,85,91,92,93,94,95,96,97,98,99,100,105}
    for ty,row in enumerate(rows):
        for tx,code in enumerate(row):
            color=colors[ty][tx]&15
            if code in plant_codes:plants.add((tx,ty))
            if 72<=code<=83 or (code==87 and color==10):
                fence.append(dict(x=tx+0.5,y=ty+0.5,code=code,bridge=(code==87)))
            for gy in range(8):
                byte=source[0x3000+code*8+gy]
                for gx in range(8):
                    if color&8:
                        value=(byte>>(6-(gx//2)*2))&3
                        palette=[13,12,5,color&7][value]
                        block=value>=2
                    else:
                        block=bool(byte&(128>>gx));palette=color if block else 13
                    px,py=tx*8+gx,ty*8+7-gy
                    terrain[py,px]=1 if palette==12 else 2 if palette==6 else 0
                    solid[py,px]=int(block)
                    if code==87 and palette in [0,1]:
                        (finish_black if palette==0 else finish_white)[py,px]=True
    props=[]
    while plants:
        seed=min(plants,key=lambda p:(p[1],p[0]));plants.remove(seed);queue=deque([seed]);group=[seed]
        while queue:
            x,y=queue.popleft()
            for nx,ny in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]:
                if (nx,ny) in plants:plants.remove((nx,ny));queue.append((nx,ny));group.append((nx,ny))
        xs,ys=zip(*group);values=[colors[y][x]&7 for x,y in group]
        props.append(dict(x=(min(xs)+max(xs)+1)/2,y=(min(ys)+max(ys)+1)/2,
                          width=max(xs)-min(xs)+1,depth=max(ys)-min(ys)+1,
                          color=max(set(values),key=values.count)))
    data=dict(format=1,source_sha256=hashlib.sha256((ROOT/'bmxracer.d64').read_bytes()).hexdigest(),
              course=course,screen_sequence=list(source[0x5700:0x574e]),finish_scroll=finish,
              width=40,height=len(rows),centimeters_per_pixel=7.5,
              codes=rows,colors=colors,road_rects=rectangles(terrain==1),
              water_rects=rectangles(terrain==2),ground_rects=rectangles(terrain!=2),
              finish_black_rects=rectangles(finish_black),finish_white_rects=rectangles(finish_white),
              shrubs=props,fences=fence)
    target=ROOT/'Content/Data';target.mkdir(parents=True,exist_ok=True)
    (target/f'course{course}.json').write_text(json.dumps(data,separators=(',',':'))+'\n',encoding='utf-8')
    (target/f'course{course}-collision.bin').write_bytes(solid.tobytes())
    (target/f'course{course}-terrain.bin').write_bytes(terrain.tobytes())
    for source_path,name in [(ram_path or RAM_PATH,'original-ram.bin' if course==1 else f'course{course}-ram.bin'),(io_path or ROOT/'reference/extracted/course1-initial-io.bin','original-io.bin' if course==1 else f'course{course}-io.bin')]:
        if source_path.resolve()!=(target/name).resolve():shutil.copyfile(source_path,target/name)
    print(f'Course One: {len(rows)} world rows, finish at scroll {finish}; {len(props)} plant groups, {len(fence)} fence cells.')
    print('Road/water/grass rectangles:',len(data['road_rects']),len(data['water_rects']),len(data['ground_rects']))


if __name__=='__main__':export()
