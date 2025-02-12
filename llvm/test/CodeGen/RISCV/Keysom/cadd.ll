; RUN: llc -mtriple=riscv32 -mattr=+c \
; RUN:     -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32C_CADD %s
; RUN: llc -mtriple=riscv32 -mattr=+c \
; RUN:     -mattr=+xkeysomnocadd \
; RUN:     -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32C_NOCADD %s

define i32 @add(i32 %a, i32 %b) nounwind {
; RV32C_CADD-LABEL: add:
; RV32C_CADD:       # %bb.0:
; RV32C_CADD-NEXT:    c.add a0, a1
; RC32C_CADD-NEXT:    c.jr ra

; RV32C_NOCADD-LABEL: add:
; RV32C_NOCADD:       # %bb.0:
; RV32C_NOCADD-NEXT:    add a0, a0, a1
; RC32C_NOCADD-NEXT:    c.jr ra

  %1 = add i32 %a, %b
  ret i32 %1
}
