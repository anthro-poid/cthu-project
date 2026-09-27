; EXPECT: 16

define i32 @choose(i1 %condition, i32 %value) {
entry:
  br i1 %condition, label %left, label %right

left:
  br label %merge

right:
  br label %merge

merge:
  %selected = phi i32 [ %value, %left ], [ %value, %right ]
  %result = add i32 %selected, 1
  ret i32 %result
}

define i32 @main() {
entry:
  %first = call i32 @choose(i1 true, i32 7)
  %second = call i32 @choose(i1 false, i32 7)
  %result = add i32 %first, %second
  ret i32 %result
}
