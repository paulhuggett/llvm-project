; Compile to bitcode with the M extension disabled. At link-time the M extension
; is explicitly enabled and we check the the output uses the 'mul' instruction.

; REQUIRES: riscv
; RUN: opt -mtriple=riscv32 -mattr=-m -verify-machineinstrs -o %t.bc < %s
; RUN: llvm-dis -o - %t.bc | FileCHECK -check-prefix=OPT %s
; RUN: ld.lld -mllvm -mattr=+m %t.bc -o %t.bin
; RUN: llvm-objdump -d %t.bin | FileCheck -check-prefix=LINK %s

define signext i32 @_start(i32 %a) nounwind {
  %1 = mul i32 %a, %a
  ret i32 %1
}

; OPT: attributes #0 = { nounwind "target-features"="-m" }

; LINK: <_start>:
; LINK-NEXT: mul a0, a0, a0
; LINK-NEXT: ret
