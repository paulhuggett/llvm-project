; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocli < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCLI %s

define i32 @f() nounwind {
; RV32IC: c.li
; RV32IC_NOCLI-NOT: c.li
entry:
  ret i32 20
}
