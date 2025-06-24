; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s

; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnoclw < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCLW %s

define i32 @load(i32 %a, ptr %b) nounwind {
; RV32IC-LABEL: load:
; RV32IC:       # %bb.0:
; RV32IC-NEXT:    c.lw a0, 0(a1)
; RV32IC-NEXT:    c.jr ra

; RV32IC_NOCLW-LABEL: load:
; RV32IC_NOCLW:       # %bb.0:
; RV32IC_NOCLW-NEXT:    lw a0, 0(a1)
; RV32IC_NOCLW-NEXT:    c.jr ra

  %val1 = load volatile i32, ptr %b
  ret i32 %val1
}
