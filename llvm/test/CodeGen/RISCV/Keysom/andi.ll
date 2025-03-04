; RUN: llc -mtriple=riscv32 -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32I %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+xkeysomnoandi < %s \
; RUN:   | FileCheck -check-prefix=RV32I_NO %s

define i32 @f(i32 %x) nounwind {
; RV32I:        andi {{.+}}, {{.+}}, {{[0-9]+}}
; RV32I_NO-NOT: andi {{.+}}, {{.+}}, {{[0-9]+}}
entry:
  %and = and i32 %x, 43
  ret i32 %and
}
