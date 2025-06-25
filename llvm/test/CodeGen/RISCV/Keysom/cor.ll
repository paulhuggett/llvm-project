; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -mattr=+c -riscv-no-aliases -mattr=+xkeysomnocor < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f(i32 %a, i32 %b) {
; RV32IC:        c.or {{.+}}, {{.+}}
; RV32IC_NO-NOT: c.or {{.+}}, {{.+}}
  %1 = or i32 %a, %b
  ret i32 %1
}
