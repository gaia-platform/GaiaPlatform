; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -instsimplify -S | FileCheck %s

define i32 @zero_dividend(i32 %A) {
; CHECK-LABEL: @zero_dividend(
; CHECK-NEXT:    ret i32 0
;
  %B = urem i32 0, %A
  ret i32 %B
}

define <2 x i32> @zero_dividend_vector(<2 x i32> %A) {
; CHECK-LABEL: @zero_dividend_vector(
; CHECK-NEXT:    ret <2 x i32> zeroinitializer
;
  %B = srem <2 x i32> zeroinitializer, %A
  ret <2 x i32> %B
}

define <2 x i32> @zero_dividend_vector_undef_elt(<2 x i32> %A) {
; CHECK-LABEL: @zero_dividend_vector_undef_elt(
; CHECK-NEXT:    ret <2 x i32> zeroinitializer
;
  %B = urem <2 x i32> <i32 undef, i32 0>, %A
  ret <2 x i32> %B
}

; Division-by-zero is undef. UB in any vector lane means the whole op is undef.

define <2 x i8> @srem_zero_elt_vec_constfold(<2 x i8> %x) {
; CHECK-LABEL: @srem_zero_elt_vec_constfold(
; CHECK-NEXT:    ret <2 x i8> undef
;
  %rem = srem <2 x i8> <i8 1, i8 2>, <i8 0, i8 -42>
  ret <2 x i8> %rem
}

define <2 x i8> @urem_zero_elt_vec_constfold(<2 x i8> %x) {
; CHECK-LABEL: @urem_zero_elt_vec_constfold(
; CHECK-NEXT:    ret <2 x i8> undef
;
  %rem = urem <2 x i8> <i8 1, i8 2>, <i8 42, i8 0>
  ret <2 x i8> %rem
}

define <2 x i8> @srem_zero_elt_vec(<2 x i8> %x) {
; CHECK-LABEL: @srem_zero_elt_vec(
; CHECK-NEXT:    ret <2 x i8> undef
;
  %rem = srem <2 x i8> %x, <i8 -42, i8 0>
  ret <2 x i8> %rem
}

define <2 x i8> @urem_zero_elt_vec(<2 x i8> %x) {
; CHECK-LABEL: @urem_zero_elt_vec(
; CHECK-NEXT:    ret <2 x i8> undef
;
  %rem = urem <2 x i8> %x, <i8 0, i8 42>
  ret <2 x i8> %rem
}

define <2 x i8> @srem_undef_elt_vec(<2 x i8> %x) {
; CHECK-LABEL: @srem_undef_elt_vec(
; CHECK-NEXT:    ret <2 x i8> undef
;
  %rem = srem <2 x i8> %x, <i8 -42, i8 undef>
  ret <2 x i8> %rem
}

define <2 x i8> @urem_undef_elt_vec(<2 x i8> %x) {
; CHECK-LABEL: @urem_undef_elt_vec(
; CHECK-NEXT:    ret <2 x i8> undef
;
  %rem = urem <2 x i8> %x, <i8 undef, i8 42>
  ret <2 x i8> %rem
}

; Division-by-zero is undef. UB in any vector lane means the whole op is undef.
; Thus, we can simplify this: if any element of 'y' is 0, we can do anything.
; Therefore, assume that all elements of 'y' must be 1.

define <2 x i1> @srem_bool_vec(<2 x i1> %x, <2 x i1> %y) {
; CHECK-LABEL: @srem_bool_vec(
; CHECK-NEXT:    ret <2 x i1> zeroinitializer
;
  %rem = srem <2 x i1> %x, %y
  ret <2 x i1> %rem
}

define <2 x i1> @urem_bool_vec(<2 x i1> %x, <2 x i1> %y) {
; CHECK-LABEL: @urem_bool_vec(
; CHECK-NEXT:    ret <2 x i1> zeroinitializer
;
  %rem = urem <2 x i1> %x, %y
  ret <2 x i1> %rem
}

define <2 x i32> @zext_bool_urem_divisor_vec(<2 x i1> %x, <2 x i32> %y) {
; CHECK-LABEL: @zext_bool_urem_divisor_vec(
; CHECK-NEXT:    ret <2 x i32> zeroinitializer
;
  %ext = zext <2 x i1> %x to <2 x i32>
  %r = urem <2 x i32> %y, %ext
  ret <2 x i32> %r
}

define i32 @zext_bool_srem_divisor(i1 %x, i32 %y) {
; CHECK-LABEL: @zext_bool_srem_divisor(
; CHECK-NEXT:    ret i32 0
;
  %ext = zext i1 %x to i32
  %r = srem i32 %y, %ext
  ret i32 %r
}

define i32 @select1(i32 %x, i1 %b) {
; CHECK-LABEL: @select1(
; CHECK-NEXT:    ret i32 0
;
  %rhs = select i1 %b, i32 %x, i32 1
  %rem = srem i32 %x, %rhs
  ret i32 %rem
}

define i32 @select2(i32 %x, i1 %b) {
; CHECK-LABEL: @select2(
; CHECK-NEXT:    ret i32 0
;
  %rhs = select i1 %b, i32 %x, i32 1
  %rem = urem i32 %x, %rhs
  ret i32 %rem
}

define i32 @rem1(i32 %x, i32 %n) {
; CHECK-LABEL: @rem1(
; CHECK-NEXT:    [[MOD:%.*]] = srem i32 [[X:%.*]], [[N:%.*]]
; CHECK-NEXT:    ret i32 [[MOD]]
;
  %mod = srem i32 %x, %n
  %mod1 = srem i32 %mod, %n
  ret i32 %mod1
}

define i32 @rem2(i32 %x, i32 %n) {
; CHECK-LABEL: @rem2(
; CHECK-NEXT:    [[MOD:%.*]] = urem i32 [[X:%.*]], [[N:%.*]]
; CHECK-NEXT:    ret i32 [[MOD]]
;
  %mod = urem i32 %x, %n
  %mod1 = urem i32 %mod, %n
  ret i32 %mod1
}

define i32 @rem3(i32 %x, i32 %n) {
; CHECK-LABEL: @rem3(
; CHECK-NEXT:    [[MOD:%.*]] = srem i32 [[X:%.*]], [[N:%.*]]
; CHECK-NEXT:    [[MOD1:%.*]] = urem i32 [[MOD]], [[N]]
; CHECK-NEXT:    ret i32 [[MOD1]]
;
  %mod = srem i32 %x, %n
  %mod1 = urem i32 %mod, %n
  ret i32 %mod1
}

define i32 @urem_dividend_known_smaller_than_constant_divisor(i32 %x) {
; CHECK-LABEL: @urem_dividend_known_smaller_than_constant_divisor(
; CHECK-NEXT:    [[AND:%.*]] = and i32 [[X:%.*]], 250
; CHECK-NEXT:    ret i32 [[AND]]
;
  %and = and i32 %x, 250
  %r = urem i32 %and, 251
  ret i32 %r
}

define i32 @not_urem_dividend_known_smaller_than_constant_divisor(i32 %x) {
; CHECK-LABEL: @not_urem_dividend_known_smaller_than_constant_divisor(
; CHECK-NEXT:    [[AND:%.*]] = and i32 [[X:%.*]], 251
; CHECK-NEXT:    [[R:%.*]] = urem i32 [[AND]], 251
; CHECK-NEXT:    ret i32 [[R]]
;
  %and = and i32 %x, 251
  %r = urem i32 %and, 251
  ret i32 %r
}

define i32 @urem_constant_dividend_known_smaller_than_divisor(i32 %x) {
; CHECK-LABEL: @urem_constant_dividend_known_smaller_than_divisor(
; CHECK-NEXT:    ret i32 250
;
  %or = or i32 %x, 251
  %r = urem i32 250, %or
  ret i32 %r
}

define i32 @not_urem_constant_dividend_known_smaller_than_divisor(i32 %x) {
; CHECK-LABEL: @not_urem_constant_dividend_known_smaller_than_divisor(
; CHECK-NEXT:    [[OR:%.*]] = or i32 [[X:%.*]], 251
; CHECK-NEXT:    [[R:%.*]] = urem i32 251, [[OR]]
; CHECK-NEXT:    ret i32 [[R]]
;
  %or = or i32 %x, 251
  %r = urem i32 251, %or
  ret i32 %r
}

; This would require computing known bits on both x and y. Is it worth doing?

define i32 @urem_dividend_known_smaller_than_divisor(i32 %x, i32 %y) {
; CHECK-LABEL: @urem_dividend_known_smaller_than_divisor(
; CHECK-NEXT:    [[AND:%.*]] = and i32 [[X:%.*]], 250
; CHECK-NEXT:    [[OR:%.*]] = or i32 [[Y:%.*]], 251
; CHECK-NEXT:    [[R:%.*]] = urem i32 [[AND]], [[OR]]
; CHECK-NEXT:    ret i32 [[R]]
;
  %and = and i32 %x, 250
  %or = or i32 %y, 251
  %r = urem i32 %and, %or
  ret i32 %r
}

define i32 @not_urem_dividend_known_smaller_than_divisor(i32 %x, i32 %y) {
; CHECK-LABEL: @not_urem_dividend_known_smaller_than_divisor(
; CHECK-NEXT:    [[AND:%.*]] = and i32 [[X:%.*]], 251
; CHECK-NEXT:    [[OR:%.*]] = or i32 [[Y:%.*]], 251
; CHECK-NEXT:    [[R:%.*]] = urem i32 [[AND]], [[OR]]
; CHECK-NEXT:    ret i32 [[R]]
;
  %and = and i32 %x, 251
  %or = or i32 %y, 251
  %r = urem i32 %and, %or
  ret i32 %r
}

declare i32 @external()

define i32 @rem4() {
; CHECK-LABEL: @rem4(
; CHECK-NEXT:    [[CALL:%.*]] = call i32 @external(), !range !0
; CHECK-NEXT:    ret i32 [[CALL]]
;
  %call = call i32 @external(), !range !0
  %urem = urem i32 %call, 3
  ret i32 %urem
}

!0 = !{i32 0, i32 3}

define i32 @rem5(i32 %x, i32 %y) {
; CHECK-LABEL: @rem5(
; CHECK-NEXT:    ret i32 0
;
  %shl = shl nsw i32 %x, %y
  %mod = srem i32 %shl, %x
  ret i32 %mod
}

define <2 x i32> @rem6(<2 x i32> %x, <2 x i32> %y) {
; CHECK-LABEL: @rem6(
; CHECK-NEXT:    ret <2 x i32> zeroinitializer
;
  %shl = shl nsw <2 x i32> %x, %y
  %mod = srem <2 x i32> %shl, %x
  ret <2 x i32> %mod
}

; make sure the previous fold doesn't take place for wrapped shifts

define i32 @rem7(i32 %x, i32 %y) {
; CHECK-LABEL: @rem7(
; CHECK-NEXT:    [[SHL:%.*]] = shl i32 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[MOD:%.*]] = srem i32 [[SHL]], [[X]]
; CHECK-NEXT:    ret i32 [[MOD]]
;
  %shl = shl i32 %x, %y
  %mod = srem i32 %shl, %x
  ret i32 %mod
}

define i32 @rem8(i32 %x, i32 %y) {
; CHECK-LABEL: @rem8(
; CHECK-NEXT:    ret i32 0
;
  %shl = shl nuw i32 %x, %y
  %mod = urem i32 %shl, %x
  ret i32 %mod
}

define <2 x i32> @rem9(<2 x i32> %x, <2 x i32> %y) {
; CHECK-LABEL: @rem9(
; CHECK-NEXT:    ret <2 x i32> zeroinitializer
;
  %shl = shl nuw <2 x i32> %x, %y
  %mod = urem <2 x i32> %shl, %x
  ret <2 x i32> %mod
}

; make sure the previous fold doesn't take place for wrapped shifts

define i32 @rem10(i32 %x, i32 %y) {
; CHECK-LABEL: @rem10(
; CHECK-NEXT:    [[SHL:%.*]] = shl i32 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[MOD:%.*]] = urem i32 [[SHL]], [[X]]
; CHECK-NEXT:    ret i32 [[MOD]]
;
  %shl = shl i32 %x, %y
  %mod = urem i32 %shl, %x
  ret i32 %mod
}

define i32 @srem_with_sext_bool_divisor(i1 %x, i32 %y) {
; CHECK-LABEL: @srem_with_sext_bool_divisor(
; CHECK-NEXT:    ret i32 0
;
  %s = sext i1 %x to i32
  %r = srem i32 %y, %s
  ret i32 %r
}

define <2 x i32> @srem_with_sext_bool_divisor_vec(<2 x i1> %x, <2 x i32> %y) {
; CHECK-LABEL: @srem_with_sext_bool_divisor_vec(
; CHECK-NEXT:    ret <2 x i32> zeroinitializer
;
  %s = sext <2 x i1> %x to <2 x i32>
  %r = srem <2 x i32> %y, %s
  ret <2 x i32> %r
}

