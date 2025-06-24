; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s

; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocbeqz < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCBEQZ %s

; RV32IC:  c.beqz
; RV32IC_NOCBEQZ-NOT:  c.beqz

define i32 @f(i32 %x) nounwind {
entry:
  %cmp = icmp eq i32 %x, 0
  %cond = select i1 %cmp, i32 123, i32 456
  ret i32 %cond
}
