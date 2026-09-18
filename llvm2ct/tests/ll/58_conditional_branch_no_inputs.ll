; EXPECT: 11

define i32 @choose() {
entry:
  %condition = icmp eq i32 1, 2
  br i1 %condition, label %true, label %false

true:
  ret i32 7

false:
  ret i32 11
}

define i32 @main() {
entry:
  %result = call i32 @choose()
  ret i32 %result
}
