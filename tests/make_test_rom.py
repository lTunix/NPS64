"""Original, minimal MIPS test IPL3. No Nintendo IPL or commercial ROM data."""
import struct
from pathlib import Path

rom = bytearray(4096)
rom[:4] = bytes.fromhex('80371240')
rom[8:12] = struct.pack('>I', 0xA4000040)
rom[16:24] = bytes.fromhex('4e50533634544553')
rom[32:52] = b'NPS64 OPEN TEST'.ljust(20, b' ')
rom[62] = ord('E')
code = []
def li(reg, value):
    code.extend([0x3C000000 | (reg << 16) | (value >> 16),
                 0x34000000 | (reg << 21) | (reg << 16) | (value & 65535)])
def sw(reg, base, offset):
    code.append(0xAC000000 | (base << 21) | (reg << 16) | offset)
# Disable interrupts; stay in SP DMEM, fill RDRAM as a 32-bit turquoise surface.
code += [0x40806000, 0, 0]  # mtc0 zero,status
li(8, 0xA0100000)
li(9, 320 * 240)
li(10, 0x48DAC1FF)
loop = len(code)
sw(10, 8, 0)
code += [0x25080004, 0x2529FFFF]
code += [0x15200000 | ((loop - (len(code) + 1)) & 65535), 0]
li(8, 0xA4400000)
vi = {0: 0x00003203, 4: 0x00100000, 8: 320, 12: 2,
      20: 0x03E52239, 24: 525, 28: 0x0C15, 32: 0x0C150C15,
      36: 0x006C02EC, 40: 0x002501FF, 44: 0x000E0204,
      48: 0x200, 52: 0x400}
for offset, value in vi.items():
    li(9, value)
    sw(9, 8, offset)
code += [0x1000FFFF, 0]  # beq zero,zero,-1; nop
rom[64:64+len(code)*4] = struct.pack('>'+'I'*len(code), *code)
dst = Path('/work/build/testdata/games')
dst.mkdir(parents=True, exist_ok=True)
(dst/'NPS64-test.z64').write_bytes(rom)
print('Generated original N64 test ROM (4096 bytes)')
