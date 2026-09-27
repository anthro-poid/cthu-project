; EXPECT: 22

define i32 @main() {
entry:
  br label %header

header:
  %index = phi i32 [ 0, %entry ], [ %even.next, %even ], [ %odd.next, %odd ]
  %sum = phi i32 [ 0, %entry ], [ %even.sum, %even ], [ %odd.sum, %odd ]
  %continue = icmp slt i32 %index, 4
  br i1 %continue, label %body, label %exit

body:
  %masked = and i32 %index, 1
  %is.even = icmp eq i32 %masked, 0
  br i1 %is.even, label %even, label %odd

even:
  %even.sum = add i32 %sum, 10
  %even.next = add i32 %index, 1
  br label %header

odd:
  %odd.sum = add i32 %sum, 1
  %odd.next = add i32 %index, 1
  br label %header

exit:
  ret i32 %sum
}
