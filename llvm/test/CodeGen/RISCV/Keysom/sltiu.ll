; RUN: llc -mtriple=riscv32 -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32I %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+xkeysomnosltiu < %s \
; RUN:   | FileCheck -check-prefix=RV32I_NO %s

define i32 @f(i32 %x) nounwind {
; RV32I:        sltiu {{.+}}, {{.+}}, {{[0-9]+}}
; RV32I_NO-NOT: sltiu {{.+}}, {{.+}}, {{[0-9]+}}
entry:
  %cmp = icmp ult i32 %x, 43
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}
