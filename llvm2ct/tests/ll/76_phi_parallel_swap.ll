; EXPECT: 21

define i32 @main() {
entry:
  br label %header

header:
  %index = phi i32 [ 0, %entry ], [ %next, %latch ]
  %first = phi i32 [ 1, %entry ], [ %second, %latch ]
  %second = phi i32 [ 2, %entry ], [ %first, %latch ]
  %continue = icmp slt i32 %index, 3
  br i1 %continue, label %latch, label %exit

latch:
  %next = add i32 %index, 1
  br label %header

exit:
  %tens = mul i32 %first, 10
  %result = add i32 %tens, %second
  ret i32 %result
}
