; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_AMOMAXUW %s
; RUN: llc -mtriple=riscv32 -mattr=+a    \
; RUN:     -mattr=+xkeysomnoamomaxuw < %s \
; RUN:   | FileCheck -check-prefix=RV32A_NOAMOMAXUW %s

define i32 @amomaxuw(ptr %p) nounwind {
  %v = atomicrmw umax ptr %p, i32 13 seq_cst, align 4
  ret i32 %v
}

; RV32A_AMOMAXUW-LABEL: amomaxuw:
; RV32A_AMOMAXUW:       # %bb.0:
; RV32A_AMOMAXUW-NEXT:    li a1, 13
; RV32A_AMOMAXUW-NEXT:    amomaxu.w.aqrl a0, a1, (a0)
; RV32A_AMOMAXUW-NEXT:    ret

; RV32A_NOAMOMAXUW-LABEL: amomaxuw:
; RV32A_MOAMOMAXUW:       # %bb.0:
; RV32A_MOAMOMAXUW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32A_MOAMOMAXUW-NEXT:    li a1, 13
; RV32A_MOAMOMAXUW-NEXT:    call __sync_fetch_and_max_4
; RV32A_MOAMOMAXUW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32A_MOAMOMAXUW-NEXT:    addi sp, sp, 16
; RV32A_MOAMOMAXUW-NEXT:    ret
