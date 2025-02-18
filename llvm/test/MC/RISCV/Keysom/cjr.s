# RUN: llvm-mc %s -triple=riscv32 -mattr=+c -M no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s

# RUN: not llvm-mc -triple=riscv32 -mattr=+c    \
# RUN:   -mattr=+xkeysomnocjr                    \
# RUN:   -M no-aliases -show-encoding < %s 2>&1 \
# RUN:     | FileCheck -check-prefixes=CHECK-NO-EXT %s

# CHECK-ASM-AND-OBJ: c.jr ra
# CHECK-ASM: encoding: [0x82,0x80]
# CHECK-NO-EXT:  error: instruction requires the following: eliminate 'XKeysomNoCjr'

c.jr ra
