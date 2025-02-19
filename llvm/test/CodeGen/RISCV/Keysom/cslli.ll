; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocslli < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f(i32 %a, i32 %b) {
; RV32IC:        c.slli
; RV32IC_NO-NOT: c.slli
  %1 = shl i32 %a, 7
  ret i32 %1
}
