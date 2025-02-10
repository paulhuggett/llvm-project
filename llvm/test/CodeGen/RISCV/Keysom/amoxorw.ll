; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_AMOXORW %s
; RUN: llc -mtriple=riscv32 -mattr=+a    \
; RUN:     -mattr=+xkeysomnoamoxorw < %s \
; RUN:   | FileCheck -check-prefix=RV32A_NOAMOXORW %s

define void @amoxorw(ptr %p) nounwind {
  %v = atomicrmw xor ptr %p, i32 13 seq_cst, align 4
  ret void
}

; RV32A_AMOXORW-LABEL: amoxorw:
; RV32A_AMOXORW:       # %bb.0:
; RV32A_AMOXORW-NEXT:    li a1, 13
; RV32A_AMOXORW-NEXT:    amoxor.w.aqrl zero, a1, (a0)
; RV32A_AMOXORW-NEXT:    ret

; RV32A_NOAMOXORW-LABEL: amoxorw:
; RV32A_NOAMOXORW:       # %bb.0:
; RV32A_NOAMOXORW-NEXT:    addi sp, sp, -16
; RV32A_NOAMOXORW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32A_NOAMOXORW-NEXT:    li a1, 13
; RV32A_NOAMOXORW-NEXT:    call __sync_fetch_and_xor_4
; RV32A_NOAMOXORW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32A_NOAMOXORW-NEXT:    addi sp, sp, 16
; RV32A_NOAMOXORW-NEXT:    ret
