; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocmv < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f(i32 %a, ptr %b) {
; RV32IC:        c.mv
; RV32IC_NO-NOT: c.mv
  %val1 = load volatile i32, ptr %b
  %tst1 = icmp eq i32 0, %val1
  %val2 = select i1 %tst1, i32 %a, i32 %val1
  ret i32 %val2
}
