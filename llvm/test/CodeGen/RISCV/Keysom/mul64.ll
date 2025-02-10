

; RUN: llc -mtriple=riscv64 -mattr=+m -mattr=-xkeysomnomul -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_MUL %s
; RUN: llc -mtriple=riscv64 -mattr=+m -mattr=+xkeysomnomul -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NO_MUL %s

define signext i64 @square(i64 %a) nounwind {
  %1 = mul i64 %a, %a
  ret i64 %1
}

; RV64M_MUL-LABEL: square:
; RV64M_MUL:       # %bb.0:
; RV64M_MUL-NEXT:    mul a0, a0, a0
; RV64M_MUL-NEXT:    ret

; RV64M_NO_MUL-LABEL: square:
; RV64M_NO_MUL:       # %bb.0
; RV64M_NO_MUL-NEXT:    addi sp, sp, -16
; RV64M_NO_MUL-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NO_MUL-NEXT:    mv a1, a0
; RV64M_NO_MUL-NEXT:    call __muldi3
; RV64M_NO_MUL-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NO_MUL-NEXT:    addi sp, sp, 16
; RV64M_NO_MUL-NEXT:    ret

