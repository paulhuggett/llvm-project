; RUN: llc -mtriple=riscv32 -riscv-no-aliases < %s \
; RUN:   | FileCheck -check-prefix=RV32IC %s
;
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+xkeysomnolui < %s \
; RUN:   | FileCheck -check-prefix=RV32IC_NO %s

define i32 @f() {
; RV32IC:        lui {{.+}}, {{[0-9]+}}
; RV32IC_NO-NOT: lui {{.+}}, {{[0-9]+}}
  ret i32 4096
}
