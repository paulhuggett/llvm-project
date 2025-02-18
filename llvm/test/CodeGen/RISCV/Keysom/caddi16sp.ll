; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocaddi16sp < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

declare void @g(ptr noundef) local_unnamed_addr

define void @f() nounwind {
; RV32IC: c.addi16sp
; RV32IC_NO-NOT: c.addi16sp
entry:
  %mem = alloca [8 x i32], align 4
  call void @g(ptr noundef nonnull %mem) #3
  ret void
}
