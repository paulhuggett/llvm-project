# RUN: llvm-mc -triple=riscv32 -mattr=+xkeysomnolh < %s
# RUN: llvm-mc -triple=riscv32 -mattr=+xkeysomnolhu < %s
# RUN: not llvm-mc -triple=riscv32 -mattr=+xkeysomnolh -mattr=+xkeysomnolhu < %s
