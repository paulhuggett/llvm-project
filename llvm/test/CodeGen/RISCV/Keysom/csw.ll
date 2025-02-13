; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s

; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysonnocsw < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCSW %s

define void @store(i32 %a, ptr %b) nounwind {
; RV32IC-LABEL: store:
; RV32IC:       # %bb.0:
; RV32IC-NEXT:    c.sw a0, 0(a1)
; RV32IC-NEXT:    c.jr ra

; RV32IC_NOCSW-LABEL: store:
; RV32IC_NOCSW:       # %bb.0:
; RV32IC_NOCSW-NEXT:    sw a0, 0(a1)
; RV32IC_NOCSW-NEXT:    c.jr ra

  store i32 %a, ptr %b, align 4
  ret void
}
