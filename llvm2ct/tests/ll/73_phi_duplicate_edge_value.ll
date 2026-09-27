; EXPECT: 27

define i32 @choose(i1 %condition, i32 %value, i32 %other) {
entry:
  br i1 %condition, label %left, label %right

left:
  br label %merge

right:
  %adjusted = add i32 %other, 1
  br label %merge

merge:
  %first = phi i32 [ %value, %left ], [ %adjusted, %right ]
  %second = phi i32 [ %value, %left ], [ %other, %right ]
  %result = add i32 %first, %second
  ret i32 %result
}

define i32 @main() {
entry:
  %left = call i32 @choose(i1 true, i32 5, i32 8)
  %right = call i32 @choose(i1 false, i32 5, i32 8)
  %result = add i32 %left, %right
  ret i32 %result
}
