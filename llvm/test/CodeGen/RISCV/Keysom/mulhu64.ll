; RUN: llc -mtriple=riscv64 -mattr=+m -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_MULHU %s

; RUN: llc -mtriple=riscv64 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhu     \
; RUN:     -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64M_NOMULHU %s

define signext i64 @multiply(i64 noundef %x, i64 noundef %y) nounwind {
  %x128 = zext i64 %x to i128
  %y128 = zext i64 %y to i128
  %mul = mul nuw i128 %y128, %x128
  %shr = lshr i128 %mul, 64
  %result = trunc nuw i128 %shr to i64
  ret i64 %result
}

; RV64M_MULHU-LABEL: multiply:
; RV64M_MULHU:       # %bb.0:
; RV64M_MULHU-NEXT:    mulhu a0, a1, a0
; RV64M_MULHU-NEXT:    ret

; RV64M_NOMULHU-LABEL: multiply:
; RV64M_NOMULHU:       # %bb.0:
; RV64M_NOMULHU-NEXT:    addi sp, sp, -16
; RV64M_NOMULHU-NEXT:    sd ra, 8(sp) # 8-byte Folded Spill
; RV64M_NOMULHU-NEXT:    mv a2, a0
; RV64M_NOMULHU-NEXT:    mv a0, a1
; RV64M_NOMULHU-NEXT:    li a1, 0
; RV64M_NOMULHU-NEXT:    li a3, 0
; RV64M_NOMULHU-NEXT:    call __multi3
; RV64M_NOMULHU-NEXT:    mv a0, a1
; RV64M_NOMULHU-NEXT:    ld ra, 8(sp) # 8-byte Folded Reload
; RV64M_NOMULHU-NEXT:    addi sp, sp, 16
; RV64M_NOMULHU-NEXT:    ret
