; RUN: llc -mtriple=riscv32 -mattr=+m < %s \
; RUN:   | FileCheck -check-prefix=RV32M_REM %s
; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnorem < %s  \
; RUN:   | FileCheck -check-prefix=RV32M_NOREM %s
; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnorem       \
; RUN:     -mattr=+xkeysomnodiv < %s  \
; RUN:   | FileCheck -check-prefix=RV32M_NOREM_NODIV %s
; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnorem       \
; RUN:     -mattr=+xkeysomnomul < %s  \
; RUN:   | FileCheck -check-prefix=RV32M_NOREM_NOMUL %s
; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnorem       \
; RUN:     -mattr=+xkeysomnodiv       \
; RUN:     -mattr=+xkeysomnomul < %s  \
; RUN:   | FileCheck -check-prefix=RV32M_NOREM_NODIV_NOMUL %s

define signext i32 @remainder(i32 noundef %x, i32 noundef %y) nounwind {
  %rem = srem i32 %x, %y
  ret i32 %rem
}

; RV32M_REM-LABEL: remainder:
; RV32M_REM:       # %bb.0:
; RV32M_REM-NEXT:    rem a0, a0, a1
; RV32M_REM-NEXT:    ret

; RV32M_NOREM-LABEL: remainder:
; RV32M_NOREM:       # %bb.0:
; RV32M_NOREM-NEXT:    div a2, a0, a1
; RV32M_NOREM-NEXT:    mul a1, a2, a1
; RV32M_NOREM-NEXT:    sub a0, a0, a1
; RV32M_NOREM-NEXT:    ret

; RV32M_NOREM_NODIV-LABEL: remainder:
; RV32M_NOREM_NODIV:       # %bb.0:
; RV32M_NOREM_NODIV-NEXT:    addi sp, sp, -16
; RV32M_NOREM_NODIV-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOREM_NODIV-NEXT:    call __modsi3
; RV32M_NOREM_NODIV-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOREM_NODIV-NEXT:    addi sp, sp, 16
; RV32M_NOREM_NODIV-NEXT:    ret

; RV32M_NOREM_NOMUL-LABEL: remainder:
; RV32M_NOREM_NOMUL:       # %bb.0:
; RV32M_NOREM_NOMUL-NEXT:    addi sp, sp, -16
; RV32M_NOREM_NOMUL-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOREM_NOMUL-NEXT:    sw s0, 8(sp) # 4-byte Folded Spill
; RV32M_NOREM_NOMUL-NEXT:    mv s0, a0
; RV32M_NOREM_NOMUL-NEXT:    div a0, a0, a1
; RV32M_NOREM_NOMUL-NEXT:    call __mulsi3
; RV32M_NOREM_NOMUL-NEXT:    sub a0, s0, a0
; RV32M_NOREM_NOMUL-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOREM_NOMUL-NEXT:    lw s0, 8(sp) # 4-byte Folded Reload
; RV32M_NOREM_NOMUL-NEXT:    addi sp, sp, 16
; RV32M_NOREM_NOMUL-NEXT:    ret

; RV32M_NOREM_NODIV_NOMUL-LABEL: remainder:
; RV32M_NOREM_NODIV_NOMUL:       # %bb.0:
; RV32M_NOREM_NODIV_NOMUL-NEXT:    addi sp, sp, -16
; RV32M_NOREM_NODIV_NOMUL-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOREM_NODIV_NOMUL-NEXT:    call __modsi3
; RV32M_NOREM_NODIV_NOMUL-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOREM_NODIV_NOMUL-NEXT:    addi sp, sp, 16
; RV32M_NOREM_NODIV_NOMUL-NEXT:    ret

