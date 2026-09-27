; EXPECT: 99

define i32 @choose(i1 %condition, i32 %first, i32 %second) {
entry:
  br i1 %condition, label %left, label %right

left:
  br label %merge

right:
  br label %merge

merge:
  %high = phi i32 [ %first, %left ], [ %second, %right ]
  %low = phi i32 [ %second, %left ], [ %first, %right ]
  %tens = mul i32 %high, 10
  %result = add i32 %tens, %low
  ret i32 %result
}

define i32 @main() {
entry:
  %left = call i32 @choose(i1 true, i32 2, i32 7)
  %right = call i32 @choose(i1 false, i32 2, i32 7)
  %result = add i32 %left, %right
  ret i32 %result
}
