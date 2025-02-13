; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s

; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysonnoclw < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCLW %s

define i32 @select(i32 %a, ptr %b) nounwind {
; RV32IC-LABEL: select:
; RV32IC:       # %bb.0:
; RV32IC-NEXT:    c.lw a0, 0(a1)
; RV32IC-NEXT:    c.jr ra

; RV32IC_NOCLW-LABEL: select:
; RV32IC_NOCLW:       # %bb.0:
; RV32IC_NOCLW-NEXT:    c.lw a0, 0(a1)
; RV32IC_NOCLW-NEXT:    c.jr ra

  %val1 = load volatile i32, ptr %b
  ret i32 %val1
}
