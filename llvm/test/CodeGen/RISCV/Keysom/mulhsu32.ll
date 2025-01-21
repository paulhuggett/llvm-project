; RUN: llc -mtriple=riscv32 -mattr=+m -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV32M_MULHSU %s
; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhsu    \
; RUN:     -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV32M_NOMULHSU %s
; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhsu    \
; RUN:     -mattr=+xkeysomnomul       \
; RUN:     -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV32M_NOMULHSU_NOMUL %s
; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhsu    \
; RUN:     -mattr=+xkeysomnomulhu     \
; RUN:     -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV32M_NOMULHSU_NOMULHU %s
; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhsu    \
; RUN:     -mattr=+xkeysomnomul       \
; RUN:     -mattr=+xkeysomnomulhu     \
; RUN:     -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV32M_NOMULHSU_NOMUL_NOMULHU %s

define signext i32 @multiply(i32 noundef %x, i32 noundef %y) nounwind {
  %x64 = sext i32 %x to i64
  %y64 = zext i32 %y to i64
  %mul = mul nsw i64 %y64, %x64
  %shr = lshr i64 %mul, 32
  %result = trunc nuw i64 %shr to i32
  ret i32 %result
}

; RV32M_MULHSU-LABEL: multiply:
; RV32M_MULHSU:       # %bb.0:
; RV32M_MULHSU-NEXT:    mulhsu a0, a0, a1
; RV32M_MULHSU-NEXT:    ret

; RV32M_NOMULHSU-LABEL: multiply:
; RV32M_NOMULHSU:       # %bb.0:
; RV32M_NOMULHSU-NEXT:    srai a2, a0, 31
; RV32M_NOMULHSU-NEXT:    mulhu a0, a1, a0
; RV32M_NOMULHSU-NEXT:    mul a1, a1, a2
; RV32M_NOMULHSU-NEXT:    add a0, a0, a1
; RV32M_NOMULHSU-NEXT:    ret

; RV32M_NOMULHSU_NOMUL-LABEL: multiply:
; RV32M_NOMULHSU_NOMUL:       # %bb.0:
; RV32M_NOMULHSU_NOMUL-NEXT:    addi sp, sp, -16
; RV32M_NOMULHSU_NOMUL-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOMULHSU_NOMUL-NEXT:    sw s0, 8(sp) # 4-byte Folded Spill
; RV32M_NOMULHSU_NOMUL-NEXT:    sw s1, 4(sp) # 4-byte Folded Spill
; RV32M_NOMULHSU_NOMUL-NEXT:    mv s0, a1
; RV32M_NOMULHSU_NOMUL-NEXT:    mv s1, a0
; RV32M_NOMULHSU_NOMUL-NEXT:    srai a1, a0, 31
; RV32M_NOMULHSU_NOMUL-NEXT:    mv a0, s0
; RV32M_NOMULHSU_NOMUL-NEXT:    call __mulsi3
; RV32M_NOMULHSU_NOMUL-NEXT:    mulhu a1, s0, s1
; RV32M_NOMULHSU_NOMUL-NEXT:    add a0, a1, a0
; RV32M_NOMULHSU_NOMUL-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOMULHSU_NOMUL-NEXT:    lw s0, 8(sp) # 4-byte Folded Reload
; RV32M_NOMULHSU_NOMUL-NEXT:    lw s1, 4(sp) # 4-byte Folded Reload
; RV32M_NOMULHSU_NOMUL-NEXT:    addi sp, sp, 16
; RV32M_NOMULHSU_NOMUL-NEXT:    ret

; RV32M_NOMULHSU_NOMULHU-LABEL: multiply:
; RV32M_NOMULHSU_NOMULHU:       # %bb.0:
; RV32M_NOMULHSU_NOMULHU-NEXT:    addi sp, sp, -16
; RV32M_NOMULHSU_NOMULHU-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOMULHSU_NOMULHU-NEXT:    mv a2, a0
; RV32M_NOMULHSU_NOMULHU-NEXT:    srai a3, a0, 31
; RV32M_NOMULHSU_NOMULHU-NEXT:    mv a0, a1
; RV32M_NOMULHSU_NOMULHU-NEXT:    li a1, 0
; RV32M_NOMULHSU_NOMULHU-NEXT:    call __muldi3
; RV32M_NOMULHSU_NOMULHU-NEXT:    mv a0, a1
; RV32M_NOMULHSU_NOMULHU-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOMULHSU_NOMULHU-NEXT:    addi sp, sp, 16
; RV32M_NOMULHSU_NOMULHU-NEXT:    ret

; RV32M_NOMULHSU_NOMUL_NOMULHU-LABEL: multiply:
; RV32M_NOMULHSU_NOMUL_NOMULHU:       # %bb.0:
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    addi sp, sp, -16
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    mv a2, a0
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    srai a3, a0, 31
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    mv a0, a1
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    li a1, 0
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    call __muldi3
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    mv a0, a1
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    addi sp, sp, 16
; RV32M_NOMULHSU_NOMUL_NOMULHU-NEXT:    ret
