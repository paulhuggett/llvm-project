; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_AMOORW %s
; RUN: llc -mtriple=riscv32 -mattr=+a    \
; RUN:     -mattr=+xkeysomnoamoorw  < %s \
; RUN:   | FileCheck -check-prefix=RV32A_NOAMOORW %s

define void @amoorw(ptr %p) nounwind {
  %v = atomicrmw or ptr %p, i32 13 seq_cst, align 4
  ret void
}


; RV32A_AMOORW-LABEL: amoorw:
; RV32A_AMOORW:       # %bb.0:
; RV32A_AMOORW-NEXT:    li a1, 13
; RV32A_AMOORW-NEXT:    amoor.w.aqrl zero, a1, (a0)
; RV32A_AMOORW-NEXT:    ret

; RV32A_NOAMOORW-LABEL: amoorw:
; RV32A_NOAMOORW:       # %bb.0:
; RV32A_NOAMOORW-NEXT:    addi sp, sp, -16
; RV32A_NOAMOORW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32A_NOAMOORW-NEXT:    li a1, 13
; RV32A_NOAMOORW-NEXT:    call __sync_fetch_and_or_4
; RV32A_NOAMOORW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32A_NOAMOORW-NEXT:    addi sp, sp, 16
; RV32A_NOAMOORW-NEXT:    ret
