; EXPECT: 5

define i32 @main() {
entry:
  br label %body

body:
  %index = phi i32 [ 0, %entry ], [ %next, %condition ]
  %next = add i32 %index, 1
  br label %condition

condition:
  %continue = icmp slt i32 %next, 5
  br i1 %continue, label %body, label %exit

exit:
  ret i32 %next
}
