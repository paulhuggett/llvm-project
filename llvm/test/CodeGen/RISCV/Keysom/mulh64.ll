; RUN: llc -mtriple=riscv64 -mattr=+m -mattr=-xkeysomnomulh -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_MULH %s
;
; RUN: llc -mtriple=riscv64 -mattr=+m -mattr=+xkeysomnomulh -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOMULH %s
;
; RUN: llc -mtriple=riscv64 -mattr=+m -mattr=+xkeysomnomulh -mattr=+xkeysomnomul -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOMULH_NOMUL %s
;
; RUN: llc -mtriple=riscv64 -mattr=+m -mattr=+xkeysomnomulh -mattr=+xkeysomnomulhu -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOMULH_NOMULHU %s

define signext i64 @multiply(i64 noundef %x, i64 noundef %y) nounwind {
  %x128 = sext i64 %x to i128
  %y128 = sext i64 %y to i128
  %mul = mul nsw i128 %y128, %x128
  %shr = lshr i128 %mul, 64
  %result = trunc nuw i128 %shr to i64
  ret i64 %result
}

; RV64M_MULH-LABEL: multiply:
; RV64M_MULH:       # %bb.0:
; RV64M_MULH-NEXT:    mulh a0, a1, a0
; RV64M_MULH-NEXT:    ret


; RV64M_NOMULH-LABEL: multiply:
; RV64M_NOMULH:       # %bb.0:
; RV64M_NOMULH-NEXT:    srai a2, a0, 63
; RV64M_NOMULH-NEXT:    srai a3, a1, 63
; RV64M_NOMULH-NEXT:    mulhu a4, a1, a0
; RV64M_NOMULH-NEXT:    mul a1, a1, a2
; RV64M_NOMULH-NEXT:    add a1, a4, a1
; RV64M_NOMULH-NEXT:    mul a0, a3, a0
; RV64M_NOMULH-NEXT:    add a0, a1, a0
; RV64M_NOMULH-NEXT:    ret


; RV64M_NOMULH_NOMUL-LABEL: multiply:
; RV64M_NOMULH_NOMUL:       # %bb.0:
; RV64M_NOMULH_NOMUL-NEXT:    addi sp, sp, -32
; RV64M_NOMULH_NOMUL-NEXT:    sd ra, 24(sp) # 8-byte Folded Spill
; RV64M_NOMULH_NOMUL-NEXT:    sd s0, 16(sp) # 8-byte Folded Spill
; RV64M_NOMULH_NOMUL-NEXT:    sd s1, 8(sp) # 8-byte Folded Spill
; RV64M_NOMULH_NOMUL-NEXT:    sd s2, 0(sp) # 8-byte Folded Spill
; RV64M_NOMULH_NOMUL-NEXT:    mv s0, a1
; RV64M_NOMULH_NOMUL-NEXT:    mv s1, a0
; RV64M_NOMULH_NOMUL-NEXT:    srai a1, a0, 63
; RV64M_NOMULH_NOMUL-NEXT:    srai s2, s0, 63
; RV64M_NOMULH_NOMUL-NEXT:    mv a0, s0
; RV64M_NOMULH_NOMUL-NEXT:    call __muldi3
; RV64M_NOMULH_NOMUL-NEXT:    mulhu a1, s0, s1
; RV64M_NOMULH_NOMUL-NEXT:    add s0, a1, a0
; RV64M_NOMULH_NOMUL-NEXT:    mv a0, s2
; RV64M_NOMULH_NOMUL-NEXT:    mv a1, s1
; RV64M_NOMULH_NOMUL-NEXT:    call __muldi3
; RV64M_NOMULH_NOMUL-NEXT:    add a0, s0, a0
; RV64M_NOMULH_NOMUL-NEXT:    ld ra, 24(sp) # 8-byte Folded Reload
; RV64M_NOMULH_NOMUL-NEXT:    ld s0, 16(sp) # 8-byte Folded Reload
; RV64M_NOMULH_NOMUL-NEXT:    ld s1, 8(sp) # 8-byte Folded Reload
; RV64M_NOMULH_NOMUL-NEXT:    ld s2, 0(sp) # 8-byte Folded Reload
; RV64M_NOMULH_NOMUL-NEXT:    addi sp, sp, 32
; RV64M_NOMULH_NOMUL-NEXT:    ret


; RV64M_NOMULH_NOMULHU-LABEL: multiply:
; RV64M_NOMULH_NOMULHU:       # %bb.0:
; RV64M_NOMULH_NOMULHU-NEXT:    addi sp, sp, -16
; RV64M_NOMULH_NOMULHU-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOMULH_NOMULHU-NEXT:    mv a4, a1
; RV64M_NOMULH_NOMULHU-NEXT:    mv a2, a0
; RV64M_NOMULH_NOMULHU-NEXT:    srai a3, a0, 63
; RV64M_NOMULH_NOMULHU-NEXT:    srai a1, a1, 63
; RV64M_NOMULH_NOMULHU-NEXT:    mv a0, a4
; RV64M_NOMULH_NOMULHU-NEXT:    call __multi3
; RV64M_NOMULH_NOMULHU-NEXT:    mv a0, a1
; RV64M_NOMULH_NOMULHU-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOMULH_NOMULHU-NEXT:    addi sp, sp, 16
; RV64M_NOMULH_NOMULHU-NEXT:    ret
