; RUN: llc -mtriple=riscv64 -mattr=+m < %s \
; RUN:   | FileCheck -check-prefix=RV64M_REMU %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnoremu < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOREMU %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnoremu      \
; RUN:     -mattr=+xkeysomnodivu < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOREMU_NODIVU %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnoremu      \
; RUN:     -mattr=+xkeysomnomul < %s  \
; RUN:   | FileCheck -check-prefix=RV64M_NOREMU_NOMUL %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnoremu      \
; RUN:     -mattr=+xkeysomnomul       \
; RUN:     -mattr=+xkeysomnodivu < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOREMU_NOMUL_NODIVU %s

define signext i64 @remainder(i64 noundef %x, i64 noundef %y) nounwind {
  %rem = urem i64 %x, %y
  ret i64 %rem
}

; RV64M_REMU-LABEL: remainder:
; RV64M_REMU:       # %bb.0:
; RV64M_REMU-NEXT:    remu a0, a0, a1
; RV64M_REMU-NEXT:    ret

; RV64M_NOREMU-LABEL: remainder:
; RV64M_NOREMU:       # %bb.0:
; RV64M_NOREMU-NEXT:    divu a2, a0, a1
; RV64M_NOREMU-NEXT:    mul a1, a2, a1
; RV64M_NOREMU-NEXT:    sub a0, a0, a1
; RV64M_NOREMU-NEXT:    ret

; RV64M_NOREMU_NODIVU-LABEL: remainder:
; RV64M_NOREMU_NODIVU:       # %bb.0:
; RV64M_NOREMU_NODIVU-NEXT:    addi sp, sp, -16
; RV64M_NOREMU_NODIVU-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOREMU_NODIVU-NEXT:    call __umoddi3
; RV64M_NOREMU_NODIVU-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOREMU_NODIVU-NEXT:    addi sp, sp, 16
; RV64M_NOREMU_NODIVU-NEXT:    ret

; RV64M_NOREMU_NOMUL-LABEL: remainder:
; RV64M_NOREMU_NOMUL:       # %bb.0:
; RV64M_NOREMU_NOMUL-NEXT:    addi sp, sp, -16
; RV64M_NOREMU_NOMUL-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOREMU_NOMUL-NEXT:    sd s0, 0(sp) # 8-byte Folded Spill
; RV64M_NOREMU_NOMUL-NEXT:    mv s0, a0
; RV64M_NOREMU_NOMUL-NEXT:    divu a0, a0, a1
; RV64M_NOREMU_NOMUL-NEXT:    call __muldi3
; RV64M_NOREMU_NOMUL-NEXT:    sub a0, s0, a0
; RV64M_NOREMU_NOMUL-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOREMU_NOMUL-NEXT:    ld s0, 0(sp) # 8-byte Folded Reload
; RV64M_NOREMU_NOMUL-NEXT:    addi sp, sp, 16
; RV64M_NOREMU_NOMUL-NEXT:    ret

; RV64M_NOREMU_NOMUL_NODIVU-LABEL: remainder:
; RV64M_NOREMU_NOMUL_NODIVU:       # %bb.0:
; RV64M_NOREMU_NOMUL_NODIVU-NEXT:    addi sp, sp, -16
; RV64M_NOREMU_NOMUL_NODIVU-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOREMU_NOMUL_NODIVU-NEXT:    call __umoddi3
; RV64M_NOREMU_NOMUL_NODIVU-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOREMU_NOMUL_NODIVU-NEXT:    addi sp, sp, 16
; RV64M_NOREMU_NOMUL_NODIVU-NEXT:    ret
