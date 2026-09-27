; EXPECT: 1

define i32 @main() {
entry:
  br label %loop

loop:
  %index = phi i32 [ 0, %entry ], [ %next.index, %latch ]
  %flag = phi i1 [ false, %entry ], [ %next.flag, %latch ]
  %condition = icmp slt i32 %index, 5
  br i1 %condition, label %body, label %exit

body:
  %next.flag = xor i1 %flag, true
  br label %latch

latch:
  %next.index = add i32 %index, 1
  br label %loop

exit:
  %result = zext i1 %flag to i32
  ret i32 %result
}
