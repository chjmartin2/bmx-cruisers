"""Readable address-range listing of the extracted C64 RAM."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / '.tools/python'))
from py65.devices.mpu6502 import MPU
from py65.disassembler import Disassembler

ram = Path(sys.argv[1]).read_bytes()
disassembler = Disassembler(MPU(memory=list(ram)))
for region in sys.argv[2:]:
    start, end = (int(n, 16) for n in region.split(':'))
    pc = start
    while pc < end:
        size, text = disassembler.instruction_at(pc)
        print(f'{pc:04x}: {ram[pc:pc+size].hex(" "):8s} {text}')
        pc += size
