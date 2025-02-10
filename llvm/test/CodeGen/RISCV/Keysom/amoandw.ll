; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_AMOANDW %s
; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:     -mattr=+xkeysomnoamoandw        \
; RUN:   | FileCheck -check-prefix=RV32A_NOAMOANDW %s

define void @amoandw(ptr %p) nounwind {
  %v = atomicrmw and ptr %p, i32 13 seq_cst, align 4
  ret void
}


; RV32A_AMOANDW-LABEL: amoandw:
; RV32A_AMOANDW:       # %bb.0:
; RV32A_AMOANDW-NEXT:    li a1, 13
; RV32A_AMOANDW-NEXT:    amoand.w.aqrl zero, a1, (a0)
; RV32A_AMOANDW-NEXT:    ret

; RV32A_NOAMOANDW-LABEL: amoandw:
; RV32A_NOAMOANDW:       # %bb.0:
; RV32A_NOAMOANDW-NEXT:    addi sp, sp, -16
; RV32A_NOAMOANDW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32A_NOAMOANDW-NEXT:    li a1, 13
; RV32A_NOAMOANDW-NEXT:    call __sync_fetch_and_and_4
; RV32A_NOAMOANDW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32A_NOAMOANDW-NEXT:    addi sp, sp, 16
; RV32A_NOAMOANDW-NEXT:    ret
