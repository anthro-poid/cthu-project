; EXPECT: 10

define i32 @main() {
entry:
  br label %loop

loop:
  %sum = phi i32 [ 0, %entry ], [ %next.sum, %latch ]
  %index = phi i32 [ 0, %entry ], [ %next.index, %latch ]
  %condition = icmp slt i32 %index, 5
  br i1 %condition, label %body, label %exit

body:
  %next.sum = add i32 %sum, %index
  br label %latch

latch:
  %next.index = add i32 %index, 1
  br label %loop

exit:
  ret i32 %sum
}
