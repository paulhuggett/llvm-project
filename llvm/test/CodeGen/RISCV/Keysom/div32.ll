; RUN: llc -mtriple=riscv32 -mattr=+m < %s \
; RUN:   | FileCheck -check-prefix=RV32M_DIV %s
; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnodiv < %s  \
; RUN:   | FileCheck -check-prefix=RV32M_NODIV %s

define signext i32 @divide(i32 noundef %x, i32 noundef %y) nounwind {
  %div = sdiv i32 %x, %y
  ret i32 %div
}

; RV32M_DIV-LABEL: divide:
; RV32M_DIV:       # %bb.0:
; RV32M_DIV-NEXT:    div a0, a0, a1
; RV32M_DIV-NEXT:    ret

; RV32M_NODIV-LABEL: divide:
; RV32M_NODIV:       # %bb.0:
; RV32M_NODIV-NEXT:    addi sp, sp, -16
; RV32M_NODIV-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NODIV-NEXT:    call __divsi3
; RV32M_NODIV-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NODIV-NEXT:    addi sp, sp, 16
; RV32M_NODIV-NEXT:    ret
