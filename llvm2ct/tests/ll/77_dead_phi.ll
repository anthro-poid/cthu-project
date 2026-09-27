; EXPECT: 10

define i32 @choose(i1 %condition, i32 %value) {
entry:
  br i1 %condition, label %left, label %right

left:
  %left.live = add i32 %value, 1
  %left.dead = add i32 %value, 100
  br label %merge

right:
  %right.live = sub i32 %value, 1
  %right.dead = add i32 %value, 200
  br label %merge

merge:
  %live = phi i32 [ %left.live, %left ], [ %right.live, %right ]
  %dead = phi i32 [ %left.dead, %left ], [ %right.dead, %right ]
  ret i32 %live
}

define i32 @main() {
entry:
  %left = call i32 @choose(i1 true, i32 5)
  %right = call i32 @choose(i1 false, i32 5)
  %result = add i32 %left, %right
  ret i32 %result
}
