; RUN: llc -mtriple=riscv64 -mattr=+m < %s \
; RUN:   | FileCheck -check-prefix=RV64M_REM %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnorem < %s  \
; RUN:   | FileCheck -check-prefix=RV64M_NOREM %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnorem       \
; RUN:     -mattr=+xkeysomnodiv < %s  \
; RUN:   | FileCheck -check-prefix=RV64M_NOREM_NODIV %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnorem       \
; RUN:     -mattr=+xkeysomnomul < %s  \
; RUN:   | FileCheck -check-prefix=RV64M_NOREM_NOMUL %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnorem       \
; RUN:     -mattr=+xkeysomnomul       \
; RUN:     -mattr=+xkeysomnodiv < %s  \
; RUN:   | FileCheck -check-prefix=RV64M_NOREM_NOMUL_NODIV %s

define signext i64 @remainder(i64 noundef %x, i64 noundef %y) nounwind {
  %rem = srem i64 %x, %y
  ret i64 %rem
}

; RV64M_REM-LABEL: remainder:
; RV64M_REM:       # %bb.0:
; RV64M_REM-NEXT:    rem a0, a0, a1
; RV64M_REM-NEXT:    ret

; RV64M_NOREM-LABEL: remainder:
; RV64M_NOREM:       # %bb.0:
; RV64M_NOREM-NEXT:    div a2, a0, a1
; RV64M_NOREM-NEXT:    mul a1, a2, a1
; RV64M_NOREM-NEXT:    sub a0, a0, a1
; RV64M_NOREM-NEXT:    ret

; RV64M_NOREM_NODIV-LABEL: remainder:
; RV64M_NOREM_NODIV:       # %bb.0:
; RV64M_NOREM_NODIV-NEXT:    addi sp, sp, -16
; RV64M_NOREM_NODIV-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOREM_NODIV-NEXT:    call __moddi3
; RV64M_NOREM_NODIV-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOREM_NODIV-NEXT:    addi sp, sp, 16
; RV64M_NOREM_NODIV-NEXT:    ret

; RV64M_NOREM_NOMUL-LABEL: remainder:
; RV64M_NOREM_NOMUL:       # %bb.0:
; RV64M_NOREM_NOMUL-NEXT:    addi sp, sp, -16
; RV64M_NOREM_NOMUL-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOREM_NOMUL-NEXT:    sd s0, 0(sp) # 8-byte Folded Spill
; RV64M_NOREM_NOMUL-NEXT:    mv s0, a0
; RV64M_NOREM_NOMUL-NEXT:    div a0, a0, a1
; RV64M_NOREM_NOMUL-NEXT:    call __muldi3
; RV64M_NOREM_NOMUL-NEXT:    sub a0, s0, a0
; RV64M_NOREM_NOMUL-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOREM_NOMUL-NEXT:    ld s0, 0(sp) # 8-byte Folded Reload
; RV64M_NOREM_NOMUL-NEXT:    addi sp, sp, 16
; RV64M_NOREM_NOMUL-NEXT:    ret

; RV64M_NOREM_NOMUL_NODIV-LABEL: remainder:
; RV64M_NOREM_NOMUL_NODIV:       # %bb.0:
; RV64M_NOREM_NOMUL_NODIV-NEXT:    addi sp, sp, -16
; RV64M_NOREM_NOMUL_NODIV-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOREM_NOMUL_NODIV-NEXT:    call __moddi3
; RV64M_NOREM_NOMUL_NODIV-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOREM_NOMUL_NODIV-NEXT:    addi sp, sp, 16
; RV64M_NOREM_NOMUL_NODIV-NEXT:    ret
