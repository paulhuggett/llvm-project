; RUN: llc -mtriple=riscv64 -mattr=+m < %s \
; RUN:   | FileCheck -check-prefix=RV64M_DIV %s
; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnodiv < %s  \
; RUN:   | FileCheck -check-prefix=RV64M_NODIV %s

define signext i64 @divide(i64 noundef %x, i64 noundef %y) nounwind {
  %div = sdiv i64 %x, %y
  ret i64 %div
}

; RV64M_DIV-LABEL: divide:
; RV64M_DIV:       # %bb.0:
; RV64M_DIV-NEXT:    div a0, a0, a1
; RV64M_DIV-NEXT:    ret

; RV64M_NODIV-LABEL: divide:
; RV64M_NODIV:       # %bb.0:
; RV64M_NODIV-NEXT:    addi sp, sp, -16
; RV64M_NODIV-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NODIV-NEXT:    call __divdi3
; RV64M_NODIV-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NODIV-NEXT:    addi sp, sp, 16
; RV64M_NODIV-NEXT:    ret
