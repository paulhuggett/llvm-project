; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -passes=newgvn -enable-store-refinement -S | FileCheck %s
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"

;; Both of these tests are tests of phi nodes that end up all equivalent to each other
;; Without proper leader ordering, we will end up cycling the leader between all of them and never converge.

define void @foo(i1 %arg, i1 %arg2) {
; CHECK-LABEL: @foo(
; CHECK-NEXT:  bb:
; CHECK-NEXT:    br label [[BB1:%.*]]
; CHECK:       bb1:
; CHECK-NEXT:    [[TMP:%.*]] = phi i32 [ 0, [[BB:%.*]] ], [ 1, [[BB18:%.*]] ]
; CHECK-NEXT:    br label [[BB2:%.*]]
; CHECK:       bb2:
; CHECK-NEXT:    br label [[BB4:%.*]]
; CHECK:       bb4:
; CHECK-NEXT:    br i1 [[ARG:%.*]], label [[BB18]], label [[BB7:%.*]]
; CHECK:       bb7:
; CHECK-NEXT:    br label [[BB9:%.*]]
; CHECK:       bb9:
; CHECK-NEXT:    br i1 [[ARG]], label [[BB2]], label [[BB11:%.*]]
; CHECK:       bb11:
; CHECK-NEXT:    br i1 [[ARG2:%.*]], label [[BB16:%.*]], label [[BB14:%.*]]
; CHECK:       bb14:
; CHECK-NEXT:    br label [[BB4]]
; CHECK:       bb16:
; CHECK-NEXT:    br label [[BB7]]
; CHECK:       bb18:
; CHECK-NEXT:    br label [[BB1]]
;
bb:
  br label %bb1

bb1:                                              ; preds = %bb18, %bb
  %tmp = phi i32 [ 0, %bb ], [ 1, %bb18 ]
  br label %bb2

bb2:                                              ; preds = %bb9, %bb1
  %tmp3 = phi i32 [ %tmp, %bb1 ], [ %tmp8, %bb9 ]
  br label %bb4

bb4:                                              ; preds = %bb14, %bb2
  %tmp5 = phi i32 [ %tmp3, %bb2 ], [ %tmp15, %bb14 ]
  br i1 %arg, label %bb18, label %bb7

bb7:                                              ; preds = %bb16, %bb4
  %tmp8 = phi i32 [ %tmp17, %bb16 ], [ %tmp5, %bb4 ]
  br label %bb9

bb9:                                              ; preds = %bb7
  br i1 %arg, label %bb2, label %bb11

bb11:                                             ; preds = %bb9
  br i1 %arg2, label %bb16, label %bb14

bb14:                                             ; preds = %bb11
  %tmp15 = phi i32 [ %tmp8, %bb11 ]
  br label %bb4

bb16:                                             ; preds = %bb11
  %tmp17 = phi i32 [ %tmp8, %bb11 ]
  br label %bb7

bb18:                                             ; preds = %bb4
  br label %bb1
}

%struct.a = type {}
%struct.b = type {}

declare void @c.d.p(i64, ptr)

define void @e(i32 %a0, i32 %a1, ptr %p2) {
; CHECK-LABEL: @e(
; CHECK-NEXT:    [[F:%.*]] = alloca i32, align 4
; CHECK-NEXT:    store i32 [[A0:%.*]], ptr [[F]], align 4, !g [[META0:![0-9]+]]
; CHECK-NEXT:    br label [[H:%.*]]
; CHECK:       h:
; CHECK-NEXT:    call void @c.d.p(i64 8, ptr undef)
; CHECK-NEXT:    [[I:%.*]] = load i32, ptr [[F]], align 4
; CHECK-NEXT:    [[J:%.*]] = load i32, ptr null, align 4
; CHECK-NEXT:    [[K:%.*]] = icmp eq i32 [[I]], [[J]]
; CHECK-NEXT:    br i1 [[K]], label [[L:%.*]], label [[Q:%.*]]
; CHECK:       l:
; CHECK-NEXT:    br label [[R:%.*]]
; CHECK:       q:
; CHECK-NEXT:    br label [[R]]
; CHECK:       r:
; CHECK-NEXT:    switch i32 undef, label [[N:%.*]] [
; CHECK-NEXT:      i32 0, label [[S:%.*]]
; CHECK-NEXT:    ]
; CHECK:       s:
; CHECK-NEXT:    store i32 [[A1:%.*]], ptr [[F]], align 4, !g [[META0]]
; CHECK-NEXT:    br label [[H]]
; CHECK:       n:
; CHECK-NEXT:    ret void
;
  %f = alloca i32
  store i32 %a0, ptr %f, !g !0
  br label %h

h:                                                ; preds = %s, %0
  call void @c.d.p(i64 8, ptr undef)
  %i = load i32, ptr %f
  %j = load i32, ptr null
  %k = icmp eq i32 %i, %j
  br i1 %k, label %l, label %q

l:                                                ; preds = %h
  br label %r

q:                                                ; preds = %h
  %m = load ptr, ptr null
  br label %r

r:                                                ; preds = %q, %l
  switch i32 undef, label %n [
  i32 0, label %s
  ]

s:                                                ; preds = %r
  store i32 %a1, ptr %f, !g !0
  br label %h

n:                                                ; preds = %r
  %o = load ptr, ptr %p2
  ret void
}

!0 = !{}
