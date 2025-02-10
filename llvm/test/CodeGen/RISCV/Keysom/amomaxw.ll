; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_AMOMAXW %s
; RUN: llc -mtriple=riscv32 -mattr=+a    \
; RUN:     -mattr=+xkeysomnoamomaxw < %s \
; RUN:   | FileCheck -check-prefix=RV32A_NOAMOMAXW %s

define i32 @amomaxw(ptr %p) nounwind {
  %v = atomicrmw max ptr %p, i32 13 seq_cst, align 4
  ret i32 %v
}

; RV32A_AMOMAXW-LABEL: amomaxw:
; RV32A_AMOMAXW:       # %bb.0:
; RV32A_AMOMAXW-NEXT:    li a1, 13
; RV32A_AMOMAXW-NEXT:    amomax.w.aqrl a0, a1, (a0)
; RV32A_AMOMAXW-NEXT:    ret

; RV32A_NOAMOMAXW-LABEL: amomaxw:
; RV32A_MOAMOMAXW:       # %bb.0:
; RV32A_MOAMOMAXW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32A_MOAMOMAXW-NEXT:    li a1, 13
; RV32A_MOAMOMAXW-NEXT:    call __sync_fetch_and_max_4
; RV32A_MOAMOMAXW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32A_MOAMOMAXW-NEXT:    addi sp, sp, 16
; RV32A_MOAMOMAXW-NEXT:    ret
