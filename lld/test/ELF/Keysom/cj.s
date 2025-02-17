# REQUIRES: riscv
## Relax R_RISCV_CALL and R_RISCV_CALL_PLT.
## Features +c and +c,+xkeysomnocj

# RUN: rm -rf %t && split-file %s %t && cd %t

## RV32 +C
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+c,+relax a.s -o a.32c.o
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+c,+relax b.s -o b.32c.o
# RUN: ld.lld -shared -soname=b.so b.32c.o -o b.32c.so
# RUN: ld.lld -T lds a.32c.o b.32c.o -o 32c
# RUN: llvm-objdump -td --no-show-raw-insn -M no-aliases 32c | FileCheck %s --check-prefixes=RVC32

## RV32 +C +xkeysomnocj
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+c,+relax,+xkeysomnocj --defsym=nocj=1 a.s -o a.32c.nocj.o
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+c,+relax,+xkeysomnocj --defsym=nocj=1 b.s -o b.32c.nocj.o
# RUN: ld.lld -T lds a.32c.nocj.o b.32c.nocj.o -o 32c.nocj
# RUN: llvm-objdump -td --no-show-raw-insn -M no-aliases 32c.nocj | FileCheck %s --check-prefixes=RVC32_NOCJ

# RVC32: c.j {{.*}} <{{.*}}>
# RVC32_NOCJ-NOT: c.j {{.*}} <{{.*}}>

#--- a.s
  .option arch, +c
  /* Add the xkeysomnocj (disable c.j) feature is selected on the command-line */
  .ifdef nocj
  .option arch, +xkeysomnocj
  .attribute Tag_arch, "rv32ic_xkeysomnocj"
  .else
  .attribute Tag_arch, "rv32ic"
  .endif

  .global _start, _start_end
_start:
  tail a@plt
  jump a, t0
.balign 16
  call a   # rv32c: c.jal; rv64c: jal
  call bar # PLT call can be relaxed. rv32c: c.jal; rv64c: jal

a:
  ret

_start_end:

.section .mid,"ax",@progbits
  call _start@plt # rv32c: c.jal; rv64c: jal
  call _start@plt

.section .mid2,"ax",@progbits
  call _start@plt

.section .high,"ax",@progbits
  call _start@plt # relaxable for %t/32c
  call _start@plt # not relaxed

#--- b.s
.globl bar
bar:
  ret

#--- lds
SECTIONS {
  .text 0x10000 : { *(.text) }
  .plt 0x10400 : { *(.plt) }
  .mid 0x10800 : { *(.mid); mid_end = .; }
  .mid2 mid_end+4 : { *(.mid2) }
  # 22 is the size of _start in %t/32c (RVC32).
  .high 0x110000+(_start_end-_start)-22 : { *(.high); high_end = .; }
}
