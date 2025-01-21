; RUN: llc -mtriple=riscv32 -mattr=+m < %s \
; RUN:   | FileCheck -check-prefix=RV32M_REMU %s

; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnoremu < %s \
; RUN:   | FileCheck -check-prefix=RV32M_NOREMU %s

; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnoremu      \
; RUN:     -mattr=+xkeysomnodivu < %s \
; RUN:   | FileCheck -check-prefix=RV32M_NOREMU_NODIVU %s

; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnoremu      \
; RUN:     -mattr=+xkeysomnomul < %s \
; RUN:   | FileCheck -check-prefix=RV32M_NOREMU_NOMUL %s

; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnoremu      \
; RUN:     -mattr=+xkeysomnomul       \
; RUN:     -mattr=+xkeysomnodivu < %s \
; RUN:   | FileCheck -check-prefix=RV32M_NOREMU_NOMUL_NODIVU %s

define signext i32 @remainder(i32 noundef %x, i32 noundef %y) nounwind {
  %rem = urem i32 %x, %y
  ret i32 %rem
}

; RV32M_REMU-LABEL: remainder:
; RV32M_REMU:       # %bb.0:
; RV32M_REMU-NEXT:    remu a0, a0, a1
; RV32M_REMU-NEXT:    ret

; RV32M_NOREMU-LABEL: remainder:
; RV32M_NOREMU:       # %bb.0:
; RV32M_NOREMU-NEXT:    divu a2, a0, a1
; RV32M_NOREMU-NEXT:    mul a1, a2, a1
; RV32M_NOREMU-NEXT:    sub a0, a0, a1
; RV32M_NOREMU-NEXT:    ret

; RV32M_NOREMU_NODIVU-LABEL: remainder:
; RV32M_NOREMU_NODIVU:       # %bb.0:
; RV32M_NOREMU_NODIVU-NEXT:    addi sp, sp, -16
; RV32M_NOREMU_NODIVU-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOREMU_NODIVU-NEXT:    call __umodsi3
; RV32M_NOREMU_NODIVU-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOREMU_NODIVU-NEXT:    addi sp, sp, 16
; RV32M_NOREMU_NODIVU-NEXT:    ret

; RV32M_NOREMU_NOMUL-LABEL: remainder:
; RV32M_NOREMU_NOMUL:       # %bb.0:
; RV32M_NOREMU_NOMUL-NEXT:   addi sp, sp, -16
; RV32M_NOREMU_NOMUL-NEXT:   sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOREMU_NOMUL-NEXT:   sw s0, 8(sp) # 4-byte Folded Spill
; RV32M_NOREMU_NOMUL-NEXT:   mv s0, a0
; RV32M_NOREMU_NOMUL-NEXT:   divu a0, a0, a1
; RV32M_NOREMU_NOMUL-NEXT:   call __mulsi3
; RV32M_NOREMU_NOMUL-NEXT:   sub a0, s0, a0
; RV32M_NOREMU_NOMUL-NEXT:   lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOREMU_NOMUL-NEXT:   lw s0, 8(sp) # 4-byte Folded Reload
; RV32M_NOREMU_NOMUL-NEXT:   addi sp, sp, 16
; RV32M_NOREMU_NOMUL-NEXT:   ret

; RV32M_NOREMU_NOMUL_NODIVU-LABEL: remainder:
; RV32M_NOREMU_NOMUL_NODIVU:       # %bb.0:
; RV32M_NOREMU_NOMUL_NODIVU-NEXT:    addi sp, sp, -16
; RV32M_NOREMU_NOMUL_NODIVU-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOREMU_NOMUL_NODIVU-NEXT:    call __umodsi3
; RV32M_NOREMU_NOMUL_NODIVU-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOREMU_NOMUL_NODIVU-NEXT:    addi sp, sp, 16
; RV32M_NOREMU_NOMUL_NODIVU-NEXT:    ret
