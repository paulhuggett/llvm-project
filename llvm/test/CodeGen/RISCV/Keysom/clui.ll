; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; Switch off the c.lui instruction
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnoclui < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCLUI %s

; Switch off the lui instruction
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnolui < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOLUI %s

define i32 @f() nounwind {
; RV32IC: c.lui
; RV32IC_NOCLUI-NOT: c.lui {{.+}}, {{[0-9]+}}
; RV32IC_NOLUI-NOT: c.lui {{.+}}, {{[0-9]+}}
; RV32IC_NOLUI-NOT: lui {{.+}}, {{[0-9]+}}
entry:
  ret i32 65536
}
