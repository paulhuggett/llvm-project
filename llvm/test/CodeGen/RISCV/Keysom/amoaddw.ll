; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_AMOADDW %s
; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:     -mattr=+xkeysomnoamoaddw        \
; RUN:   | FileCheck -check-prefix=RV32A_NOAMOADDW %s

define void @amoaddw(ptr nocapture noundef %x) nounwind {
entry:
  %0 = atomicrmw add ptr %x, i32 1 seq_cst, align 4
  ret void
}

; RV32A_AMOADDW-LABEL: amoaddw:
; RV32A_AMOADDW:       # %bb.0:
; RV32A_AMOADDW-NEXT:    li a1, 1
; RV32A_AMOADDW-NEXT:    amoadd.w.aqrl zero, a1, (a0)
; RV32A_AMOADDW-NEXT:    ret

; RV32A_NOAMOADDW-LABEL: amoaddw:
; RV32A_NOAMOADDW:       # %bb.0
; RV32A_NOAMOADDW-NEXT:    addi sp, sp, -16
; RV32A_NOAMOADDW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32A_NOAMOADDW-NEXT:    li a1, 1
; RV32A_NOAMOADDW-NEXT:    call __sync_fetch_and_add_4
; RV32A_NOAMOADDW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32A_NOAMOADDW-NEXT:    addi sp, sp, 16
; RV32A_NOAMOADDW-NEXT:    ret
