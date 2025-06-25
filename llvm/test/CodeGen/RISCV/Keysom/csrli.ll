; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocsrli < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f(i32 %a, i32 %b) {
; RV32IC:        c.srli
; RV32IC_NO-NOT: c.srli
  %shr = lshr i32 %a, 1
  ret i32 %shr
}
