;; Checks that ensure that we do get an xori when the disabling flag is not
;; supplied.
;
; RUN: llc -mtriple=riscv32 -riscv-no-aliases < %s              \
; RUN:   | FileCheck -check-prefixes=RV32I_f,RV32I_g,RV32I_j,RV32I_k %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -global-isel < %s \
; RUN:   | FileCheck -check-prefixes=RV32I_f,RV32I_g,RV32I_j %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+a < %s    \
; RUN:   | FileCheck -check-prefixes=RV32IA_j %s

; RUN: llc -mtriple=riscv32 -riscv-no-aliases -mattr=+f < %s    \
; RUN:   | FileCheck -check-prefixes=RV32IF_m %s

;; Now without xori.
;
; RUN: llc -mtriple=riscv32 -riscv-no-aliases      \
; RUN:     -mattr=+xkeysomnoxori < %s              \
; RUN:   | FileCheck -check-prefixes=RV32I_f_NOXORI,RV32I_g_NOXORI,RV32I_k_NOXORI %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases      \
; RUN:     -global-isel -mattr=+xkeysomnoxori < %s \
; RUN:   | FileCheck -check-prefixes=RV32I_f_NOXORI,RV32I_g_NOXORI,RV32I_k_NOXORI,RV32IF_m_NOXORI %s
; RUN: llc -mtriple=riscv32 -riscv-no-aliases      \
; RUN:     -mattr=+a -mattr=+xkeysomnoxori < %s    \
; RUN:   | FileCheck -check-prefixes=RV32IA_j %s

; NO_XORI-NOT: xori {{.+}}, {{.+}}, {{-?[0-9]+}}
define i32 @f(i32 %x) nounwind {
; RV32I_f-LABEL: f:
; RV32I_f: xori a0, a0, 43
; RV32I_f_NOXORI-LABEL: f:
; RV32I_f_NOXORI-NOT: xori {{.+}}, {{.+}}, {{-?[0-9]+}}
entry:
  %xor = xor i32 %x, 43
  ret i32 %xor
}

define i32 @g(i32 %x, i32 %y) nounwind {
; RV32I_g-LABEL: g:
; RV32I_g: xori {{.+}}, {{.+}}, {{-?[0-9]+}}

; RV32I_g_NOXORI-LABEL: g:
; RV32I_g_NOXORI-NOT: xori {{.+}}, {{.+}}, {{-?[0-9]+}}
entry:
  %sub = sub i32 31, %y
  %shl = shl i32 %x, %sub
  ret i32 %shl
}

define i8 @j(ptr %ptr, i8 %v1, i16 %v2, i32 %v4, i64 %v8) nounwind {
; RV32I_j-LABEL: j:
; RV32I_j: call __atomic_fetch_nand_1
; RV32I_j: call __atomic_fetch_nand_2
; RV32I_j: call __atomic_fetch_nand_4
; RV32I_j: call __atomic_fetch_nand_8

; RV32IA_j-LABEL: j:
; RV32IA_j: xori {{.+}}, {{.+}}, {{-?[0-9]+}}
entry:
  %0 = atomicrmw nand ptr %ptr, i8 %v1 seq_cst, align 1
  %1 = atomicrmw nand ptr %ptr, i16 %v2 seq_cst, align 2
  %2 = atomicrmw nand ptr %ptr, i32 %v4 seq_cst, align 4
  %3 = atomicrmw nand ptr %ptr, i64 %v8 seq_cst, align 8
  ret i8 %0
}

; The compiler can transform expressions of the form (x << C1) op C2 (where op
; is ANDI/ORI/XORI), into (x op (C2>>C1)) << C1.
define i32 @k(i32 %x) nounwind {
; RV32I_k-LABEL: k:
; RV32I_k: xori a0, a0, 255

; RV32I_k_NOXORI-LABEL: k:
; RV32I_k_NOXORI-NOT: xori {{.+}}, {{.+}}, {{-?[0-9]+}}
entry:
  %shl = shl i32 %x, 4
  %xor = xor i32 %shl, 4080
  ret i32 %xor
}

define i32 @m(float %a) nounwind "target-features"="+f" {
; RV32IF_m-LABEL: m:
; RV32IF_m: xori {{.+}}, {{.+}}, {{-?[0-9]+}}

; RV32IF_m_NOXORI-LABEL: m:
; RV32IF_m_NOXORI-NOT: xori {{.+}}, {{.+}}, {{-?[0-9]+}}
entry:
  %cmp = fcmp ule float %a, 0.000000e+00
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}
