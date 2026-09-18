; EXPECT: 1

define i32 @choose(i1 %condition) {
entry:
  br i1 %condition, label %branch_frame0, label %other

branch_frame0:
  ret i32 1

other:
  ret i32 2
}

define i32 @main() {
entry:
  %condition = icmp eq i32 1, 1
  %result = call i32 @choose(i1 %condition)
  ret i32 %result
}
