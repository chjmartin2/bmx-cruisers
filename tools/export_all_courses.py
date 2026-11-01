"""Walk the original loader and finish routine; capture each actual course boundary."""
from pathlib import Path
import sys,json
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.tools/python'))
from py65.devices.mpu6502 import MPU
from export_course import export
from smooth_course import export as smooth

ram=list((ROOT/'Content/Data/original-ram.bin').read_bytes())
ram[0xd000:0xe000]=(ROOT/'Content/Data/original-io.bin').read_bytes()
ram[0xffd2]=0x60 # ROM character output is presentation only.
cpu=MPU(memory=ram)
report=[]
for course in range(1,6):
    if course>1:
        ram[0x40a:0x410]=[0xb0]*6
        ram[0x41a]=0xb0+course
        rp=ROOT/f'Content/Data/course{course}-ram.bin';ip=ROOT/f'Content/Data/course{course}-io.bin'
        rp.write_bytes(bytes(ram));ip.write_bytes(bytes(ram[0xd000:0xe000]))
        export(course,rp,ip)
        smooth(course)
    finished=False
    for row in range(1,800):
        cpu.pc=0xc412;cpu.sp=0xfd;ram[0x1fe]=0xff;ram[0x1ff]=0x0f
        for instruction in range(3000000):
            if cpu.pc==0x5500:finished=True
            if cpu.pc==0x1000:break
            cpu.step()
        else:raise RuntimeError('Original loader did not return')
        if finished:break
    assert finished
    report.append(dict(course=course,finish_scroll=row,next_sequence_index=ram[0x52fc],movement_delay=ram[0x4555]))
    print(report[-1],flush=True)
(ROOT/'Content/Data/courses.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
