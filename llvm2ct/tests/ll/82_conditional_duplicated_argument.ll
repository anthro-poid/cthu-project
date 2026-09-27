; EXPECT: 15

define i32 @choose(i1 %condition, i32 %value) {
entry:
  br i1 %condition, label %duplicate, label %single

duplicate:
  %first = phi i32 [ %value, %entry ]
  %second = phi i32 [ %value, %entry ]
  %result = add i32 %first, %second
  ret i32 %result

single:
  ret i32 %value
}

define i32 @main() {
entry:
  %duplicated = call i32 @choose(i1 true, i32 5)
  %single = call i32 @choose(i1 false, i32 5)
  %result = add i32 %duplicated, %single
  ret i32 %result
}
