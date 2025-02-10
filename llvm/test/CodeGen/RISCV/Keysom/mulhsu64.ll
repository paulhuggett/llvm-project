; RUN: llc -mtriple=riscv64 -mattr=+m -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_MULHSU %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhsu    \
; RUN:     -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOMULHSU %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhsu   \
; RUN:     -mattr=+xkeysomnomul      \
; RUN:     -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOMULHSU_NOMUL %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhsu    \
; RUN:     -mattr=+xkeysomnomulhu     \
; RUN:     -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOMULHSU_NOMULHU %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhsu    \
; RUN:     -mattr=+xkeysomnomulhu     \
; RUN:     -mattr=+xkeysomnomul       \
; RUN:     -verify-machineinstrs < %s \
; RUN:  | FileCheck -check-prefix=RV64M_NOMULHSU_NOMULHU_NOMUL %s

define signext i64 @multiply(i64 noundef %x, i64 noundef %y) nounwind {
  %x128 = sext i64 %x to i128
  %y128 = zext i64 %y to i128
  %mul = mul nsw i128 %y128, %x128
  %shr = lshr i128 %mul, 64
  %result = trunc nuw i128 %shr to i64
  ret i64 %result
}

; RV64M_MULHSU-LABEL: multiply:
; RV64M_MULHSU:       # %bb.0:
; RV64M_MULHSU-NEXT:    mulhsu a0, a0, a1
; RV64M_MULHSU-NEXT:    ret

; RV64M_NOMULHSU-LABEL: multiply:
; RV64M_NOMULHSU:       # %bb.0:
; RV64M_NOMULHSU-NEXT:    srai a2, a0, 63
; RV64M_NOMULHSU-NEXT:    mulhu a0, a1, a0
; RV64M_NOMULHSU-NEXT:    mul a1, a1, a2
; RV64M_NOMULHSU-NEXT:    add a0, a0, a1
; RV64M_NOMULHSU-NEXT:    ret

; RV64M_NOMULHSU_NOMUL-LABEL: multiply:
; RV64M_NOMULHSU_NOMUL:       # %bb.0:
; RV64M_NOMULHSU_NOMUL-NEXT:    addi sp, sp, -32
; RV64M_NOMULHSU_NOMUL-NEXT:    sd ra, 24(sp) # 8-byte Folded Spill
; RV64M_NOMULHSU_NOMUL-NEXT:    sd s0, 16(sp) # 8-byte Folded Spill
; RV64M_NOMULHSU_NOMUL-NEXT:    sd s1, 8(sp) # 8-byte Folded Spill
; RV64M_NOMULHSU_NOMUL-NEXT:    mv s0, a1
; RV64M_NOMULHSU_NOMUL-NEXT:    mv s1, a0
; RV64M_NOMULHSU_NOMUL-NEXT:    srai a1, a0, 63
; RV64M_NOMULHSU_NOMUL-NEXT:    mv a0, s0
; RV64M_NOMULHSU_NOMUL-NEXT:    call __muldi3
; RV64M_NOMULHSU_NOMUL-NEXT:    mulhu a1, s0, s1
; RV64M_NOMULHSU_NOMUL-NEXT:    add a0, a1, a0
; RV64M_NOMULHSU_NOMUL-NEXT:    ld ra, 24(sp) # 8-byte Folded Reload
; RV64M_NOMULHSU_NOMUL-NEXT:    ld s0, 16(sp) # 8-byte Folded Reload
; RV64M_NOMULHSU_NOMUL-NEXT:    ld s1, 8(sp) # 8-byte Folded Reload
; RV64M_NOMULHSU_NOMUL-NEXT:    addi sp, sp, 32
; RV64M_NOMULHSU_NOMUL-NEXT:    ret

; RV64M_NOMULHSU_NOMULHU-LABEL: multiply:
; RV64M_NOMULHSU_NOMULHU:       # %bb.0:
; RV64M_NOMULHSU_NOMULHU-NEXT:    addi sp, sp, -16
; RV64M_NOMULHSU_NOMULHU-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOMULHSU_NOMULHU-NEXT:    mv a2, a0
; RV64M_NOMULHSU_NOMULHU-NEXT:    srai a3, a0, 63
; RV64M_NOMULHSU_NOMULHU-NEXT:    mv a0, a1
; RV64M_NOMULHSU_NOMULHU-NEXT:    li a1, 0
; RV64M_NOMULHSU_NOMULHU-NEXT:    call __multi3
; RV64M_NOMULHSU_NOMULHU-NEXT:    mv a0, a1
; RV64M_NOMULHSU_NOMULHU-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOMULHSU_NOMULHU-NEXT:    addi sp, sp, 16
; RV64M_NOMULHSU_NOMULHU-NEXT:    ret

; RV64M_NOMULHSU_NOMULHU_NOMUL-LABEL: multiply:
; RV64M_NOMULHSU_NOMULHU_NOMUL:       # %bb.0:
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    addi sp, sp, -16
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    mv a2, a0
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    srai a3, a0, 63
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    mv a0, a1
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    li a1, 0
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    call __multi3
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    mv a0, a1
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    addi sp, sp, 16
; RV64M_NOMULHSU_NOMULHU_NOMUL-NEXT:    ret

