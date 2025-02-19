; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocandi < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f(i32 %a) {
; RV32IC:        c.andi
; RV32IC_NO-NOT: c.andi
  %1 = and i32 %a, 11
  ret i32 %1
}
