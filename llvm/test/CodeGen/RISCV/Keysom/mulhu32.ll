; RUN: llc -mtriple=riscv32 -mattr=+m -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV32M_MULHU %s

; RUN: llc -mtriple=riscv32 -mattr=+m \
; RUN:     -mattr=+xkeysomnomulhu     \
; RUN:     -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV32M_NOMULHU %s

define signext i32 @multiply(i32 noundef %x, i32 noundef %y) nounwind {
  %x64 = zext i32 %x to i64
  %y64 = zext i32 %y to i64
  %mul = mul nuw i64 %y64, %x64
  %shr = lshr i64 %mul, 32
  %result = trunc nuw i64 %shr to i32
  ret i32 %result
}

; RV32M_MULHU-LABEL: multiply:
; RV32M_MULHU:       # %bb.0:
; RV32M_MULHU-NEXT:    mulhu a0, a1, a0
; RV32M_MULHU-NEXT:    ret

; RV32M_NOMULHU-LABEL: multiply:
; RV32M_NOMULHU:       # %bb.0:
; RV32M_NOMULHU-NEXT:    addi sp, sp, -16
; RV32M_NOMULHU-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32M_NOMULHU-NEXT:    mv a2, a0
; RV32M_NOMULHU-NEXT:    mv a0, a1
; RV32M_NOMULHU-NEXT:    li a1, 0
; RV32M_NOMULHU-NEXT:    li a3, 0
; RV32M_NOMULHU-NEXT:    call __muldi3
; RV32M_NOMULHU-NEXT:    mv a0, a1
; RV32M_NOMULHU-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32M_NOMULHU-NEXT:    addi sp, sp, 16
; RV32M_NOMULHU-NEXT:    ret
