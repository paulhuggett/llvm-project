; RUN: llc -mtriple=riscv32 -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+xkeysomnoori < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f(i32 %a) {
; RV32IC:        ori {{.+}}, {{.+}}, {{[0-9]+}}
; RV32IC_NO-NOT: c.xor {{.+}}, {{.+}}
  %1 = or i32 %a, 1
  ret i32 %1
}
