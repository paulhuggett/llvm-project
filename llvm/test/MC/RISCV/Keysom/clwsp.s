# RUN: llvm-mc %s -triple=riscv32 -mattr=+c -M no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s

# RUN: not llvm-mc -triple=riscv32 -mattr=+c    \
# RUN:   -mattr=+xkeysomnoclwsp                 \
# RUN:   -M no-aliases -show-encoding < %s 2>&1 \
# RUN:     | FileCheck -check-prefixes=CHECK-NO-EXT %s

# CHECK-ASM-AND-OBJ: c.lwsp ra, 0(sp)
# CHECK-ASM: encoding: [0x82,0x40]
# CHECK-NO-EXT:  error: instruction requires the following: eliminate 'XKeysomNoClwsp'
c.lwsp ra, 0(sp)

# CHECK-ASM-AND-OBJ: c.lwsp s0, 0(sp)
# CHECK-ASM: encoding: [0x02,0x44]
# CHECK-NO-EXT:  error: instruction requires the following: eliminate 'XKeysomNoClwsp'
c.lwsp x8, (x2)
