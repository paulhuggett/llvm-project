; RUN: llc -mtriple=riscv32 -mattr=+m < %s \
; RUN:   | FileCheck -check-prefix=RV32M_DIVU %s
; RUN: llc -mtriple=riscv32 -mattr=+m < %s \
; RUN:     -mattr=+xkeysomnodivu           \
; RUN:   | FileCheck -check-prefix=RV32M_NODIVU %s

define signext i32 @divide(i32 noundef %x, i32 noundef %y) nounwind {
  %div = udiv i32 %x, %y
  ret i32 %div
}


; RV32M_DIVU-LABEL: divide:
; RV32M_DIVU:       # %bb.0:
; RV32M_DIVU-NEXT:    divu a0, a0, a1
; RV32M_DIVU-NEXY:    ret

; RV32M_NODIVU-LABEL: divide:
; RV32M_NODIVU:       # %bb.0:
; RV32M_NODIVU-NEXT:    addi sp, sp, -16
; RV32M_NODIVU-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NODIVU-NEXT:    call __udivsi3
; RV32M_NODIVU-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NODIVU-NEXT:    addi sp, sp, 16
; RV32M_NODIVU-NEXT:    ret
