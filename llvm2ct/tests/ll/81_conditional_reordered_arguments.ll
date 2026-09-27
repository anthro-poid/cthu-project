; EXPECT: 81

define i32 @choose(i1 %condition, i32 %first, i32 %second) {
entry:
  br i1 %condition, label %ordered, label %reordered

ordered:
  %ordered.result = add i32 %first, %second
  ret i32 %ordered.result

reordered:
  %high = phi i32 [ %second, %entry ]
  %low = phi i32 [ %first, %entry ]
  %tens = mul i32 %high, 10
  %reordered.result = add i32 %tens, %low
  ret i32 %reordered.result
}

define i32 @main() {
entry:
  %ordered = call i32 @choose(i1 true, i32 2, i32 7)
  %reordered = call i32 @choose(i1 false, i32 2, i32 7)
  %result = add i32 %ordered, %reordered
  ret i32 %result
}
