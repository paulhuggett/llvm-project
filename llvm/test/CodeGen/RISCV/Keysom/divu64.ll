; RUN: llc -mtriple=riscv64 -mattr=+m < %s \
; RUN:   | FileCheck -check-prefix=RV64M_DIVU %s
; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnodivu < %s  \
; RUN:   | FileCheck -check-prefix=RV64M_NODIVU %s

define signext i64 @divide(i64 noundef %x, i64 noundef %y) nounwind {
  %div = udiv i64 %x, %y
  ret i64 %div
}

; RV64M_DIVU-LABEL: divide:
; RV64M_DIVU:       # %bb.0:
; RV64M_DIVU-NEXT:    divu a0, a0, a1
; RV64M_DIVU-NEXT:    ret


; RV64M_NODIVU-LABEL: divide:
; RV64M_NODIVU:       # %bb.0:
; RV64M_NODIVU-NEXT:    addi sp, sp, -16
; RV64M_NODIVU-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NODIVU-NEXT:    call __udivdi3
; RV64M_NODIVU-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NODIVU-NEXT:    addi sp, sp, 16
; RV64M_NODIVU-NEXT:    ret
