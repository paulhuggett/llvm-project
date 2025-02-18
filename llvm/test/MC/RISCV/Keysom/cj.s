# RUN: llvm-mc %s -triple=riscv32 -mattr=+c -M no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s

# RUN: not llvm-mc -triple=riscv32 -mattr=+c    \
# RUN:   -mattr=+xkeysomnocj                    \
# RUN:   -M no-aliases -show-encoding < %s 2>&1 \
# RUN:     | FileCheck -check-prefixes=CHECK-NO-EXT %s

# CHECK-ASM-AND-OBJ: c.j 2046
# CHECK-ASM: encoding: [0xfd,0xaf]
# CHECK-NO-EXT:  error: instruction requires the following: eliminate 'XKeysomNoCj'

c.j 2046
