; EXPECT: 10

define i32 @choose(i1 %condition, i32 %first, i32 %second) {
entry:
  br i1 %condition, label %both, label %first_only

both:
  %result = add i32 %first, %second
  ret i32 %result

first_only:
  ret i32 %first
}

define i32 @main() {
entry:
  %both = call i32 @choose(i1 true, i32 3, i32 4)
  %first = call i32 @choose(i1 false, i32 3, i32 4)
  %result = add i32 %both, %first
  ret i32 %result
}
