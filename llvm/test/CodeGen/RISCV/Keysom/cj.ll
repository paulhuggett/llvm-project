; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s

; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocj < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCJ %s

@a = external global i32, align 4

; Function Attrs: noinline nounwind optnone
define void @cj() nounwind {
; RV32IC-LABEL: cj:
; RV32IC:       # %bb.0:
; RV32IC-NEXT:    lui a0, %hi(a)
; RV32IC-NEXT:    c.li a1, 1
; RV32IC-NEXT:  .LBB0_1:
; RV32IC-NEXT:    # =>This Inner Loop Header: Depth=1
; RV32IC-NEXT:    sw a1, %lo(a)(a0)
; RV32IC-NEXT:    c.j .LBB0_1

; RV32IC_NOCJ-LABEL: cj:
; RV32IC_NOCJ:       # %bb.0:
; RV32IC_NOCJ-NEXT:    lui a0, %hi(a)
; RV32IC_NOCJ-NEXT:    c.li a1, 1
; RV32IC_NOCJ-NEXT:  .LBB0_1:
; RV32IC_NOCJ-NEXT:    # =>This Inner Loop Header: Depth=1
; RV32IC_NOCJ-NEXT:    sw a1, %lo(a)(a0)
; RV32IC_NOCJ-NEXT:    jal zero, .LBB0_1

entry:
  br label %loop
loop:                                               ; preds = %loop, %entry
  store volatile i32 1, ptr @a, align 4
  br label %loop
}

