; RUN: llc -mtriple=riscv32 -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefixes=RV32I,RV32I_k %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+xkeysomnoslli < %s \
; RUN:   | FileCheck -check-prefixes=RV32I_NO,RV32I_k_NOSLLI %s

define i32 @f(i32 %x) nounwind {
; RV32I-LABEL: f:
; RV32I:        slli {{.+}}, {{.+}}, {{[0-9]+}}
; RV32I_NO-NOT: slli {{.+}}, {{.+}}, {{[0-9]+}}
entry:
  %shl = shl i32 %x, 3
  ret i32 %shl
}

; The compiler can transform expressions of the form (x << C1) op C2 (where op
; is ANDI/ORI/XORI), into (x op (C2>>C1)) << C1.
define i32 @k(i32 %x) nounwind {
; RV32I_k-LABEL: k:
; RV32I_k: slli a0, a0, 4

; RV32I_k_NOSLLI-LABEL: k:
; RV32I_k_NOSLLI-NOT: slli {{.+}}, {{.+}}, {{-?[0-9]+}}
entry:
  %shl = shl i32 %x, 4
  %xor = xor i32 %shl, 4080
  ret i32 %xor
}
