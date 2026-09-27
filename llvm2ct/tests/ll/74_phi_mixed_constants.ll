; EXPECT: 351

define i32 @choose(i1 %condition) {
entry:
  br i1 %condition, label %left, label %right

left:
  br label %merge

right:
  br label %merge

merge:
  %small = phi i8 [ 100, %left ], [ 250, %right ]
  %flag = phi i1 [ true, %left ], [ false, %right ]
  %wide = zext i8 %small to i32
  %bit = zext i1 %flag to i32
  %result = add i32 %wide, %bit
  ret i32 %result
}

define i32 @main() {
entry:
  %left = call i32 @choose(i1 true)
  %right = call i32 @choose(i1 false)
  %result = add i32 %left, %right
  ret i32 %result
}
