# RUN: llvm-mc %s -triple=riscv32 -mattr=+c -M no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s

# RUN: not llvm-mc -triple=riscv32 -mattr=+c    \
# RUN:   -mattr=+xkeysomnocjalr                    \
# RUN:   -M no-aliases -show-encoding < %s 2>&1 \
# RUN:     | FileCheck -check-prefixes=CHECK-NO-EXT %s

# CHECK-ASM-AND-OBJ: c.jalr s0
# CHECK-ASM: encoding: [0x02,0x94]
# CHECK-NO-EXT:  error: instruction requires the following: eliminate 'XKeysomNoCjalr'

c.jalr s0
