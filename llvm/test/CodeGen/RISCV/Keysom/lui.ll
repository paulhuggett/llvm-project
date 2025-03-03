; RUN: llc -mtriple=riscv32 -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32I %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+xkeysomnolui < %s \
; RUN:   | FileCheck -check-prefix=RV32I_NO %s

; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+c < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+c,+xkeysomnolui < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f() nounwind {
; RV32I:        lui {{.+}}, {{[0-9]+}}
; RV32I_NO-NOT: lui {{.+}}, {{[0-9]+}}
; RV32IC: c.lui {{.+}}, {{[0-9]+}}
; RV32IC_NO-NOT: c.lui {{.+}}, {{[0-9]+}}
  ret i32 65536
}
