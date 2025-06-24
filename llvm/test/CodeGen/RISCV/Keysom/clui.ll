; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnoclui < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCLUI %s

define i32 @f() nounwind {
; RV32IC: c.lui
; RV32IC_NOCLUI-NOT: c.lui
entry:
  ret i32 65536
}
