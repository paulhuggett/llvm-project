; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:   | FileCheck -check-prefix=RV32A_LRW %s
; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:     -mattr=+xkeysomnolrw        \
; RUN:   | FileCheck -check-prefix=RV32A_NOLRW_NOSCW %s
; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:     -mattr=+xkeysomnoscw        \
; RUN:   | FileCheck -check-prefix=RV32A_NOLRW_NOSCW %s
; RUN: llc -mtriple=riscv32 -mattr=+a < %s \
; RUN:     -mattr=+xkeysomnoscw -mattr=+xkeysomnolrw \
; RUN:   | FileCheck -check-prefix=RV32A_NOLRW_NOSCW %s

define void @cas(ptr nocapture noundef %p) nounwind {
entry:
  %0 = cmpxchg ptr %p, i32 0, i32 1 seq_cst seq_cst, align 4
  ret void
}

; RV32A_LRW-LABEL: cas:
; RV32A_LRW:       # %bb.0:
; RV32A_LRW-NEXT:    li a1, 1
; RV32A_LRW-NEXT:  .LBB0_1: # %entry
; RV32A_LRW-NEXT:  # =>This Inner Loop Header: Depth=1
; RV32A_LRW-NEXT:    lr.w.aqrl a2, (a0)
; RV32A_LRW-NEXT:    bnez a2, .LBB0_3
; RV32A_LRW-NEXT:  # %bb.2:
; RV32A_LRW-NEXT:  # in Loop: Header=BB0_1 Depth=1
; RV32A_LRW-NEXT:    sc.w.rl a3, a1, (a0)
; RV32A_LRW-NEXT:    bnez a3, .LBB0_1
; RV32A_LRW-NEXT:  .LBB0_3:
; RV32A_LRW-NEXT:    ret


; RV32A_NOLRW_NOSCW-LABEL: cas:
; RV32A_NOLRW_NOSCW:       # %bb.0:
; RV32A_NOLRW_NOSCW-NEXT:    addi sp, sp, -16
; RV32A_NOLRW_NOSCW-NEXT:    sw ra, 12(sp) # 4-byte Folded Spill
; RV32A_NOLRW_NOSCW-NEXT:    sw zero, 8(sp)
; RV32A_NOLRW_NOSCW-NEXT:    addi a1, sp, 8
; RV32A_NOLRW_NOSCW-NEXT:    li a2, 1
; RV32A_NOLRW_NOSCW-NEXT:    li a3, 5
; RV32A_NOLRW_NOSCW-NEXT:    li a4, 5
; RV32A_NOLRW_NOSCW-NEXT:    call __atomic_compare_exchange_4
; RV32A_NOLRW_NOSCW-NEXT:    lw ra, 12(sp) # 4-byte Folded Reload
; RV32A_NOLRW_NOSCW-NEXT:    addi sp, sp, 16
; RV32A_NOLRW_NOSCW-NEXT:    ret
