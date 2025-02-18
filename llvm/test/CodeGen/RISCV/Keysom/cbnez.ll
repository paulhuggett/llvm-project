; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocbnez < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NOCBNEZ %s

define i32 @ne_medium_ledge_pos(i32 %in0) minsize {
; RV32IC: c.bnez
; RV32IC_NOCBNEZ-NOT: c.bnez
entry:
  %cmp = icmp ne i32 %in0, 33
  %res = select i1 %cmp, i32 -99, i32 42
  ret i32 %res
}
