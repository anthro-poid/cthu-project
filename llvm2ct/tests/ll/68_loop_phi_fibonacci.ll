; EXPECT: 13

define i32 @main() {
entry:
  br label %loop

loop:
  %remaining = phi i32 [ 7, %entry ], [ %next.remaining, %latch ]
  %previous = phi i32 [ 0, %entry ], [ %current, %latch ]
  %current = phi i32 [ 1, %entry ], [ %next, %latch ]
  %condition = icmp sgt i32 %remaining, 0
  br i1 %condition, label %body, label %exit

body:
  %next = add i32 %previous, %current
  br label %latch

latch:
  %next.remaining = sub i32 %remaining, 1
  br label %loop

exit:
  ret i32 %previous
}
