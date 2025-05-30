# REQUIRES: x86
# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-darwin %t/foo1.s -o %t/foo1.o
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-darwin %t/foo2.s -o %t/foo2.o
# RUN: %lld -dylib --icf=all -framework CoreFoundation %t/foo1.o %t/foo2.o -o %t/foo
# RUN: llvm-objdump --no-print-imm-hex --macho --rebase --bind --syms -d %t/foo | FileCheck %s --check-prefixes=CHECK,LITERALS
# RUN: %lld -dylib -framework CoreFoundation %t/foo1.o %t/foo2.o -o %t/foo
# RUN: llvm-objdump --no-print-imm-hex --macho --rebase --bind --syms -d %t/foo | FileCheck %s --check-prefix=LITERALS

# Check that string deduplication for symbol names is working
# RUN: %lld -dylib -framework CoreFoundation %t/foo1.o %t/foo2.o -o %t/foo_no_dedup -no-deduplicate-symbol-strings
# RUN: llvm-strings %t/foo | FileCheck %s --check-prefix=CHECK-DEDUP
# RUN: llvm-strings %t/foo_no_dedup | FileCheck %s --check-prefix=CHECK-NO-DEDUP
# CHECK-DEDUP: _named_cfstring
# CHECK-DEDUP-NOT: _named_cfstring
# CHECK-NO-DEDUP: _named_cfstring
# CHECK-NO-DEDUP: _named_cfstring
# CHECK-NO-DEDUP-NOT: _named_cfstring


# CHECK:       (__TEXT,__text) section
# CHECK-NEXT:  _foo1:
# CHECK-NEXT:  _foo2:
# CHECK-NEXT:       movq    _named_cfstring(%rip), %rax
# CHECK-NEXT:  _foo1_utf16:
# CHECK-NEXT:       movq    [[#]](%rip), %rax
# CHECK-NEXT:  _named_foo1:
# CHECK-NEXT:  _named_foo2:
# CHECK-NEXT:       movq    _named_cfstring(%rip), %rax
# CHECK-NEXT:  _foo2_utf16:
# CHECK-NEXT:       movq    [[#]](%rip), %rax

# CHECK:       SYMBOL TABLE:
# CHECK-DAG:   [[#%.16x,FOO:]] g     F __TEXT,__text _foo1
# CHECK-DAG:   [[#FOO]]        g     F __TEXT,__text _foo2

## Make sure we don't emit redundant bind / rebase opcodes for folded sections.
# LITERALS:       Rebase table:
# LITERALS-NEXT:  segment  section          address  type
# LITERALS-NEXT:  __DATA_CONST __cfstring   {{.*}}   pointer
# LITERALS-NEXT:  __DATA_CONST __cfstring   {{.*}}   pointer
# LITERALS-NEXT:  __DATA_CONST __cfstring   {{.*}}   pointer
# LITERALS-EMPTY:
# LITERALS-NEXT:  Bind table:
# LITERALS-NEXT:  segment      section      address  type       addend dylib            symbol
# LITERALS-NEXT:  __DATA_CONST __cfstring   {{.*}}   pointer         0 CoreFoundation   ___CFConstantStringClassReference
# LITERALS-NEXT:  __DATA_CONST __cfstring   {{.*}}   pointer         0 CoreFoundation   ___CFConstantStringClassReference
# LITERALS-NEXT:  __DATA_CONST __cfstring   {{.*}}   pointer         0 CoreFoundation   ___CFConstantStringClassReference
# LITERALS-EMPTY:

#--- foo1.s
.cstring
L_.str.0:
  .asciz  "bar"
## This string is at a different offset than the corresponding "foo" string in
## foo2.s. Make sure that we treat references to either string as equivalent.
L_.str:
  .asciz  "foo"

.section  __DATA,__cfstring
.p2align  3
L__unnamed_cfstring_:
  .quad  ___CFConstantStringClassReference
  .long  1992 ## utf-8
  .space  4
  .quad  L_.str
  .quad  3 ## strlen

_named_cfstring:
  .quad  ___CFConstantStringClassReference
  .long  1992 ## utf-8
  .space  4
  .quad  L_.str
  .quad  3 ## strlen

.section  __TEXT,__ustring
l_.ustr:
  .short  102 ## f
  .short  111 ## o
  .short  0   ## \0
  .short  111 ## o
  .short  0   ## \0

## FIXME: We should be able to deduplicate UTF-16 CFStrings too.
## Note that this string contains a null byte in the middle -- any dedup code
## we add should take care to handle this correctly.
## Technically, UTF-8 should support encoding null bytes too, but since we
## atomize the __cstring section at every null byte, this isn't supported. ld64
## doesn't support it either, and clang seems to always emit a UTF-16 CFString
## if it needs to contain a null, so I think we're good here.
.section  __DATA,__cfstring
.p2align  3
L__unnamed_cfstring_.2:
  .quad  ___CFConstantStringClassReference
  .long  2000 ## utf-16
  .space  4
  .quad  l_.ustr
  .quad  4 ## strlen

.text
.globl  _foo1, _foo1_utf16, _named_foo1
_foo1:
  movq L__unnamed_cfstring_(%rip), %rax

_foo1_utf16:
  movq L__unnamed_cfstring_.2(%rip), %rax

_named_foo1:
  movq _named_cfstring(%rip), %rax

.subsections_via_symbols

#--- foo2.s
.cstring
L_.str:
  .asciz  "foo"

.section  __DATA,__cfstring
.p2align  3
L__unnamed_cfstring_:
  .quad  ___CFConstantStringClassReference
  .long  1992 ## utf-8
  .space  4
  .quad  L_.str
  .quad  3 ## strlen

_named_cfstring:
  .quad  ___CFConstantStringClassReference
  .long  1992 ## utf-8
  .space  4
  .quad  L_.str
  .quad  3 ## strlen

.section  __TEXT,__ustring
  .p2align  1
l_.ustr:
  .short  102 ## f
  .short  111 ## o
  .short  0   ## \0
  .short  111 ## o
  .short  0   ## \0

.section  __DATA,__cfstring
.p2align  3
L__unnamed_cfstring_.2:
  .quad  ___CFConstantStringClassReference
  .long  2000 ## utf-16
  .space  4
  .quad  l_.ustr
  .quad  4 ## strlen

.text
.globl  _foo2, _foo2_utf16, _named_foo2
_foo2:
  movq L__unnamed_cfstring_(%rip), %rax

_foo2_utf16:
  movq L__unnamed_cfstring_.2(%rip), %rax

_named_foo2:
  movq _named_cfstring(%rip), %rax

.subsections_via_symbols
