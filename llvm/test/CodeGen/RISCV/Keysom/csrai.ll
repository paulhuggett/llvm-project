; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocsrai < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f(i32 %a, i32 %b) {
; RV32IC:        c.srai
; RV32IC_NO-NOT: c.srai
  %1 = add i32 %a, 1
  %2 = and i32 %1, 11
  %3 = shl i32 %2, 7
  %4 = ashr i32 %b, 9
  %5 = add i32 %3, %4
  ret i32 %5
}
