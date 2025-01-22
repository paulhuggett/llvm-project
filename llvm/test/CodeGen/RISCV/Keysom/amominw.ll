; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_AMOMINW %s
; RUN: llc -mtriple=riscv32 -mattr=+a    \
; RUN:     -mattr=+xkeysomnoamominw < %s \
; RUN:   | FileCheck -check-prefix=RV32A_NOAMOMINW %s

define i32 @amominw(ptr %p) nounwind {
  %v = atomicrmw min ptr %p, i32 13 seq_cst, align 4
  ret i32 %v
}

; RV32A_AMOMINW-LABEL: amominw:
; RV32A_AMOMINW:       # %bb.0:
; RV32A_AMOMINW-NEXT:    li a1, 13
; RV32A_AMOMINW-NEXT:    amomin.w.aqrl a0, a1, (a0)
; RV32A_AMOMINW-NEXT:    ret

; RV32A_NOAMOMINW-LABEL: amominw:
; RV32A_MOAMOMINW:       # %bb.0:
; RV32A_MOAMOMINW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill 
; RV32A_MOAMOMINW-NEXT:    li a1, 13 
; RV32A_MOAMOMINW-NEXT:    call __sync_fetch_and_min_4 
; RV32A_MOAMOMINW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload 
; RV32A_MOAMOMINW-NEXT:    addi sp, sp, 16 
; RV32A_MOAMOMINW-NEXT:    ret 
