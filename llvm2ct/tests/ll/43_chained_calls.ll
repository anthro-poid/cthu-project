; EXPECT: 42

define i32 @add(i32 %a, i32 %b) {
entry:
  %result = add i32 %a, %b
  ret i32 %result
}

define i32 @main() {
entry:
  %first = call i32 @add(i32 10, i32 20)
  %second = call i32 @add(i32 %first, i32 12)
  ret i32 %second
}
