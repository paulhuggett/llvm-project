; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocand < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f(i32 %a, i32 %b) {
; RV32IC:        c.and {{.+}}, {{.+}}
; RV32IC_NO-NOT: c.and {{.+}}, {{.+}}
  %1 = and i32 %a, %b
  ret i32 %1
}
