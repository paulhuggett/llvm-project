; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocaddi < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCADDI %s

define i32 @f() nounwind {
; RV32IC: c.addi
; RV32IC_NOCADDI-NOT: c.addi
entry:
  ret i32 1735925788
}
