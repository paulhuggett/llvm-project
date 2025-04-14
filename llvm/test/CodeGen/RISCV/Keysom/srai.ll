; RUN: llc -mtriple=riscv32 -riscv-no-aliases -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefixes=RV32I %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -verify-machineinstrs \
; RUN:     -mattr=+xkeysomnosrai < %s \
; RUN:   | FileCheck -check-prefixes=RV32I_NOSRAI %s

define i32 @f(i32 %x) nounwind {
; RV32I-LABEL: f:
; RV32I:            srai {{.+}}, {{.+}}, {{[0-9]+}}
; RV32I_NOSRAI-NOT: srai {{.+}}, {{.+}}, {{[0-9]+}}
entry:
  %shl = ashr i32 %x, 3
  ret i32 %shl
}
