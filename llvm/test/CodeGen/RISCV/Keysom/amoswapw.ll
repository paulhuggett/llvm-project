; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_AMOSWAPW %s
; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:     -mattr=+xkeysomnoamoswapw       \
; RUN:   | FileCheck -check-prefix=RV32A_NOAMOSWAPW %s

define i32 @swap(ptr %p) nounwind {
  %v = atomicrmw xchg ptr %p, i32 13 seq_cst, align 4
  ret i32 %v
}

; RV32A_AMOSWAPW-LABEL: swap:
; RV32A_AMOSWAPW:       # %bb.0:
; RV32A_AMOSWAPW-NEXT:    li a1, 13
; RV32A_AMOSWAPW-NEXT:    amoswap.w.aqrl a0, a1, (a0)
; RV32A_AMOSWAPW-NEXT:    ret

; RV32A_NOAMOSWAPW-LABEL: swap:
; RV32A_NOAMOSWAPW:       # %bb.0:
; RV32A_NOAMOSWAPW-NEXT:    addi sp, sp, -16
; RV32A_NOAMOSWAPW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32A_NOAMOSWAPW-NEXT:    li a1, 13
; RV32A_NOAMOSWAPW-NEXT:    call __sync_lock_test_and_set_4
; RV32A_NOAMOSWAPW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32A_NOAMOSWAPW-NEXT:    addi sp, sp, 16
; RV32A_NOAMOSWAPW-NEXT:    ret
