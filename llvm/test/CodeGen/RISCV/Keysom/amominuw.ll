; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_AMOMINUW %s
; RUN: llc -mtriple=riscv32 -mattr=+a    \
; RUN:     -mattr=+xkeysomnoamominw < %s \
; RUN:   | FileCheck -check-prefix=RV32A_NOAMOMINUW %s

define i32 @amominuw(ptr %p) nounwind {
  %v = atomicrmw umin ptr %p, i32 13 seq_cst, align 4
  ret i32 %v
}

; RV32A_AMOMINUW-LABEL: amominuw:
; RV32A_AMOMINUW:       # %bb.0:
; RV32A_AMOMINUW-NEXT:    li a1, 13
; RV32A_AMOMINUW-NEXT:    amominu.w.aqrl a0, a1, (a0)
; RV32A_AMOMINUW-NEXT:    ret

; RV32A_NOAMOMINUW-LABEL: amominuw:
; RV32A_MOAMOMINUW:       # %bb.0:
; RV32A_MOAMOMINUW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32A_MOAMOMINUW-NEXT:    li a1, 13
; RV32A_MOAMOMINUW-NEXT:    call __sync_fetch_and_min_4
; RV32A_MOAMOMINUW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32A_MOAMOMINUW-NEXT:    addi sp, sp, 16
; RV32A_MOAMOMINUW-NEXT:    ret
