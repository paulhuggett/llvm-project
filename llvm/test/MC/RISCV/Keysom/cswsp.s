# RUN: llvm-mc %s -triple=riscv32 -mattr=+c -M no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s

# RUN: not llvm-mc -triple=riscv32 -mattr=+c    \
# RUN:   -mattr=+xkeysomnocswsp                 \
# RUN:   -M no-aliases -show-encoding < %s 2>&1 \
# RUN:     | FileCheck -check-prefixes=CHECK-NO-EXT %s

# CHECK-ASM-AND-OBJ: c.swsp ra, 0(sp)
# CHECK-ASM: encoding: [0x06,0xc0]
# CHECK-NO-EXT:  error: instruction requires the following: eliminate 'XKeysomNoCswsp'
c.swsp ra, 0(sp)

# CHECK-ASM-AND-OBJ: c.swsp s0, 0(sp)
# CHECK-ASM: encoding: [0x22,0xc0]
# CHECK-NO-EXT:  error: instruction requires the following: eliminate 'XKeysomNoCswsp'
c.swsp x8, (x2)
