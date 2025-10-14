# RUN: llvm-mc -triple=riscv32 -mattr=+xkeysomnolb < %s
# RUN: llvm-mc -triple=riscv32 -mattr=+xkeysomnolbu < %s
# RUN: not llvm-mc -triple=riscv32 -mattr=+xkeysomnolb -mattr=+xkeysomnolbu < %s
